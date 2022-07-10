#include <memory>
#include <any>
#include <utility>
#include <vector>
#include <string>

#include <grpc++/grpc++.h>
#include "proto/service.pb.h"
#include "proto/graph.pb.h"
#include "proto/service.grpc.pb.h"
#include "framework/processor.h"


class StrategyServiceImpl : public StrategyService::Service {
public:
  explicit StrategyServiceImpl(std::shared_ptr<Processor>  processor) : processor_(std::move(processor)) { }
  ::grpc::Status Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                      ::StrategyResponse *response) override {
    if (processor_->Run(request, response)) {
      return grpc::Status::OK;
    }
    return grpc::Status::CANCELLED;
  }

private:
  std::shared_ptr<Processor> processor_;
};


int main() {
  GraphsConf graphs_conf;
  auto processor = std::make_shared<Processor>(graphs_conf);
  std::string server_address("0.0.0.0:8000");
  StrategyServiceImpl service{processor};

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}



