#include "proto/service.grpc.pb.h"
#include <grpc++/grpc++.h>
#include "proto/graph.pb.h"
#include <memory>
#include <any>


void BuildRequest(StrategyRequest* request) {
  request->set_graph("ad_process");
  request->set_logid(std::abs(rand()));
  for (int i=0;i<10;++i) {
    auto ad = request->add_ad_infos();
    ad->set_ad_id(i);
    int32_t ran = std::abs(rand()) % 1000;
    ad->set_bid(ran * 1.0 / 1000 + 0.001);
  }
}


void Call() {
  StrategyRequest request;
  srand(time(0));
  BuildRequest(&request);
  StrategyResponse response;
  grpc::ClientContext context;
  auto stub = StrategyService::NewStub(
      grpc::CreateChannel("127.0.0.1:8000",grpc::InsecureChannelCredentials()));

  grpc::Status status = stub->Rank(&context, request, &response);
  if (status.ok()) {
    std::cout << response.DebugString() << std::endl;
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
  }
}

int main() {
  Call();
}








