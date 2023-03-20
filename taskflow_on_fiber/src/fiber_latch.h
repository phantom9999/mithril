#pragma once

#include <atomic>
#include <boost/noncopyable.hpp>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>

namespace engine {

class FiberLatch : public boost::noncopyable {
 public:
  explicit FiberLatch(int32_t size) : count_(size) { }
  ~FiberLatch();

  void CountDown();

  void Wait();

 private:
  std::atomic_int32_t count_;
  boost::fibers::mutex mutex_;
  boost::fibers::condition_variable condition_variable_;
};
}
