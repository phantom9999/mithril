#include "fiber_pool.h"
#include <boost/log/trivial.hpp>

namespace engine {

FiberPool::FiberPool(size_t thread_size, size_t queue_size)
  : thread_size_{thread_size}, task_queue_{queue_size} {
  try {
    for (std::uint32_t i = 0; i < thread_size_; ++i) {
      threads_.emplace_back(&FiberPool::Worker, this);
    }
  }
  catch (...) {
    CloseQueue();
    throw;
  }
}

FiberPool::~FiberPool() {
  for (auto &thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void FiberPool::Worker() {
  BOOST_LOG_TRIVIAL(info) << "add to pool";
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::work_stealing>(thread_size_, true);
  WorkTask task_tuple;
  while (boost::fibers::channel_op_status::success == task_queue_.pop(task_tuple)) {
    auto &[launch_policy, task_to_run] = task_tuple;
    boost::fibers::fiber(launch_policy, [task = std::move(task_to_run)]() {
      task->execute();
    }).detach();
  }
}



}

