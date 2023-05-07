#pragma once

#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <future>

#include <absl/time/time.h>
#include <absl/time/clock.h>

#include "model/predict_status.h"

namespace torch::serving {
class IServable;
class BatchConfig;
class PredictContext;

struct BatchTask {
  std::shared_ptr<PredictContext> context;
  std::promise<PredictStatus> promise;
  uint64_t size;
  std::shared_ptr<IServable> servable;
};
using BatchTaskPtr = std::shared_ptr<BatchTask>;

struct BatchTaskGroup {
  absl::Time begin_{absl::Now()};
  uint64_t item_size_{0};
  std::vector<BatchTaskPtr> task_list_;
};

using BatchTaskGroupPtr = std::shared_ptr<BatchTaskGroup>;

class ServableQueue {
 public:
  ServableQueue(uint32_t time_windows, uint32_t size_windows, uint32_t queue_size);

  void AddTask(const BatchTaskPtr& task);

  BatchTaskGroupPtr GetTaskGroup();

  void Stop() {
    is_stop_ = true;
  }

  bool IsStop() {
    return is_stop_ && group_list_.empty();
  }

 private:
  bool IsGroupClose(const BatchTaskGroupPtr& group) const;
 private:
  // 末尾入，头部出
  std::list<BatchTaskGroupPtr> group_list_;
  std::mutex mutex_;
  const uint32_t time_windows_;
  const uint32_t size_windows_;
  const uint32_t queue_size_;
  std::atomic_bool is_stop_{false};
};

using ServableQueuePtr = std::shared_ptr<ServableQueue>;

using ModelBatchQueue = std::list<ServableQueuePtr>;
using ServableQueueNode = ModelBatchQueue::iterator;

class SharedBatchScheduler {
 public:
  explicit SharedBatchScheduler(const BatchConfig& config);
  ~SharedBatchScheduler();
  ServableQueueNode AddQueue();

 private:
  void Work();

 private:
  ModelBatchQueue queue_;
  ServableQueueNode current_;
  std::mutex mutex_;
  std::vector<std::thread> workers_;
  std::atomic_bool running_{true};
  uint32_t time_windows_{0};
  uint32_t size_windows_{0};
  uint32_t queue_size_{0};
};

}
