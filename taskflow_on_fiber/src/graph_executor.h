#pragma once

#include <memory>
#include "fiber_pool.h"
#include "fiber_latch.h"

namespace engine {

class FrozenGraph;
class NodeView;
class FiberPool;

// 一次执行的session
class GraphSession {
 public:
  explicit GraphSession(const std::shared_ptr<FrozenGraph>& graph, const std::shared_ptr<FiberPool>& fiber_pool);

  void Run();

 private:
  void Work();

  static void CallBack(NodeView *node_view);

 private:
  std::shared_ptr<FrozenGraph> frozen_graph_;
  std::unique_ptr<FiberLatch> fiber_latch_;
  bool run_{false};
  std::shared_ptr<FiberPool> fiber_pool_;
};

// 图执行器
class GraphExecutor : public boost::noncopyable {
 public:
  explicit GraphExecutor(size_t thread_size = std::thread::hardware_concurrency(),
                         size_t queue_size = std::thread::hardware_concurrency() * 8);
  ~GraphExecutor();

  std::unique_ptr<GraphSession> BuildNewSession(const std::shared_ptr<FrozenGraph>& graph);

 private:
  std::shared_ptr<FiberPool> fiber_pool_;
};
}
