#pragma once

#include <queue>
#include <memory>
#include <memory>

#include <taskflow/taskflow.hpp>

#include "framework/process_handler.h"
#include "framework/graph_factory.h"

struct HandlerQueue {
  std::queue<std::unique_ptr<ProcessHandler>> queue_;
  std::mutex mutex_;
};

class Processor {
public:
  explicit Processor(const GraphsConf& graphs_conf, size_t max_concurrent = std::thread::hardware_concurrency());

  bool Run(const StrategyRequest* request, StrategyResponse* response);

  std::tuple<std::string, Session_Type, Session_Type> SelectGraph(const StrategyRequest* request) {
    // todo: 选图的逻辑
    return std::make_tuple("xx", Session_Type_REQUEST1, Session_Type_RESPONSE1);
  }

private:
  std::unique_ptr<GraphFactory> factory_;
  tf::Executor executor_;
  std::unordered_map<std::string, std::unique_ptr<HandlerQueue>> queue_;
};

