#pragma once

#include <memory>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

namespace flow {
class Graph;
class FlowContext;
class FiberPool;
class OpNode;
class ExecutorDef;
class GraphDef;

// 执行
class GraphSession : boost::noncopyable {
 public:
  explicit GraphSession(const std::shared_ptr<Graph>& graph, const std::shared_ptr<FiberPool>& fiber_pool);

  void Run();

  std::shared_ptr<FlowContext> GetContext();

 private:
  void Work();

  static void CallBack(OpNode *op_node);

 private:
  std::shared_ptr<Graph> graph_;
  bool run_{false};
  std::shared_ptr<FlowContext> flow_context_;
  std::shared_ptr<FiberPool> fiber_pool_;
};


// 构建执行图
class GraphExecutor : public boost::noncopyable {
 public:
  ~GraphExecutor();
  bool Init(const std::string& filename);

  bool Init(const ExecutorDef& executor_def);

  std::unique_ptr<GraphSession> BuildGraphSession(uint32_t graph_name);

  std::shared_ptr<Graph> BuildGraph(const GraphDef& graph_def);

 private:
  std::vector<std::shared_ptr<Graph>> graphs_;
  std::shared_ptr<flow::FiberPool> fiber_pool_;
};


}



