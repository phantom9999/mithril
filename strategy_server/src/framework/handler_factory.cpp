#include "handler_factory.h"
#include "framework/op_kernel.h"

HandlerFactory::HandlerFactory(const GraphDef& graph_def) {
  for (const auto& opdef : graph_def.op_defs()) {
    std::vector<Session_Type> data(opdef.inputs_size());
    for (const auto& key : opdef.inputs()) {
      data.push_back(static_cast<Session_Type>(key));
    }
    graph_.emplace(opdef.output(), std::move(data));
  }
}

std::unique_ptr<ProcessHandler> HandlerFactory::BuildNew(tf::Executor& executor) {
  auto handler = std::make_unique<ProcessHandler>(executor);
  auto& opmap = handler->kernels_;
  std::unordered_map<Session_Type, tf::Task> task_map;
  // 绑定内核
  for (const auto& [output, inputs] : graph_) {
    auto* kernel = creaters_[output]->Create(inputs, output);
    auto task = handler->taskflow_.emplace([kernel](){
      kernel->Run();
    });
    task.data(&handler->global_context_);
    kernel->BindTask(task);
    opmap.emplace(output, kernel);
    task_map.emplace(output, task);
  }
  // 处理依赖问题
  for (const auto& [output, inputs] : graph_) {
    auto& task = task_map[output];
    for (const auto& key : inputs) {
      if (auto it = task_map.find(key); it != task_map.end()) {
        task.precede(it->second);
      }
    }
  }
  return handler;
}

