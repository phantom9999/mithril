#include "model_manager.h"
#include <mutex>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <absl/strings/numbers.h>
#include <glog/logging.h>
#define GLOG_STL_LOGGING_FOR_UNORDERED
#include <glog/stl_logging.h>

#include "server_config.pb.h"

#include "servables/servable.h"
#include "servables/torch_factory.h"
#include "servables/onnx_factory.h"
#include "model/model_define.h"
#include "model/servable_model.h"
#include "batch/batch_factory.h"
#include "batch/shared_batch_scheduler.h"

namespace {

void ModelVersionCheck(const boost::filesystem::path& model_path, const std::unordered_set<torch::serving::ModelVersion>& current_versions, torch::serving::ModelState* model_state) {
  if (model_state == nullptr) {
    return;
  }
  boost::system::error_code ec;
  std::unordered_set<torch::serving::ModelVersion> exist_versions;
  for (const auto& version_dir : boost::filesystem::directory_iterator(model_path, ec)) {
    if (!boost::filesystem::is_directory(version_dir, ec)) {
      LOG(WARNING) << version_dir.path().string() << " is not dir";
      continue;
    }
    torch::serving::ModelVersion version;
    if (!absl::SimpleAtoi(version_dir.path().filename().string(), &version)) {
      LOG(WARNING) << version_dir.path().filename().string() << " is not number";
      continue;
    }
    // 版本检查
    if (current_versions.find(version) != current_versions.end()) {
      // 已经存在
      exist_versions.insert(version);
      continue;
    }
    // 新的版本，文件校验
    std::vector<boost::filesystem::path> check_files{
        version_dir / torch::serving::CHECK_FILE,
        version_dir / torch::serving::MD5_FILE,
        version_dir / torch::serving::FEATURE_SPEC_FILE,
    };
    bool ok = true;
    for (const auto& file : check_files) {
      if (!boost::filesystem::exists(file, ec) || !boost::filesystem::is_regular(file, ec)) {
        LOG(WARNING) << "miss " << file.string();
        ok = false;
        break;
      }
    }
    if (!ok) {
      LOG(WARNING) << "skip " << version_dir.path().string();
      continue;
    }
    model_state->to_add_list.push_back(version);
  }
  if (model_state->to_add_list.empty() && exist_versions.size() == current_versions.size()) {
    // 没有版本变化
    return;
  }
  for (const auto& version : current_versions) {
    if (exist_versions.find(version) == exist_versions.end()) {
      model_state->to_rm_list.push_back(version);
    }
  }
}
}

namespace torch::serving {
void ModelManager::Work() {
  boost::system::error_code ec;
  for (const auto& entry : model_paths_) {
    auto& model_name = entry.first;
    boost::filesystem::path model_dir(entry.second);
    if (!boost::filesystem::exists(model_dir, ec)) {
      // 目录不存在
      LOG(WARNING) << model_dir.string() << " not exist";
      continue;
    }
    if (!boost::filesystem::is_directory(model_dir, ec)) {
      LOG(WARNING) << model_dir.string() << " not dir";
      continue;
    }

    auto it = model_data_.find(model_name);
    if (it == model_data_.end()) {
      // 模型数据不存在
      LOG(WARNING) << "model version set miss";
      continue;
    }
    auto& torch_model = it->second;
    std::unordered_set<ModelVersion> version_set;
    torch_model->GetAllVersion(&version_set);

    ModelState model_status;
    ModelVersionCheck(model_dir, version_set, &model_status);
    if (model_status.to_add_list.empty() && model_status.to_rm_list.empty()) {
      // 不需要更新
      LOG(INFO) << model_name << " no need to update";
      continue;
    }
    torch_model->UpdateModel(&model_status);
  }
}
bool ModelManager::Init(const ModelManagerConfig &model_manager_config) {
  if (running_) {
    return false;
  }
  if (model_manager_config.has_batch_config() && model_manager_config.batch_config().enable()) {
    scheduler_ = std::make_shared<SharedBatchScheduler>(model_manager_config.batch_config());
  }
  for (const auto& config : model_manager_config.mode_configs()) {
    if (config.path().empty() || config.name().empty() || config.platform() == Define_Platform_UNKNOWN) {
      continue;
    }
    auto& model_name = config.name();
    auto& model_path = config.path();

    std::shared_ptr<ServableFactory> servable_factory;
    switch (config.platform()) {
      case Define_Platform_TORCH:{
        servable_factory = std::make_shared<TorchFactory>();
        break;
      }
      case Define_Platform_ONNX: {
        servable_factory = std::make_shared<OnnxFactory>();
        break;
      }
      default: continue;
    }
    if (config.use_batch() && scheduler_ != nullptr) {
      servable_factory = std::make_shared<BatchFactory>(servable_factory, scheduler_);
    }

    model_paths_[model_name] = model_path;
    auto servable_model = std::make_unique<ServableModel>(servable_factory, model_name, model_path);
    switch (config.policy().policy_choice_case()) {
      case ServableVersionPolicy::kLatest: {
        model_data_[model_name] = std::make_shared<LatestPolicyTorchModel>(std::move(servable_model), config.policy().latest().num_versions());
        LOG(INFO) << "add new model " << model_name << " in " << model_path
          << " use latest " << config.policy().latest().num_versions();
        break;
      }
      case ServableVersionPolicy::kAll: {
        model_data_[model_name] = std::make_shared<AllPolicyTorchModel>(std::move(servable_model));
        LOG(INFO) << "add new model " << model_name << " in " << model_path
                  << " use all ";
        break;
      }
      case ServableVersionPolicy::kSpecific: {
        auto& versions = config.policy().specific().versions();
        std::unordered_set<ModelVersion> data(versions.begin(), versions.end());
        model_data_[model_name] = std::make_shared<SpecificPolicyTorchModel>(std::move(servable_model), data);
        LOG(INFO) << "add new model " << model_name << " in " << model_path
                  << " use specific " << data;
        break;
      }
      case ServableVersionPolicy::POLICY_CHOICE_NOT_SET: {
        model_data_[model_name] = std::make_shared<AllPolicyTorchModel>(std::move(servable_model));
        LOG(INFO) << "add new model " << model_name << " in " << model_path
                  << " use default all ";
        break;
      }
    }
  }
  running_ = true;
  Work();
  worker_ = std::thread([&](){
    while (running_) {
      std::this_thread::sleep_for(std::chrono::seconds(model_manager_config.interval()));
      Work();
    }
  });
  return true;
}
void ModelManager::Stop() {
  running_ = false;
  worker_.join();
}
std::shared_ptr<IServable> ModelManager::GetServableByLabel(const std::string &name, const std::string &label) {
  auto it = model_data_.find(name);
  if (it == model_data_.end() || it->second == nullptr) {
    return nullptr;
  }
  auto& model = it->second;
  return model->GetServableByLabel(label);
}
std::shared_ptr<IServable> ModelManager::GetServableByVersion(const std::string &name, ModelVersion version) {
  auto it = model_data_.find(name);
  if (it == model_data_.end() || it->second == nullptr) {
    return nullptr;
  }
  auto& model = it->second;
  return model->GetServableByVersion(version);
}
void ModelManager::GetModelList(std::vector<std::string> *model_list) {
  if (model_list == nullptr) {
    return;
  }
  model_list->reserve(model_list->size());
  for (const auto& entry : model_paths_) {
    model_list->push_back(entry.first);
  }
}
std::shared_ptr<ModelLoadPolicy> ModelManager::GetModel(const std::string &name) {
  auto it = model_data_.find(name);
  if (it == model_data_.end()) {
    return nullptr;
  }
  return it->second;
}
bool ModelManager::Ready() const {
  return running_;
}

}
