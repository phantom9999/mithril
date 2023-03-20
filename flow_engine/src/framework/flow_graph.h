#pragma once

#include <vector>
#include <memory>
#include "flow_op.h"

namespace flow {

class FiberLatch;
class OpAttr;

// 节点
struct Node {
  int32_t name_{0};
  std::vector<int32_t> successors_; // 后侧节点
  std::vector<int32_t> dependents_; // 前部节点
  Creator creator_;
  std::shared_ptr<const OpAttr> attr_;
};
// 图
struct Graph {
  std::vector<std::unique_ptr<Node>> node_list_;
};

struct OpNode {
  int32_t name_{0};
  std::unique_ptr<Operator> op_;
  std::atomic_int32_t dependent_count_{};
  std::shared_ptr<FiberLatch> latch_;
  std::shared_ptr<FlowContext> flow_context_;
  const std::vector<OpNode*>* name_ops_;
  const std::vector<int32_t>* successors_;
};


}



