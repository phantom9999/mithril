#pragma once

#include "servables/servable.h"

namespace torch::serving {
class PredictContext;
class SharedBatchScheduler;
class ServableQueue;

class BatchServable : public IServable {
 public:
  BatchServable(const std::shared_ptr<IServable>& servable, const std::shared_ptr<ServableQueue>& servable_queue);
  bool Init(const std::string &path) override;
  PredictStatus Predict(const std::shared_ptr<PredictContext> &predict_context) override;
  const std::string &GetLabel() override;
  const inference::ModelSpec &GetSpec() override;
  PredictStatus PredictWithoutCheck(const std::shared_ptr<PredictContext> &predict_context) override;

  ~BatchServable() override;

 private:
  std::shared_ptr<IServable> servable_;
  std::shared_ptr<ServableQueue> servable_queue_;
};

}
