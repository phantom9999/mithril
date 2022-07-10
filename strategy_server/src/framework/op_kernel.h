#pragma once

#include <vector>
#include <taskflow/taskflow.hpp>
#include "proto/graph.pb.h"
#include "framework/kernel_context.h"

class OpKernel {
public:
  void Run();
  void BindMeta(const std::vector<Session::Type>& inputs, Session::Type output);

protected:
  virtual std::shared_ptr<std::any> Compute(const KernelContext& context) = 0;

  virtual void Clear() { }

private:
  void BindTask(const tf::Task& task);

private:
  tf::Task task_;
  std::vector<Session::Type> inputs_;
  Session::Type output_;
  friend class HandlerFactory;
};

