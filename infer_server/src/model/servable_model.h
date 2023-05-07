#pragma once

#include <shared_mutex>
#include <memory>
#include <map>
#include <set>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "model_define.h"

namespace torch::serving {

class IServable;
class ServableFactory;

struct ModelState {
  std::vector<ModelVersion> to_add_list;
  std::vector<ModelVersion> to_rm_list;
};

class ServableModel {
 public:
  ServableModel(const std::shared_ptr<ServableFactory>& factory, const std::string& model_name, const std::string& model_path);
  virtual ~ServableModel() = default;
  std::shared_ptr<IServable> GetServableByLabel(const std::string& label);

  std::shared_ptr<IServable> GetServableByVersion(ModelVersion model_version = 0);

  void GetAllVersion(std::unordered_set<ModelVersion>* versions);

  const std::string& GetName();

  bool AddServable(ModelVersion model_version);
  void RmServable(ModelVersion model_version);

 protected:
  const std::string model_name_;
  const std::string model_path_;
  std::shared_mutex shared_mutex_{};
  std::map<ModelVersion, std::shared_ptr<IServable>, std::greater<>> servable_verions_{};
  std::unordered_map<std::string, std::set<ModelVersion, std::greater<>>> label_to_version_;
  const std::shared_ptr<ServableFactory> factory_;
};

class ModelLoadPolicy {
 public:
  explicit ModelLoadPolicy(std::unique_ptr<ServableModel> torch_model);
  virtual ~ModelLoadPolicy() = default;
  virtual void UpdateModel(ModelState *model_state) = 0;

  std::shared_ptr<IServable> GetServableByLabel(const std::string& label);
  std::shared_ptr<IServable> GetServableByVersion(ModelVersion model_version = 0);
  void GetAllVersion(std::unordered_set<ModelVersion>* versions);

 protected:
  std::unique_ptr<ServableModel> wrapper_;
};

// 只保留版本最大的几个版本
class LatestPolicyTorchModel : public ModelLoadPolicy {
 public:
  LatestPolicyTorchModel(std::unique_ptr<ServableModel> torch_model, uint32_t num);
  void UpdateModel(ModelState *model_state) override;
 private:
  const uint32_t num_;
};

// 保留所有版本
class AllPolicyTorchModel : public ModelLoadPolicy {
 public:
  using ModelLoadPolicy::ModelLoadPolicy;
  void UpdateModel(ModelState *model_state) override;
};
// 保留指定版本
class SpecificPolicyTorchModel : public ModelLoadPolicy {
 public:
  SpecificPolicyTorchModel(std::unique_ptr<ServableModel> torch_model, const std::unordered_set<ModelVersion>& versions);
  void UpdateModel(ModelState *model_state) override;

 private:
  const std::unordered_set<ModelVersion> specific_version_;
};



}

