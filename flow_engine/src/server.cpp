#include <iostream>
#include <vector>
#include <thread>
#include <boost/log/trivial.hpp>
#include "framework/flow_executor.h"
#include "graph.pb.h"
#include "service.grpc.pb.h"
#include "framework/flow_context.h"
#include "framework/fiber_grpc.h"

class ServiceImpl : public GraphService::Service {
 public:
  explicit ServiceImpl(const std::shared_ptr<flow::GraphExecutor>& executor): executor_(executor) { }

  grpc::Status Call(::grpc::ServerContext *context, const ::Request *request, ::Response *response) override {
    auto session = executor_->BuildGraphSession(request->graph_name());
    if (session == nullptr) {
      BOOST_LOG_TRIVIAL(warning) << "graph not found";
      return grpc::Status::OK;
    }
    auto data = session->GetContext();
    using flow::FlowDefine;
    data->Put(FlowDefine::REQUEST, request);
    data->Put(FlowDefine::RESPONSE, response);
    session->Run();
    return grpc::Status::OK;
  }
 private:
  std::shared_ptr<flow::GraphExecutor> executor_{nullptr};
};

static ::flow::GrpcRegister register_method {
  "server", "method", std::make_unique<::flow::GrpcCall<::GraphService, ::Request, ::Response>>(&::GraphService::Stub::AsyncCall)
};

REGISTER_METHOD(GraphService, ::Request, ::Response, AsyncCall, method1, "grpc");
REGISTER_METHOD(GraphService, ::Request, ::Response, AsyncCall, method2, "grpc");


int main() {
  std::string filename = "../config/graph.txt";
  auto graph_executor = std::make_shared<flow::GraphExecutor>();
  if (!graph_executor->Init(filename)) {
    return -1;
  }

  ServiceImpl service(graph_executor);
  grpc::ServerBuilder builder;
  builder.AddListeningPort("0.0.0.0:8080", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  BOOST_LOG_TRIVIAL(info) << "start server";
  server->Wait();
  return 0;
}

