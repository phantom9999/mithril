#include "torch_factory.h"
#include "servables/torch_servable.h"

namespace torch::serving {


std::shared_ptr<IServable> TorchFactory::New() {
  return std::shared_ptr<TorchServable>();
}

}

