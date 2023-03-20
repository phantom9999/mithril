#include "flow_executor.h"

#include <vector>
#include <queue>

#include <boost/log/trivial.hpp>
#include <google/protobuf/text_format.h>

#include "graph.pb.h"
#include "flow_context.h"
#include "fiber_pool.h"
#include "fiber_latch.h"
#include "flow_op.h"
#include "flow_graph.h"

namespace {

bool HasDupNode(const flow::GraphDef& graph_def) {
  std::unordered_set<int32_t> node_set;
  for (const auto& node : graph_def.nodes()) {
    if (!node_set.insert(node.name()).second) {
      return true;
    }
  }
  return false;
}

bool HasDupEdge(const flow::GraphDef& graph_def) {
  std::set<std::pair<int32_t, int32_t>> edge_set;
  for (const auto& edge : graph_def.edges()) {
    if (!edge_set.insert({edge.from(), edge.to()}).second) {
      return true;
    }
  }
  return false;
}

bool HasRing(const flow::GraphDef& graph_def) {
  std::set<std::pair<int32_t, int32_t>> from_set;
  for (const auto& edge : graph_def.edges()) {
    from_set.insert({edge.from(), edge.to()});
  }
  std::vector<int32_t> degree(flow::FlowDefine::NodeName_ARRAYSIZE, 0);
  for (const auto& entry : from_set) {
    degree[entry.second] += 1;
  }
  std::unordered_multimap<int32_t, int32_t> from_to(from_set.begin(), from_set.end());
  std::queue<int32_t> que;
  std::unordered_set<int32_t> node_set;
  for (const auto& node : graph_def.nodes()) {
    int32_t name = node.name();
    if (!node_set.insert(name).second) {
      continue;
    }
    if (degree[name] == 0) {
      que.push(name);
    }
  }
  int rm_count = 0;
  while (!que.empty()) {
    int32_t name = que.front();
    que.pop();
    ++rm_count;
    auto it = from_to.equal_range(name);
    for (auto sub_it = it.first; sub_it != it.second; ++sub_it) {
      int32_t sub_name = sub_it->second;
      degree[sub_name] -= 1;
      if (degree[sub_name] == 0) {
        que.push(sub_name);
      }
    }
  }
  return rm_count != node_set.size();
}

bool MissOp(const flow::GraphDef& graph_def) {
  for (const auto& node : graph_def.nodes()) {
    if (!flow::OperatorCollector::Instance().Has(node.op_name())) {
      BOOST_LOG_TRIVIAL(warning) << flow::FlowDefine::NodeName_Name(node.name()) << " miss op " << node.op_name();
      return true;
    }
  }
  return false;
}

bool GraphCheck(const flow::GraphDef& graph_def) {
  using flow::FlowDefine;
  if (HasDupEdge(graph_def)) {
    BOOST_LOG_TRIVIAL(warning) << FlowDefine::GraphName(graph_def.name()) << " has dup edge";
    return false;
  }
  if (HasDupNode(graph_def)) {
    BOOST_LOG_TRIVIAL(warning) << FlowDefine::GraphName(graph_def.name()) << " has dup node";
    return false;
  }
  if (MissOp(graph_def)) {
    BOOST_LOG_TRIVIAL(warning) << FlowDefine::GraphName(graph_def.name()) << " has miss op";
    return false;
  }
  if (HasRing(graph_def)) {
    BOOST_LOG_TRIVIAL(warning) << FlowDefine::GraphName(graph_def.name()) << " has ring";
    return false;
  }
  return true;
}


}


namespace flow {


GraphSession::GraphSession(const std::shared_ptr<Graph>& graph, const std::shared_ptr<flow::FiberPool>& fiber_pool)
    : graph_(graph), fiber_pool_(fiber_pool) {
  flow_context_ = std::make_shared<FlowContext>(flow::FlowDefine::ContextName_ARRAYSIZE);
}

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

std::shared_ptr<FlowContext> GraphSession::GetContext() {
  return flow_context_;
}

void GraphSession::Work() {
  std::vector<std::unique_ptr<OpNode>> op_list;
  op_list.reserve(graph_->node_list_.size());
  std::vector<OpNode*> name_op(flow::FlowDefine::NodeName_ARRAYSIZE);

  // 创建实例
  auto latch = std::make_shared<flow::FiberLatch>(graph_->node_list_.size());
  for (const auto &node : graph_->node_list_) {
    auto op_node = std::make_unique<OpNode>();
    op_node->op_ = node->creator_(node->attr_);
    op_node->name_ = node->name_;
    op_node->successors_ = &node->successors_;
    op_node->name_ops_ = &name_op;
    op_node->latch_ = latch;
    op_node->dependent_count_ = node->dependents_.size();
    op_node->flow_context_ = flow_context_;
    name_op[node->name_] = op_node.get();
    op_list.push_back(std::move(op_node));
  }
  // 找到 root
  std::vector<OpNode *> roots;
  for (const auto &node : op_list) {
    using flow::FlowDefine;
    if (node->dependent_count_ == 0) {
      BOOST_LOG_TRIVIAL(info) << FlowDefine::NodeName_Name(static_cast<FlowDefine::NodeName>(node->name_))
                              << " has deps " << node->dependent_count_;
      boost::fibers::fiber(boost::fibers::launch::post, CallBack, node.get()).detach();
    }
  }
  latch->Wait();
}

void GraphSession::CallBack(OpNode *op_node) {
  op_node->op_->Compute(op_node->flow_context_);
  op_node->latch_->CountDown();
  if (op_node->successors_ == nullptr || op_node->name_ops_ == nullptr) {
    return;
  }

  for (auto &name : *op_node->successors_) {
    auto* sub_node = (*op_node->name_ops_)[name];
    if (sub_node == nullptr) {
      using flow::FlowDefine;
      BOOST_LOG_TRIVIAL(warning) << "node: " << FlowDefine::NodeName_Name(static_cast<FlowDefine::NodeName>(name))
                                 << " is nullptr";
      continue;
    }
    auto remain = sub_node->dependent_count_.fetch_sub(1);
    using flow::FlowDefine;
    BOOST_LOG_TRIVIAL(info) << "left: " << FlowDefine::NodeName_Name(static_cast<FlowDefine::NodeName>(op_node->name_))
                            << " right: "
                            << FlowDefine::NodeName_Name(static_cast<FlowDefine::NodeName>(sub_node->name_))
                            << " has deps " << remain - 1;
    if (remain == 1) {
      // 没有依赖，可以运行
      boost::fibers::fiber(boost::fibers::launch::post, CallBack, sub_node).detach();
    }
  }

}

GraphExecutor::~GraphExecutor() {
  if (fiber_pool_ != nullptr) {
    fiber_pool_->CloseQueue();
  }
}

bool GraphExecutor::Init(const std::string& filename) {
  flow::ExecutorDef executor_def;
  auto file = open(filename.c_str(), O_RDONLY);
  if (file < 0) {
    BOOST_LOG_TRIVIAL(warning) << "open " << filename << " fail";
    return false;
  }
  google::protobuf::io::FileInputStream reader{file};
  reader.SetCloseOnDelete(true);
  if (!google::protobuf::TextFormat::Parse(&reader, &executor_def)) {
    BOOST_LOG_TRIVIAL(warning) << "parse " << filename << " error";
    return false;
  }

  return Init(executor_def);
}

bool GraphExecutor::Init(const flow::ExecutorDef& executor_def) {
  // 初始化线程池
  uint32_t thread_size = executor_def.thread_size();
  uint32_t queue_size = executor_def.queue_size();
  if (thread_size == 0) {
    thread_size = std::thread::hardware_concurrency();
  }
  if (queue_size == 0) {
    queue_size = thread_size * 8;
  }
  BOOST_LOG_TRIVIAL(info) << "create pool with thread: " << thread_size << " queue: " << queue_size;
  fiber_pool_ = std::make_shared<flow::FiberPool>(thread_size, queue_size);

  // 根据graphDef生成graph
  graphs_.resize(flow::FlowDefine::GraphName_ARRAYSIZE);
  for (const auto& def : executor_def.graph_defs()) {
    if (!GraphCheck(def)) {
      continue;
    }
    if (graphs_[def.name()] != nullptr) {
      continue;
    }
    auto graph = BuildGraph(def);
    if (graph == nullptr) {
      continue;
    }
    graphs_[def.name()] = graph;
  }
  return true;
}

std::unique_ptr<GraphSession> GraphExecutor::BuildGraphSession(uint32_t graph_name) {
  if (graph_name > FlowDefine::GraphName_MAX || graph_name < FlowDefine::GraphName_MIN) {
    BOOST_LOG_TRIVIAL(warning) << "miss graph " << graph_name;
    return nullptr;
  }
  // 找到graph
  auto graph = graphs_[graph_name];
  if (graph == nullptr) {
    return nullptr;
  }
  // 创建session
  auto session = std::make_unique<GraphSession>(graph, fiber_pool_);
  return session;
}

std::shared_ptr<Graph> GraphExecutor::BuildGraph(const flow::GraphDef& graph_def) {
  auto graph = std::make_shared<Graph>();

  std::set<std::pair<int32_t , int32_t>> from_set;
  std::set<std::pair<int32_t , int32_t>> to_set;
  std::vector<std::vector<int32_t>> from_to(flow::FlowDefine::NodeName_ARRAYSIZE);
  std::vector<std::vector<int32_t>> to_from(flow::FlowDefine::NodeName_ARRAYSIZE);
  for (const auto& edge : graph_def.edges()) {
    from_to[edge.from()].push_back(edge.to());
    to_from[edge.to()].push_back(edge.from());
  }

  const auto& op_map = OperatorCollector::Instance().op_map;
  for (const auto& node_def : graph_def.nodes()) {
    auto node = std::make_unique<Node>();
    node->name_ = node_def.name();
    auto op_it = op_map.find(node_def.op_name());
    if (op_it == op_map.end()) {
      // 缺少op，直接退出
      return nullptr;
    }
    node->creator_ = op_it->second;
    // 前部节点
    node->dependents_ = to_from[node->name_];
    // 后部节点
    node->successors_ = from_to[node->name_];
    node->attr_ = std::make_shared<OpAttr>(node_def.op_attr());
    graph->node_list_.push_back(std::move(node));
  }
  return graph;
}
}
