#include "apollo_remote.h"

#include <gflags/gflags.h>
#include <brpc/channel.h>
#include <absl/strings/str_format.h>
#include <google/protobuf/util/json_util.h>

#include "apollo_utils.h"
#include "apollo_flags.h"
#include "apollo.pb.h"

namespace apollo {

bool ApolloRemote::GetConfig(const std::vector<ApolloTask> &tasks, const UpdateCallback &callback) {
  using RemoteTask = std::tuple<std::unique_ptr<brpc::Controller>, std::string, bool>;
  std::vector<RemoteTask> remote_task_list;
  for (const auto& item : tasks) {
    auto controller = std::make_unique<brpc::Controller>();
    std::string release_key = token_map[item.first];
    controller->http_request().uri() = absl::StrFormat(
        "%s/configs/%s/%s/%s?releaseKey=%s&ip=%s", FLAGS_apollo_server.c_str(), FLAGS_apollo_app.c_str(),
        FLAGS_apollo_cluster.c_str(), item.first.c_str(), release_key.c_str(), hostname_.c_str());
    channel_->CallMethod(nullptr, controller.get(), nullptr, nullptr, brpc::DoNothing());
    remote_task_list.emplace_back(std::move(controller), item.first, item.second);
  }

  bool all_success = true;
  for (const auto& item : remote_task_list) {
    auto& controller = std::get<0>(item);
    auto& ns = std::get<1>(item);
    auto& is_prop = std::get<2>(item);
    brpc::Join(controller->call_id());
    int http_status = controller->http_response().status_code();
    if (http_status == 304) {
      // 不需要更新
      continue;
    }
    if (http_status != 200) {
      LOG(WARNING) << "url (" << controller->http_request().uri() << ") status: " << http_status;
      // 状态有问题，更新失败
      all_success = false;
      continue;
    }
    apollo::ApiResponse response;
    std::string content = controller->response_attachment().to_string();
    LOG(INFO) << "url(" << controller->http_request().uri() << ") -> content: " << content;
    auto status = google::protobuf::util::JsonStringToMessage(content, &response);
    if (!status.ok()) {
      LOG(WARNING) << "url(" << controller->http_request().uri() << ") content parse error: " << status.message();
      all_success = false;
      continue;
    }
    if (!callback(ns, is_prop, response)) {
      LOG(WARNING) << "namespace [" << ns << "] callback error";
      all_success = false;
      continue;
    }
    token_map[ns] = response.releasekey();
  }
  return all_success;
}
bool ApolloRemote::Init() {
  brpc::ChannelOptions options;
  options.timeout_ms = FLAGS_apollo_timeout;
  options.protocol = brpc::PROTOCOL_HTTP;
  channel_ = std::make_shared<brpc::Channel>();
  if (channel_->Init(FLAGS_apollo_server.c_str(), &options) != 0) {
    LOG(WARNING) << "connect to " << FLAGS_apollo_server << " error";
    return false;
  }
  hostname_ = GetHostname();
  LOG(INFO) << "hostname [" << hostname_ << "]";

  return true;
}
void ApolloRemote::Close() {
  channel_.reset();
}
}
