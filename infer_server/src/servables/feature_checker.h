#pragma once

#include "model/predict_status.h"

namespace inference {
class ModelInferRequest;
class ModelInferRequest_InferInputTensor;
class ModelSpec;
class FeatureSpec;
}

namespace torch::serving {

class FeatureChecker {
 public:
  PredictStatus Check(const inference::ModelInferRequest& request, const inference::ModelSpec& model_spec);
  PredictStatus FeatureCheck(const inference::ModelInferRequest_InferInputTensor& tensor_proto, const inference::FeatureSpec& feature);
};

}


