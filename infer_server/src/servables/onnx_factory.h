#pragma once

#include "servables/servable.h"

namespace torch::serving {

class OnnxFactory : public ServableFactory {
 public:
  std::shared_ptr<IServable> New() override;

};



}
