#include "fiber_latch.h"
#include <mutex>

namespace engine {

FiberLatch::~FiberLatch() {
  std::unique_lock<boost::fibers::mutex> lock(mutex_);
}

void FiberLatch::CountDown() {
  int32_t current = count_.fetch_sub(1, std::memory_order_release);
  if (current <= 1) {
    std::unique_lock<boost::fibers::mutex> lock(mutex_);
    condition_variable_.notify_all();
  }
}

void FiberLatch::Wait() {
  if (count_.load(std::memory_order_acquire) > 0) {
    std::unique_lock<boost::fibers::mutex> lock(mutex_);
    condition_variable_.wait(lock, [&](){
      return count_.load(std::memory_order_acquire) <= 0;
    });
  }
}

}
