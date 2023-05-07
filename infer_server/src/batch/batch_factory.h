#pragma once

#include "servables/servable.h"

namespace torch::serving {

class SharedBatchScheduler;

class BatchFactory : public ServableFactory {
 public:
  BatchFactory(const std::shared_ptr<ServableFactory>& servable_factory, const std::shared_ptr<SharedBatchScheduler>& scheduler);
  std::shared_ptr<IServable> New() override;
 private:
  std::shared_ptr<ServableFactory> servable_factory_;
  std::shared_ptr<SharedBatchScheduler> scheduler_;
};


}

