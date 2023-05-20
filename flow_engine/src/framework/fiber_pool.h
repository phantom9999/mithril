#pragma once

#include <optional>
#include <boost/fiber/all.hpp>
#include "fiber_task.h"

namespace flow {

using WorkTask = std::tuple<boost::fibers::launch, std::unique_ptr<flow::IFiberTask>>;
using FiberQueue = boost::fibers::buffered_channel<WorkTask>;

class FiberPool : boost::noncopyable {
 public:
  explicit FiberPool(size_t thread_size, size_t queue_size = 32);

  template<typename Func, typename... Args>
  auto Submit(boost::fibers::launch launch_policy, Func &&func, Args &&... args) {
    auto capture = [func = std::forward<Func>(func), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
      return std::apply(std::move(func), std::move(args));
    };

    using task_result_t = std::invoke_result_t<decltype(capture)>;
    using packaged_task_t = boost::fibers::packaged_task<task_result_t()>;
    packaged_task_t task{std::move(capture)};
    using task_t = flow::FiberTask<packaged_task_t>;
    auto result_future = task.get_future();
    auto status = task_queue_.push(std::make_tuple(
        launch_policy, std::make_unique<task_t>(std::move(task))
        ));

    if (status != boost::fibers::channel_op_status::success) {
      std::cout << "not success\n";
      return std::optional<std::decay_t<decltype(result_future)>>{};
    }
    return std::make_optional(std::move(result_future));
  }

  template<typename Func, typename... Args>
  auto Submit(Func &&func, Args &&... args) {
    return Submit(boost::fibers::launch::post, std::forward<Func>(func), std::forward<Args>(args)...);
  }

  void CloseQueue() noexcept {
    task_queue_.close();
  }

  ~FiberPool();

 private:
  void Worker();

  size_t thread_size_;
  std::vector<std::thread> threads_;
  FiberQueue task_queue_;
};

}
