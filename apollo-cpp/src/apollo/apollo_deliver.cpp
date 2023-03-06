#include "apollo_deliver.h"

#include <brpc/channel.h>
#include <brpc/controller.h>

#include "apollo_flags.h"
#include "apollo.pb.h"

namespace apollo {

bool ApolloDeliver::GetConfig(const std::vector<std::pair<std::string, bool>> &tasks, const UpdateCallback &callback) {
  // 请求本集群，然后进行填充
  DeliverRequest deliver_request;
  DeliverResponse deliver_response;
  deliver_request.set_server(FLAGS_apollo_server);
  deliver_request.set_app(FLAGS_apollo_app);
  deliver_request.set_cluster(FLAGS_apollo_cluster);
  deliver_request.set_token(FLAGS_apollo_tokens);
  ApolloDeliverService_Stub stub(channel_.get());
  brpc::Controller controller;
  stub.GetConfig(&controller, &deliver_request, &deliver_response, nullptr);
  if (controller.Failed()) {
    LOG(WARNING) << "call deliver server " << FLAGS_apollo_deliver_server << " error: " << controller.ErrorText();
    return false;
  }
  std::unordered_map<std::string, const ApiResponse*> datamap;
  for (const auto& item : deliver_response.data()) {
    datamap[item.namespacename()] = &item;
  }

  return std::all_of(tasks.begin(), tasks.end(), [&](const ApolloTask& task){
    auto it = datamap.find(task.first);
    if (it == datamap.end()) {
      LOG(WARNING) << "namespace [" << task.first << "] not found in deliver data";
      return false;
    }
    auto& res = it->second;
    if (!callback(task.first, task.second, *res)) {
      LOG(WARNING) << "namespace [" << task.first << "] callback error";
      return false;
    }
    return true;
  });
}

bool ApolloDeliver::Init() {
  channel_ = std::make_shared<brpc::Channel>();
  brpc::ChannelOptions options;
  options.timeout_ms = 2000;
  if (channel_->Init(FLAGS_apollo_deliver_server.c_str(), &options) != 0) {
    LOG(WARNING) << "deliver init " << FLAGS_apollo_deliver_server << " error";
    return false;
  }
  return true;
}
void ApolloDeliver::Close() {
  channel_.reset();
}
}
