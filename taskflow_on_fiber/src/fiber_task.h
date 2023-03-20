#pragma once

#include <atomic>

#include <boost/noncopyable.hpp>

namespace engine {

/**
 * 通过上传行+模板隐藏差异
 */
class IFiberTask : public boost::noncopyable {
 public:
  IFiberTask() = default;
  virtual ~IFiberTask() = default;

  virtual void execute() = 0;

 public:
  inline static std::atomic_size_t fibers_size{0};
};

template<typename Func>
class FiberTask : public IFiberTask {
 public:
  explicit FiberTask(Func &&func) : func_{std::move(func)} {}

  ~FiberTask() override = default;

  /**
   * Run the task.
   */
  void execute() override {
    ++fibers_size;
    func_();
    --fibers_size;
  }

 private:
  Func func_;
};

}
