#include "servable_model.h"
#include <queue>
#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <algorithm>

#include "servables/servable.h"

namespace torch::serving {

ServableModel::ServableModel(const std::shared_ptr<ServableFactory>& factory, const std::string &model_name, const std::string &model_path)
  :factory_(factory), model_name_(model_name), model_path_(model_path) {

}

std::shared_ptr<IServable> ServableModel::GetServableByLabel(const std::string &label) {
  std::shared_lock lock(shared_mutex_);
  auto it = label_to_version_.find(label);
  if (it == label_to_version_.end() || it->second.empty()) {
    return nullptr;
  }
  auto& versions = it->second;
  if (versions.empty()) {
    return nullptr;
  }
  auto sub_it = servable_verions_.find(versions.begin().operator*());
  if (sub_it == servable_verions_.end()) {
    return nullptr;
  }
  return sub_it->second;
}
std::shared_ptr<IServable> ServableModel::GetServableByVersion(ModelVersion model_version) {
  {
    std::shared_lock lock(shared_mutex_);
    if (servable_verions_.empty()) {
      return nullptr;
    }
  }
  if (model_version == 0) {
    // 使用最新版本
    std::shared_lock lock(shared_mutex_);
    auto it = servable_verions_.begin();
    return it->second;
  } else {
    std::shared_lock lock(shared_mutex_);
    auto it = servable_verions_.find(model_version);
    if (it == servable_verions_.end()) {
      return nullptr;
    }
    return it->second;
  }
  return nullptr;
}
bool ServableModel::AddServable(ModelVersion model_version) {
  auto version_path = boost::filesystem::path(model_path_) / std::to_string(model_version);
  auto servable = factory_->New();
  if (!servable->Init(version_path.string())) {
    return false;
  }
  {
    std::unique_lock lock(shared_mutex_);
    servable_verions_[model_version] = servable;
    label_to_version_[servable->GetLabel()].insert(model_version);
  }
  return true;
}
void ServableModel::RmServable(ModelVersion model_version) {
  std::unique_lock lock(shared_mutex_);
  auto it = servable_verions_.find(model_version);
  if (it == servable_verions_.end()) {
    return;
  }
  auto& servable = it->second;
  if (servable != nullptr) {
    label_to_version_[servable->GetLabel()].erase(model_version);
  }
  servable_verions_.erase(it);
}
void ServableModel::GetAllVersion(std::unordered_set<ModelVersion> *versions) {
  if (versions == nullptr) {
    return;
  }
  std::shared_lock lock(shared_mutex_);
  for (const auto& entry : servable_verions_) {
    versions->insert(entry.first);
  }
}

const std::string& ServableModel::GetName() {
  return model_name_;
}

void LatestPolicyTorchModel::UpdateModel(ModelState *model_state) {
  if (model_state == nullptr || num_ <= 0) {
    return;
  }
  std::unordered_set<ModelVersion> remain_set;
  wrapper_->GetAllVersion(&remain_set);
  for (const auto& item : model_state->to_rm_list) {
    remain_set.erase(item);
  }
  std::priority_queue<ModelVersion, std::vector<ModelVersion>, std::greater<>> remain_que(remain_set.begin(), remain_set.end());
  std::sort(model_state->to_add_list.begin(), model_state->to_add_list.end(), std::greater<>());

  std::unordered_set<ModelVersion> add_version;
  std::unordered_set<ModelVersion> rm_version;
  for (const auto& version : model_state->to_add_list) {
    if (remain_que.size() < num_) {
      // 直接加载
      if (!wrapper_->AddServable(version)) {
        continue;
      }
      remain_que.push(version);
      add_version.insert(version);
    } else {
      // 可能要淘汰版本
      ModelVersion last_version = remain_que.top();
      if (last_version > version) {
        // 不需要加载
        continue;
      }
      if (!wrapper_->AddServable(version)) {
        continue;
      }
      // 更新添加版本
      add_version.insert(version);
      remain_que.push(version);
      // 清理老版本
      wrapper_->RmServable(last_version);
      rm_version.insert(last_version);
      add_version.erase(last_version);
      remain_que.pop();
    }
  }
  model_state->to_add_list.clear();
  model_state->to_add_list.insert(model_state->to_add_list.end(), add_version.begin(), add_version.end());
  model_state->to_rm_list.insert(model_state->to_rm_list.end(), rm_version.begin(), rm_version.end());
}

ModelLoadPolicy::ModelLoadPolicy(std::unique_ptr<ServableModel> torch_model) : wrapper_(std::move(torch_model)) {

}
std::shared_ptr<IServable> ModelLoadPolicy::GetServableByLabel(const std::string &label) {
  if (wrapper_ == nullptr) {
    return nullptr;
  }
  return wrapper_->GetServableByLabel(label);
}
std::shared_ptr<IServable> ModelLoadPolicy::GetServableByVersion(ModelVersion model_version) {
  if (wrapper_ == nullptr) {
    return nullptr;
  }
  return wrapper_->GetServableByVersion(model_version);
}
void ModelLoadPolicy::GetAllVersion(std::unordered_set<ModelVersion> *versions) {
  wrapper_->GetAllVersion(versions);
}

LatestPolicyTorchModel::LatestPolicyTorchModel(std::unique_ptr<ServableModel> torch_model, uint32_t num)
  : ModelLoadPolicy(std::move(torch_model)), num_(num) {

}
void AllPolicyTorchModel::UpdateModel(ModelState *model_state) {
  if (model_state == nullptr) {
    return;
  }

  {
    std::vector<ModelVersion> update_version;
    update_version.reserve(model_state->to_add_list.size());
    for (const auto& version : model_state->to_add_list) {
      if (!wrapper_->AddServable(version)) {
        continue;
      }
      update_version.push_back(version);
      LOG(INFO) << wrapper_->GetName() << " add new version " << version;
    }
    model_state->to_add_list.swap(update_version);
  }
  {
    for (const auto& version : model_state->to_rm_list) {
      wrapper_->RmServable(version);
    }
  }
}
void SpecificPolicyTorchModel::UpdateModel(ModelState *model_state) {
  if (model_state == nullptr) {
    return;
  }
  {
    std::vector<ModelVersion> update_versions;
    update_versions.reserve(model_state->to_add_list.size());
    for (const auto& version: model_state->to_add_list) {
      if (specific_version_.find(version) == specific_version_.end()) {
        continue;
      }
      if (!wrapper_->AddServable(version)) {
        continue;
      }
      LOG(INFO) << wrapper_->GetName() << " add new version " << version;
      update_versions.push_back(version);
    }
    model_state->to_add_list.swap(update_versions);
  }
  {
    for (const auto& version : model_state->to_rm_list) {
      wrapper_->RmServable(version);
    }
  }
}
SpecificPolicyTorchModel::SpecificPolicyTorchModel(std::unique_ptr<ServableModel> torch_model, const std::unordered_set<ModelVersion> &versions)
  : ModelLoadPolicy(std::move(torch_model)), specific_version_(versions) {

}


}
