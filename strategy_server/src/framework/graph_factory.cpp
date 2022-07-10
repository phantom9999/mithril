#include "graph_factory.h"

GraphFactory::GraphFactory(const GraphsConf& graphs_conf) {
  for (const auto& graph_def : graphs_conf.graph_defs()) {
    factories_.emplace(graph_def.name(), std::make_unique<HandlerFactory>(graph_def));
  }
}

std::unique_ptr<ProcessHandler> GraphFactory::BuildNew(const std::string& name, tf::Executor& executor) {
  auto it = factories_.find(name);
  if (it == factories_.end()) {
    return nullptr;
  }
  return it->second->BuildNew(executor);
}