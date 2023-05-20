#include <vector>
#include <thread>
#include <boost/type_traits.hpp>
#include <boost/function_types/function_type.hpp>

#include "framework/flow_graph.h"
#include "protos/graph.pb.h"
#include "framework/flow_executor.h"

struct TT {
  int a = 1;
};

void ContextTest() {
  std::string filename = "../config/graph.txt";
  auto graph_executor = std::make_shared<flow::GraphExecutor>();
  if (!graph_executor->Init(filename)) {
    return;
  }
  auto demo_session = graph_executor->BuildGraphSession(flow::FlowDefine::GRAPH_2);
  auto context = demo_session->GetContext();
  TT t1;
  auto t2 = std::make_shared<TT>();
  const TT* t3 = &t1;
  TT* const t4 = &t1;
  {
    context->Put(0, &t1);
    auto d1 = context->Get<TT*>(0);
    std::cout << d1->a << std::endl;

    context->Put(1, t2);
    auto d2 = context->Get<std::shared_ptr<TT>>(1);
    std::cout << d2->a << std::endl;

    context->Put(2, t3);
    auto d3 = context->Get<const TT*>(2);
    std::cout << d3->a << std::endl;

    context->Put(3, t4);
    auto d4 = context->Get<TT*>(3);
    std::cout << d4->a << std::endl;
  }
}

void GraphTest() {
  std::string filename = "../config/graph.txt";
  auto graph_executor = std::make_shared<flow::GraphExecutor>();
  if (!graph_executor->Init(filename)) {
    return;
  }
  std::vector<std::thread> workers;
  for (int i=0;i<1;++i) {
    workers.emplace_back([&](){
      for (int j=0;j<1;++j) {
        auto session = graph_executor->BuildGraphSession(flow::FlowDefine::GRAPH_2);
        session->Run();
      }
    });
  }
  for (auto&& work : workers) {
    work.join();
  }
}

int main() {

}


