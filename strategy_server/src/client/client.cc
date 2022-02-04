#include "service.grpc.pb.h"
#include <grpc++/grpc++.h>


int main() {
  StrategyRequest request;
  srand(time(0));
  request.set_logid(rand());
  StrategyResponse response;
  grpc::ClientContext context;
  auto stub = StrategyService::NewStub(
      grpc::CreateChannel("127.0.0.1:8000",grpc::InsecureChannelCredentials()));

  grpc::Status status = stub->Rank(&context, request, &response);
  if (status.ok()) {
    std::cout << response.result() << std::endl;
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
  }
}








