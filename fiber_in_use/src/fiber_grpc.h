#pragma once

#include <thread>
#include <string>
#include <memory>

#include <grpcpp/completion_queue.h>

#include "service.grpc.pb.h"
#include "fiber_define.h"

namespace fiber {

struct ClientOption {
  std::string address;
  uint32_t timeout;
};

struct CallData {
  grpc::ClientContext context;
  Promise<meta::HelloResponse> promise;
  grpc::Status status;
  meta::HelloResponse response;
};

class GrpcClient {
 public:
  explicit GrpcClient(const ClientOption& option);
  ~GrpcClient();

  Future<meta::HelloResponse> Call(const meta::HelloRequest& request);

 private:
  void Work();

 private:
  std::unique_ptr<grpc::CompletionQueue> completion_queue_;
  std::thread worker_;
  std::shared_ptr<grpc::Channel> channel_;
  gpr_timespec timespec_{};
};
}
