#pragma once

#include <boost/fiber/all.hpp>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>
#include <boost/noncopyable.hpp>
#include "protos/rpc_config.pb.h"

namespace flow {

struct RpcResult {
  grpc::Status status{};
  std::unique_ptr<google::protobuf::Message> response{};
};

struct AsyncCall {
  grpc::ClientContext context;
  grpc::Status status;
  std::unique_ptr<google::protobuf::Message> response;
  boost::fibers::promise<RpcResult> promise;
};


class PackedCall {
 public:
  virtual ~PackedCall() = default;
  virtual void Init(const std::shared_ptr<grpc::Channel>& channel, int64_t timeout) = 0;
  virtual boost::fibers::future<RpcResult> Call(const google::protobuf::Message& request, grpc::CompletionQueue*) = 0;
};

template<typename ServiceType, typename RequestType, typename ResponseType>
class GrpcCall : public PackedCall {
  using MethodType = std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>>(ServiceType::Stub::*)(grpc::ClientContext*, const RequestType&, grpc::CompletionQueue*);
 public:
  explicit GrpcCall(MethodType method) : method_(method) { }

  void Init(const std::shared_ptr<grpc::Channel> &channel, int64_t timeout) override {
    timespec_.tv_sec = timeout / 1000;
    timespec_.tv_nsec = timeout * 1000 * 1000;
    timespec_.clock_type = GPR_TIMESPAN;
    stub_ = ServiceType::NewStub(channel);
  }

  boost::fibers::future<RpcResult> Call(const google::protobuf::Message &request, grpc::CompletionQueue *completion_queue) override {
    auto call = new AsyncCall;
    call->context.set_deadline(timespec_);
    call->response = std::make_unique<ResponseType>();
    auto future = call->promise.get_future();
    auto t_request = dynamic_cast<const RequestType*>(&request);
    auto t_response = dynamic_cast<ResponseType*>(call->response.get());
    if (t_request == nullptr || t_response == nullptr || stub_ == nullptr) {
      call->promise.set_exception(std::make_exception_ptr(std::runtime_error("message error")));
      delete call;
      return future;
    }

    auto rpc = (*stub_.*method_)(&call->context, *t_request, completion_queue);
    rpc->Finish(t_response, &call->status, reinterpret_cast<void*>(call));
    return future;
  }

 private:
  MethodType method_;
  std::unique_ptr<typename ServiceType::Stub> stub_;
  gpr_timespec timespec_{};
};

class GrpcCollector : public boost::noncopyable {
 public:
  static GrpcCollector& Get() {
    static GrpcCollector grpc_collector;
    return grpc_collector;
  }

  void Add(const std::string& server, const std::string& method, std::unique_ptr<PackedCall> call);

 private:
  GrpcCollector() = default;
  std::map<std::pair<std::string, std::string>, std::unique_ptr<PackedCall>> data_{};
  friend class GrpcClient;
};


struct GrpcRegister {
  GrpcRegister(const std::string& server, const std::string& method, std::unique_ptr<PackedCall> call);
};

#define REGISTER_METHOD(GService, ReqMsg, ResMsg, AsyncMethod, method, server) \
static ::flow::GrpcRegister register_ ##method{ \
server, #method, std::make_unique<::flow::GrpcCall<GService, ReqMsg, ResMsg>>(&GraphService::Stub::AsyncMethod) \
}

class GrpcClient {
 public:
  GrpcClient();
  ~GrpcClient();

  bool Init(const std::string& filename);
  bool Init(const flow::GrpcClientConfig& config);

  void Register(const std::string& name, std::unique_ptr<PackedCall> rpc_call);

  boost::fibers::future<RpcResult> Call(const std::string& name, const google::protobuf::Message& request);

 private:
  static void Poll(grpc::CompletionQueue* cq);

 private:
  std::thread worker_;
  std::unique_ptr<grpc::CompletionQueue> completion_queue_;
  std::unordered_map<std::string, std::unique_ptr<PackedCall>> rpc_calls_;
  std::unordered_map<std::string, std::shared_ptr<grpc::Channel>> channels_;
};



}


