#pragma once

#include <memory>
#include <unordered_map>
#include <memory>

#include "proto/graph.pb.h"
#include "framework/process_handler.h"
#include "framework/handler_factory.h"

class GraphFactory {
public:
  explicit GraphFactory(const GraphsConf& graphs_conf);

  std::unique_ptr<ProcessHandler> BuildNew(const std::string& name, tf::Executor& executor);

private:
  std::unordered_map<std::string, std::unique_ptr<HandlerFactory>> factories_;
};
