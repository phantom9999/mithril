#pragma once


#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include "servable.h"
#include "kserve_predict_v2.pb.h"

namespace torch::serving {

class OnnxServable : public IServable {
 public:
  bool Init(const std::string &path) override;
  PredictStatus PredictWithoutCheck(const std::shared_ptr<PredictContext> &predict_contexts) override;
  const std::string &GetLabel() override;
  const inference::ModelSpec &GetSpec() override;

 private:
  PredictStatus BuildInputTensor(const inference::ModelInferRequest& request, std::vector<Ort::Value>* input_tensors);

  PredictStatus ParseOutputTensor(const std::vector<Ort::Value>& output_tensors, inference::ModelInferResponse* response);

 private:
  struct GlobalEnv;
  struct Module;
  static std::shared_ptr<GlobalEnv> global_env_;
  std::shared_ptr<Module> a_module_;
};

}

