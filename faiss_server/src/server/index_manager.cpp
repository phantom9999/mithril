#include "index_manager.h"
#include <sstream>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <absl/strings/numbers.h>
#include <boost/make_shared.hpp>

#include "server/server_flags.h"
#include "common/timer.h"
#include "common/constants.h"

IndexManager::IndexManager() : mulit_index_(proto::Constants::ModelName_ARRAYSIZE) {

}

bool IndexManager::Init(const proto::IndexConfig& index_config) {
  this->index_path_ = index_config.index_path();
  this->model_configs_.insert(model_configs_.end(), index_config.model_config().begin(), index_config.model_config().end());

  // 加载索引
  LoadModel();
  this->worker_ = std::thread([this](){
    while (this->running_.load()) {
      LoadModel();
      sleep(10);
    }
  });

  return true;
}

SearchStatus IndexManager::Search(const SearchParam &param, SearchResult *result) {
  auto& model_index = this->mulit_index_[param.model_name];
  if (model_index.version_.load() == 0) {
    return SearchStatus::MODEL_NOT_FOUND;
  }
  boost::shared_ptr<IndexWrapper> index = model_index.index_.load();
  if (index == nullptr) {
    return SearchStatus::MODEL_NOT_FOUND;
  }
  result->version = model_index.version_.load();
  return index->Search(param, result);
}
void IndexManager::LoadModel() {
  boost::filesystem::path index_dir(index_path_);
  boost::system::error_code ec;
  for (const auto& sub_config : model_configs_) {
    std::string model_name = proto::Constants::ModelName_Name(sub_config.model_name());
    auto model_dir = index_dir / model_name;

    uint64_t new_version{0};
    for (const auto &version_dir : boost::filesystem::directory_iterator(model_dir, ec)) {
      if (!is_directory(version_dir, ec)) {
        continue;
      }
      uint64_t current_version{0};
      std::string str_version = version_dir.path().filename().string();
      if (!absl::SimpleAtoi(str_version, &current_version)) {
        LOG(WARNING) << str_version << " is not number";
        continue;
      }
      if (current_version < new_version) {
        continue;
      }
      auto ids_file = version_dir / kFaissIdsName;
      if (!boost::filesystem::is_regular_file(ids_file, ec)) {
        continue;
      }
      new_version = current_version;
    }

    auto &model_index = this->mulit_index_[sub_config.model_name()];
    if (new_version <= model_index.version_.load()) {
      continue;
    }
    auto new_index = boost::make_shared<IndexWrapper>();
    auto version_path = model_dir / std::to_string(new_version);
    Timer timer;
    if (!new_index->Init(version_path.string())) {
      continue;
    }
    {
      auto old_index = model_index.index_.load();
      model_index.version_.store(new_version);
      model_index.index_.store(new_index);
    }
    LOG(INFO) << "model " << model_name << " add new version " << new_version << " cost " << timer.UsCost() << "us";
  }
}
bool IndexManager::GetStatus(const proto::Constants::ModelName model_name, uint64_t* version, std::vector<std::tuple<proto::Constants::IndexType, uint64_t, uint64_t>>* status) {
  auto& model_index = mulit_index_[model_name];
  if (model_index.version_.load() == 0) {
    return false;
  }
  auto index = model_index.index_.load();
  if (index == nullptr) {
    return false;
  }
  *version = model_index.version_.load();
  return index->Status(status);
}
bool IndexManager::Stop() {
  this->running_.store(false);
  this->worker_.join();
  return false;
}

