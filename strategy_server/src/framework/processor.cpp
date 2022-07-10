#include "processor.h"
#include <glog/logging.h>

Processor::Processor(const GraphsConf& graphs_conf, size_t max_concurrent) : executor_(max_concurrent) {
  factory_ = std::make_unique<GraphFactory>(graphs_conf);
  for (const auto& graph_def : graphs_conf.graph_defs()) {
    std::string name = graph_def.name();
    auto handler_queue = std::make_unique<HandlerQueue>();
    for (size_t i=0; i<std::thread::hardware_concurrency(); ++i) {
      handler_queue->queue_.emplace(factory_->BuildNew(name, executor_));
    }
    queue_.emplace(name, handler_queue.release());
  }
}

bool Processor::Run(const StrategyRequest* request, StrategyResponse* response) {
  auto [name, request_type, response_type] = SelectGraph(request);
  auto it = queue_.find(name);
  if (it == queue_.end()) {
    // 没找到
    LOG(WARNING) << "graph [" << name << "] not found";
    return false;
  }
  std::unique_ptr<ProcessHandler> handler;
  auto& handle_queue = it->second;
  {
    std::lock_guard lock(handle_queue->mutex_);
    if (!handle_queue->queue_.empty()) {
      handler = std::move(handle_queue->queue_.front());
      handle_queue->queue_.pop();
    }
  }
  if (handler == nullptr) {
    handler = factory_->BuildNew(name, executor_);
  }
  bool result = handler->Run(request, request_type, response, response_type);
  {
    std::lock_guard lock(handle_queue->mutex_);
    handle_queue->queue_.push(std::move(handler));
  }
  return result;
}

void Processor::DumpGraph(std::ostream &os) {
  os << "use http://dreampuf.github.io/GraphvizOnline/ \n";
  for (const auto& [key, que] : queue_) {
    os << "\nname : " << key << "; graph: \n";
    que->queue_.front()->Dump(os);
  }
}
