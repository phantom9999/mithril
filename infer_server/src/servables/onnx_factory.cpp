#include "onnx_factory.h"
#include "servables/onnx_servable.h"

namespace torch::serving {

std::shared_ptr<IServable> OnnxFactory::New() {
  return std::make_shared<OnnxServable>();
}
}
