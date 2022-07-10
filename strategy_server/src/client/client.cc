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

template<typename Type>
std::shared_ptr<Type> AnyCast(Session_Type index, std::vector<std::shared_ptr<std::any>>& data_) {
  auto data = data_[index];
  if (data == nullptr) {
    return nullptr;
  }
  try {
    return std::any_cast<std::shared_ptr<Type>>(*data);
  } catch (std::bad_any_cast const & e) {
    std::cout << Session_Type_Name(index) << std::boolalpha << "; has " << data->has_value() << "; but " << e.what();
    return nullptr;
  }
}


int main() {
  /*std::vector<std::shared_ptr<std::any>> data_list(10);
  {
    auto request = std::make_shared<StrategyRequest>();
    request->set_logid(1234);
    auto data = std::make_shared<std::any>(request);
    std::cout << data->type().name() << std::endl;
    data_list[Session::REQUEST1] = data;
  }
  {
    auto new_request = AnyCast<StrategyRequest>(Session::REQUEST1, data_list);
    std::cout << std::boolalpha << (new_request == nullptr) << " " << new_request->logid();
  }*/

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








