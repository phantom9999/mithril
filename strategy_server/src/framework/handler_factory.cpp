#include "handler_factory.h"
#include "framework/op_kernel.h"
#include <glog/logging.h>

HandlerFactory::HandlerFactory(const GraphDef& graph_def) {
  for (const auto& opdef : graph_def.op_defs()) {
    graph_.emplace(opdef.output(), opdef);
  }
}

std::unique_ptr<ProcessHandler> HandlerFactory::BuildNew(tf::Executor& executor) {
  auto handler = std::make_unique<ProcessHandler>(executor);
  auto& opmap = handler->kernels_;
  std::unordered_map<Session_Type, tf::Task> task_map;
  // 绑定内核
  for (const auto& [output, op_def] : graph_) {
    std::vector<Session_Type> inputs;
    for (const auto& key : op_def.inputs()) {
      inputs.push_back(static_cast<Session_Type>(key));
    }
    auto* kernel = creaters_[output](inputs, output);
    auto task = handler->taskflow_.emplace([kernel](){
      kernel->Run();
    });
    task.data(&handler->global_context_);
    task.name(op_def.name());
    kernel->BindTask(task);
    opmap.emplace(output, kernel);
    task_map.emplace(output, task);
  }
  // 处理依赖问题
  for (const auto& [output, op_def] : graph_) {
    auto& task = task_map[output];
    for (const auto& key : op_def.inputs()) {
      if (auto it = task_map.find(static_cast<Session_Type>(key)); it != task_map.end()) {
        task.succeed(it->second);
      }
    }
  }
  return handler;
}

bool HandlerFactory::Register(Session::Type key, Creater creater) {
  std::cout << "register " << Session_Type_Name(key) << std::endl;
  creaters_.emplace(key, std::move(creater));
  return true;
}

