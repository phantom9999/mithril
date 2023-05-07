#include "batch_factory.h"

#include "batch/batch_servable.h"
#include "batch/shared_batch_scheduler.h"

namespace torch::serving {

std::shared_ptr<IServable> BatchFactory::New() {
  return std::make_shared<BatchServable>(servable_factory_->New(), *scheduler_->AddQueue());
}
BatchFactory::BatchFactory(const std::shared_ptr<ServableFactory>& servable_factory, const std::shared_ptr<SharedBatchScheduler> &scheduler)
    : servable_factory_(servable_factory), scheduler_(scheduler) {

}

}

