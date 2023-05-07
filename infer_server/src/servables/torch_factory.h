#pragma once

#include "servables/servable.h"

namespace torch::serving {

class TorchFactory : public ServableFactory {
 public:

  std::shared_ptr<IServable> New() override;
};


}


