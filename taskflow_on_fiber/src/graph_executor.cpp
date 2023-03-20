#include "graph_executor.h"
#include <memory>
#include <unordered_map>
#include "graph.h"
#include "fiber_pool.h"

namespace engine {

GraphSession::GraphSession(const std::shared_ptr<FrozenGraph>& graph, const std::shared_ptr<FiberPool>& fiber_pool)
    : frozen_graph_(graph), fiber_pool_(fiber_pool) { }

void GraphSession::Run() {
  if (run_) {
    return;
  }
  run_ = true;
  auto fu = fiber_pool_->Submit(&GraphSession::Work, this);
  try {
    if (fu.has_value()) {
      fu->wait();
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }
}

void GraphSession::Work() {
  std::vector<std::unique_ptr<NodeView>> node_view_list;
  // 建立依赖关系
  std::unordered_map<Node *, NodeView *> nodeToMap;
  for (const auto &node : frozen_graph_->nodes_) {
    node_view_list.push_back(std::make_unique<NodeView>(node.get(), this));
    nodeToMap.insert({node.get(), node_view_list.back().get()});
  }
  for (const auto &node : frozen_graph_->nodes_) {
    auto it = nodeToMap.find(node.get());
    if (it == nodeToMap.end()) {
      continue;
    }
    auto *node_view = it->second;
    node_view->dependent_count_ = node->dependents_.size();
    for (const auto &sub_node : node->successors_) {
      auto sub_it = nodeToMap.find(sub_node);
      if (sub_it == nodeToMap.end()) {
        continue;
      }
      node_view->successors_.push_back(sub_it->second);
    }
  }
  nodeToMap.clear();
  fiber_latch_ = std::make_unique<FiberLatch>(node_view_list.size());

  // 找到 root
  std::vector<NodeView *> roots;
  for (const auto &node : node_view_list) {
    if (node->dependent_count_ == 0) {
      boost::fibers::fiber(boost::fibers::launch::post, CallBack, node.get()).detach();
    }
  }
  fiber_latch_->Wait();
}

void GraphSession::CallBack(NodeView *node_view) {
  node_view->node_->runnable_();
  node_view->dependent_count_ -= 1;

  for (auto &sub_node : node_view->successors_) {
    auto remain = sub_node->dependent_count_.fetch_sub(1);
    if (remain == 1) {
      // 没有依赖，可以运行
      boost::fibers::fiber(boost::fibers::launch::post, CallBack, sub_node).detach();
    }
  }
  node_view->graph_session_->fiber_latch_->CountDown();
}

GraphExecutor::GraphExecutor(size_t thread_size, size_t queue_size) {
  fiber_pool_ = std::make_shared<FiberPool>(thread_size, queue_size);
}

GraphExecutor::~GraphExecutor() {
  fiber_pool_->CloseQueue();
}

std::unique_ptr<GraphSession> GraphExecutor::BuildNewSession(const std::shared_ptr<FrozenGraph>& graph) {
  return std::make_unique<GraphSession>(graph, fiber_pool_);
}


}

