#pragma once

#include "fiber_pool.h"

namespace fiber {

class DefaultPool {
 private:
  static auto* Pool() {
    const static size_t size = std::thread::hardware_concurrency();
    static fiber::FiberPool pool(size, size*8);
    return &pool;
  }

 public:
  template<typename Func, typename... Args>
  static auto SubmitJob(boost::fibers::launch launch_policy, Func &&func, Args &&... args) {
    return Pool()->Submit(launch_policy, std::forward<Func>(func), std::forward<Args>(args)...);
  }

  template<typename Func, typename... Args>
  static auto SubmitJob(Func &&func, Args &&... args) {
    return Pool()->Submit(std::forward<Func>(func), std::forward<Args>(args)...);
  }

  static void Close() {
    Pool()->CloseQueue();
  }

 private:
  DefaultPool() = default;
};
}
