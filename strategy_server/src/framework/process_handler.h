#pragma once

#include <taskflow/taskflow.hpp>
#include "proto/graph.pb.h"
#include "framework/kernel_context.h"
#include "proto/service.pb.h"

class OpKernel;

class ProcessHandler {
public:
  explicit ProcessHandler(tf::Executor& executor): executor_(executor) { }
  /**
   * 非线程安全
   * @param request
   * @param response
   * @param response_type
   * @return
   */
  bool Run(const StrategyRequest* request, Session::Type request_type, StrategyResponse* response, Session_Type response_type);

  std::string Dump();

  void Dump(std::ostream& os);

private:
  tf::Executor& executor_;
  tf::Taskflow taskflow_;
  KernelContext global_context_;
  std::unordered_map<Session_Type, OpKernel*> kernels_;

  friend class HandlerFactory;
};
