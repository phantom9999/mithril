#include "apollo_deliver_service.h"
#include <brpc/controller.h>
#include "apollo_flags.h"
#include "apollo_global.h"
#include <google/protobuf/util/json_util.h>

namespace apollo {

void ApolloDeliverServiceImpl::GetConfig(::google::protobuf::RpcController *controller,
                                         const ::apollo::DeliverRequest *request,
                                         ::apollo::DeliverResponse *response,
                                         ::google::protobuf::Closure *done) {
  brpc::ClosureGuard done_guard(done);
  auto* cntl = dynamic_cast<brpc::Controller*>(controller);
  if (request->server() != FLAGS_apollo_server
    || request->app() != FLAGS_apollo_app
    || request->cluster() != FLAGS_apollo_cluster
    || request->token() != FLAGS_apollo_tokens) {
    cntl->SetFailed("params error");
    return;
  }
  for (const auto& entry : NamespaceSet::process_map) {
    auto* sub_data = response->add_data();
    auto* config = sub_data->mutable_configurations();
    for (const auto& sub_entry : entry.second) {
      auto& processor = sub_entry.second;
      if (processor == nullptr) {
        continue;
      }
      auto data = processor->Get();
      if (data == nullptr) {
        continue;
      }
      std::string json;
      auto status = google::protobuf::util::MessageToJsonString(*data, &json);
      if (!status.ok()) {
        LOG(WARNING) << data->GetTypeName() << " to json error: " << status.message();
        continue;
      }
      config->insert({sub_entry.first, json});
    }
  }
}
}
