#pragma once

#include <string>
#include <memory>

#include "model/predict_status.h"
#include "servables/feature_checker.h"
#include "model/model_define.h"

namespace inference {
class ModelSpec;
}

namespace torch::serving {
class PredictContext;

class IServable {
 public:
  virtual ~IServable() = default;
  virtual bool Init(const std::string &path) = 0;
  virtual PredictStatus Predict(const std::shared_ptr<PredictContext>& predict_contexts);
  virtual PredictStatus PredictWithoutCheck(const std::shared_ptr<PredictContext>& predict_context) = 0;
  virtual const std::string& GetLabel() = 0;
  virtual const inference::ModelSpec& GetSpec() = 0;

  PredictStatus Check(const std::shared_ptr<PredictContext>& predict_context);

 protected:
  bool ReadSpec(const std::string& model_dir, inference::ModelSpec* model_spec);
  bool ReadVersion(const std::string& model_dir, int64_t* model_version);
  bool Warmup(const std::string& model_dir);
 private:
  FeatureChecker checker_;
};

class ServableFactory {
 public:
  virtual ~ServableFactory() = default;
  virtual std::shared_ptr<IServable> New() = 0;
};


}

