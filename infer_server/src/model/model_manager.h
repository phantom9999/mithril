#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <memory>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <utility>

#include <boost/noncopyable.hpp>
#include "model/model_define.h"

namespace torch::serving {
class IServable;
class ModelLoadPolicy;
class ModelManagerConfig;
class SharedBatchScheduler;

class ModelManager : public boost::noncopyable {
 public:
  bool Init(const ModelManagerConfig& model_manager_config);

  std::shared_ptr<IServable> GetServableByLabel(const std::string& name, const std::string& label);
  std::shared_ptr<IServable> GetServableByVersion(const std::string& name, ModelVersion version = 0);
  std::shared_ptr<ModelLoadPolicy> GetModel(const std::string& name);

  void GetModelList(std::vector<std::string>* model_list);

  void Stop();

  bool Ready() const;

 private:

  void Work();

 private:
  std::unordered_map<std::string, std::string> model_paths_;  // <name, path>, 只读
  std::unordered_map<std::string, std::shared_ptr<ModelLoadPolicy>> model_data_;  // <name, model>, 外层只读，内侧读写
  std::thread worker_;
  bool running_{false};
  std::shared_ptr<SharedBatchScheduler> scheduler_{};
};

}
