#pragma once
#include <atomic>
#include <boost/noncopyable.hpp>

namespace fiber {

class IFiberTask {
 public:
  IFiberTask() = default;
  virtual ~IFiberTask() = default;

  IFiberTask(const IFiberTask& rhs) = delete;
  IFiberTask& operator=(const IFiberTask& rhs) = delete;
  IFiberTask(IFiberTask&& other) = default;
  IFiberTask& operator=(IFiberTask&& other) = default;

  virtual void execute() = 0;
 public:
  inline static std::atomic_size_t fibers_size {0};
};

template <typename Func>
class FiberTask: public IFiberTask
{
 public:
  explicit FiberTask(Func&& func) :func_{std::move(func)} { }

  ~FiberTask() override = default;
  FiberTask(const FiberTask& rhs) = delete;
  FiberTask& operator=(const FiberTask& rhs) = delete;
  FiberTask(FiberTask&& other)  noexcept = default;
  FiberTask& operator=(FiberTask&& other)  noexcept = default;

  void execute() override {
    fibers_size.fetch_add(1);
    func_();
    fibers_size.fetch_sub(1);
  }

 private:
  Func func_;
};


}
