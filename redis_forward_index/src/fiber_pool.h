#pragma once

#include <optional>
#include <boost/fiber/all.hpp>

#include "fiber_task.h"


namespace fiber {

using WorkTask = std::tuple<boost::fibers::launch, std::unique_ptr<fiber::IFiberTask>>;
using FiberQueue = boost::fibers::buffered_channel<WorkTask>;

class FiberPool {
 public:
  FiberPool() : FiberPool {std::thread::hardware_concurrency()} { }

  explicit FiberPool(size_t threads_size, size_t work_queue_size = 32)
    : threads_size_ {threads_size}, work_queue_ {work_queue_size} {
    try {
      for(std::uint32_t i = 0; i < threads_size_; ++i) {
        threads_.emplace_back(&FiberPool::Worker, this);
      }
    } catch(...) {
      CloseQueue();
      throw;
    }
  }

  template<typename Func, typename... Args>
  auto Submit(boost::fibers::launch launch_policy, Func&& func, Args&&... args) {
    auto capture = [func = std::forward<Func>(func),
        args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
      return std::apply(std::move(func), std::move(args));
    };

    using task_result_t = std::invoke_result_t<decltype(capture)>;

    using packaged_task_t = boost::fibers::packaged_task<task_result_t()>;

    packaged_task_t task {std::move(capture)};

    using task_t = fiber::FiberTask<packaged_task_t>;

    auto result_future = task.get_future();

    auto status = work_queue_.push(
        std::make_tuple(launch_policy, std::make_unique<task_t>(std::move(task))));

    if (status != boost::fibers::channel_op_status::success) {
      return std::optional<std::decay_t<decltype(result_future)>> {};
    }

    return std::make_optional(std::move(result_future));
  }

  template<typename Func, typename... Args>
  auto Submit(Func&& func, Args&&... args) {
    return Submit(boost::fibers::launch::post, std::forward<Func>(func), std::forward<Args>(args)...);
  }

  FiberPool(FiberPool const& rhs) = delete;

  FiberPool& operator=(FiberPool const& rhs) = delete;

  void CloseQueue() noexcept {
    work_queue_.close();
  }

  auto ThreadsSize() const noexcept {
    return threads_.size();
  }

  auto FibersSize() const noexcept {
    return fiber::IFiberTask::fibers_size.load();
  }

  ~FiberPool() {
    for(auto& thread : threads_) {
      if(thread.joinable()) {
        thread.join();
      }
    }
  }

 private:
  void Worker() {
    boost::fibers::use_scheduling_algorithm<boost::fibers::algo::work_stealing>(threads_size_, true);
    auto task_tuple = typename decltype(work_queue_)::value_type {};

    while(boost::fibers::channel_op_status::success == work_queue_.pop(task_tuple)) {
      auto& [launch_policy, task_to_run] = task_tuple;
      boost::fibers::fiber(launch_policy, [task = std::move(task_to_run)]() {
        task->execute();
      }).detach();
    }
  }

  const size_t threads_size_;

  std::vector<std::thread> threads_;

  FiberQueue work_queue_;
};
}