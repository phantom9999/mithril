#pragma once

#include <vector>
#include <taskflow/taskflow.hpp>
#include "proto/graph.pb.h"
#include "framework/kernel_context.h"

class OpKernel {
public:
  OpKernel(const std::vector<Session::Type>& inputs, Session::Type output);

  void BindTask(const tf::Task& task);

  void Run();

protected:
  virtual std::shared_ptr<std::any> Compute(const KernelContext& context) = 0;

private:
  tf::Task task_;
  std::vector<Session::Type> inputs_;
  Session::Type output_;
};

