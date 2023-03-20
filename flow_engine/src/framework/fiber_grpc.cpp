#include "fiber_grpc.h"
#include <grpcpp/grpcpp.h>
#include <boost/log/trivial.hpp>

namespace flow {

GrpcClient::GrpcClient() {
  completion_queue_ = std::make_unique<grpc::CompletionQueue>();
  worker_ = std::thread(&GrpcClient::Poll, completion_queue_.get());
}

GrpcClient::~GrpcClient() {
  completion_queue_->Shutdown();
  worker_.join();
}

void GrpcClient::Register(const std::string& name, std::unique_ptr<PackedCall> rpc_call) {
  rpc_calls_[name] = std::move(rpc_call);
}

boost::fibers::future<RpcResult> GrpcClient::Call(const std::string& name, const google::protobuf::Message& request) {
  auto it = rpc_calls_.find(name);
  if (it == rpc_calls_.end() || it->second == nullptr) {
    boost::fibers::promise<RpcResult> promise;
    auto fu = promise.get_future();
    promise.set_exception(std::make_exception_ptr(std::runtime_error("miss " + name)));
    return fu;
  }
  return it->second->Call(request, completion_queue_.get());
}


void GrpcClient::Poll(grpc::CompletionQueue* cq) {
  AsyncCall* call = nullptr;
  bool ok = false;
  while (true) {
    auto flag = cq->Next((void**)&call, &ok);
    if (!flag || !ok) {
      break;
    }
    RpcResult rpc_result;
    rpc_result.status = call->status;
    rpc_result.response = std::move(call->response);
    call->promise.set_value(std::move(rpc_result));
    delete call;
  }
}


void GrpcCollector::Add(const std::string &server, const std::string &method, std::unique_ptr<PackedCall> call) {
  if (call != nullptr) {
    data_[std::make_pair(server, method)] = std::move(call);
  }
}

GrpcRegister::GrpcRegister(const std::string& server, const std::string& method, std::unique_ptr<PackedCall> call) {
  GrpcCollector::Get().Add(server, method, std::move(call));
}

bool GrpcClient::Init(const std::string& filename) {
  return true;
}

bool GrpcClient::Init(const flow::GrpcClientConfig& config) {
  std::unordered_map<std::string, const ChannelConfig*> config_map;
  for (const auto& sub_config : config.channel_configs()) {
    channels_[sub_config.name()] = grpc::CreateChannel(sub_config.address(), grpc::InsecureChannelCredentials());
    config_map[sub_config.name()] = &sub_config;
  }
  for (auto& entry : GrpcCollector::Get().data_) {
    auto& channel_name = entry.first.first;
    auto& method_name = entry.first.second;
    auto& rpc = entry.second;
    auto it = config_map.find(channel_name);
    if (it == config_map.end()) {
      BOOST_LOG_TRIVIAL(warning) << "miss config for server: " << channel_name;
      return false;
    }
    auto channel = channels_[channel_name];
    auto& method_map = it->second->method_timeout();
    auto sub_it = method_map.find(method_name);
    if (sub_it == method_map.end()) {
      // 使用全局的
      rpc->Init(channel, it->second->timeout());
    } else {
      // 使用method的
      rpc->Init(channel, sub_it->second);
    }
    rpc_calls_[method_name] = std::move(rpc);
  }
  return true;
}



}

