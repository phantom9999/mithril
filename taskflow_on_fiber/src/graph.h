#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include <functional>
#include <atomic>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <boost/fiber/all.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include "fiber_pool.h"
#include "fiber_latch.h"

namespace engine {

using Runnable = std::function<void()>;

class Node;
class FrozenGraph;
class Task;
class GraphBuilder;
class GraphSession;
class NodeView;


// 包含算子，并且保留依赖关系
class Node {
 public:
  explicit Node(Runnable &&runnable) : runnable_(std::move(runnable)) {}

 private:
  void Precede(Node *node) {
    successors_.push_back(node);
    node->dependents_.push_back(node);
  }
  void Freeze() {
    // 避免多次添加
    {
      std::unordered_set<Node*> data{successors_.begin(), successors_.end()};
      successors_.clear();
      successors_.insert(successors_.end(), data.begin(), data.end());
    }
    {
      std::unordered_set<Node*> data{dependents_.begin(), dependents_.end()};
      dependents_.clear();
      dependents_.insert(dependents_.end(), data.begin(), data.end());
    }
  }

 private:
  Runnable runnable_;
  std::string name_;
  std::vector<Node *> successors_; // 后置
  std::vector<Node *> dependents_; // 前置
  friend class NodeView;
  friend class FrozenGraph;
  friend class Task;
  friend class GraphSession;
  friend class GraphBuilder;
};

// 维护Node的生命周期
class FrozenGraph {
 private:
  Node * Add(Runnable &&runnable) {
    nodes_.push_back(std::make_unique<Node>(std::move(runnable)));
    return nodes_.back().get();
  }

 private:
  std::vector<std::unique_ptr<Node>> nodes_{};
  friend class GraphBuilder;
  friend class GraphSession;
};

// 操作Node之间的依赖关系
class Task {
 public:
  explicit Task(Node *node) : node_(node) {}

  // 添加后置任务
  template<typename... Ts>
  Task &Precede(Ts &&... tasks) {
    (node_->Precede(tasks.node_), ...);
    return *this;
  }

  // 添加前置任务
  template<typename... Ts>
  Task &Succeed(Ts &&... tasks) {
    (tasks.node_->Precede(node_), ...);
    return *this;
  }

  Task& Name(const std::string& name) {
    node_->name_ = name;
    return *this;
  }

 private:
  Node *const node_;
};

// 构建graph用的
class GraphBuilder {
 public:
  GraphBuilder() {
    graph_ = std::make_unique<FrozenGraph>();
  }
  Task Add(Runnable &&runnable) {
    return Task(graph_->Add(std::move(runnable)));
  }

  std::shared_ptr<FrozenGraph> Freeze() {
    auto old = graph_;
    for (auto&& node : old->nodes_) {
      node->Freeze();
    }
    graph_ = std::make_shared<FrozenGraph>();
    return old;
  }

 private:
  std::shared_ptr<FrozenGraph> graph_;
};


// 每次执行时的Node
class NodeView {
 public:
  NodeView(Node* const node, GraphSession* graph_session) : node_(node), graph_session_(graph_session) { }
 private:
  Node* const node_;
  std::vector<NodeView *> successors_;
  std::atomic_int32_t dependent_count_{};
  GraphSession* const graph_session_;
  friend class GraphSession;
};

}
