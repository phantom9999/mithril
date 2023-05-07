#include "batch_servable.h"
#include "utils/item_utils.h"
#include "model/predict_context.h"
#include "batch/shared_batch_scheduler.h"

namespace torch::serving {

bool BatchServable::Init(const std::string &path) {
  return servable_ != nullptr && servable_queue_ != nullptr && servable_->Init(path);
}
PredictStatus BatchServable::Predict(const std::shared_ptr<PredictContext> &predict_context) {
  if (servable_ == nullptr) {
    return {PredictStatus::NULLPTR};
  }
  if (servable_queue_ == nullptr) {
    return servable_->Predict(predict_context);
  }

  auto check_status = servable_->Check(predict_context);
  if (!check_status.Ok()) {
    return check_status;
  }

  auto task = std::make_shared<BatchTask>();
  task->servable = servable_;
  task->context = predict_context;
  task->size = CountItem(*predict_context->request_);
  auto fu = task->promise.get_future();
  servable_queue_->AddTask(task);
  try {
    return fu.get();
  } catch (...) {
    return {PredictStatus::PREDICT_ERROR};
  }
}

const std::string &BatchServable::GetLabel() {
  return servable_->GetLabel();
}
BatchServable::BatchServable(const std::shared_ptr<IServable> &servable, const std::shared_ptr<ServableQueue>& servable_queue)
  : servable_(servable), servable_queue_(servable_queue) {

}
BatchServable::~BatchServable() {
  servable_queue_->Stop();
}
const inference::ModelSpec &BatchServable::GetSpec() {
  return servable_->GetSpec();
}
PredictStatus BatchServable::PredictWithoutCheck(const std::shared_ptr<PredictContext> &predict_context) {
  return {PredictStatus::OK};
}


}

