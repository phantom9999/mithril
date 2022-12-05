#include "fiber_grpc.h"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>

namespace fiber {

GrpcClient::GrpcClient(const ClientOption &option) {
  timespec_.tv_sec = option.timeout / 1000;
  timespec_.tv_nsec = (option.timeout % 1000) * 1000 * 1000;
  timespec_.clock_type = GPR_TIMESPAN;

  {
    grpc::ChannelArguments channel_arguments;
    channel_ = grpc::CreateCustomChannel(option.address, grpc::InsecureChannelCredentials(), channel_arguments);
  }
  completion_queue_ = std::make_unique<grpc::CompletionQueue>();
  worker_ = std::thread([this] { return Work(); });
}

GrpcClient::~GrpcClient() {
  completion_queue_->Shutdown();
  worker_.join();
  channel_.reset();
}

void GrpcClient::Work() {
  CallData* data = nullptr;
  bool ok = false;
  while (completion_queue_->Next((void**)&data, &ok)) {
    if (!ok) {
      break;
    }
    if (data->status.ok()) {
      data->promise.set_value(std::move(data->response));
    } else {
      data->promise.set_exception(std::make_exception_ptr(
          std::runtime_error(data->status.error_message())));
    }
    delete data;
    data = nullptr;
  }
}

Future<meta::HelloResponse> GrpcClient::Call(const meta::HelloRequest& request) {
  auto data = new CallData;
  data->context.set_deadline(timespec_);
  meta::HelloService::Stub stub(channel_);
  auto future = data->promise.get_future();
  auto rpc = stub.Asynchello(&data->context, request, completion_queue_.get());
  rpc->Finish(&data->response, &data->status, reinterpret_cast<void*>(data));
  return future;
}
}
