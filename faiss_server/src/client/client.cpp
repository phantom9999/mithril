#include <iostream>
#include <sstream>
#include <random>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>

#include "service.grpc.pb.h"
#include "index_constants.pb.h"

DEFINE_string(server, "localhost:8080", "address");
DEFINE_string(method, "recall", "method of rpc");
DEFINE_uint32(dim, 128, "dim");
DEFINE_uint32(query_size, 2, "size");
DEFINE_uint32(topk, 10, "topk");


void BuildRequest(proto::RetrievalRequest* request, uint32_t size, uint32_t dim, uint32_t topk) {
  std::mt19937 rng{std::random_device{}()};
  std::uniform_real_distribution<> distrib;

  request->set_topk(topk);
  request->set_query_size(size);
  uint32_t total = size * dim;
  for (int i=0; i<total; ++i) {
    request->add_query_vec(distrib(rng));
  }

  request->set_model_name(proto::Constants::DSSM_V2);
}

void Recall(const std::unique_ptr<proto::IndexService::Stub>& stub) {
  proto::RetrievalRequest request;
  proto::RetrievalResponse response;
  BuildRequest(&request, FLAGS_query_size, FLAGS_dim, FLAGS_topk);

  grpc::ClientContext context;
  auto status = stub->Retrieval(&context, request, &response);
  if (status.ok()) {
    LOG(INFO) << response.DebugString();
  } else {
    LOG(INFO) << status.error_message();
  }
}

void Status(const std::unique_ptr<proto::IndexService::Stub>& stub) {
  proto::StatusRequest request;
  proto::StatusResponse response;
  grpc::ClientContext context;
  auto status = stub->Status(&context, request, &response);
  if (status.ok()) {
    LOG(INFO) << response.DebugString();
  } else {
    LOG(INFO) << status.error_message();
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto channel = grpc::CreateChannel(FLAGS_server, grpc::InsecureChannelCredentials());
  auto stub = proto::IndexService::NewStub(channel);

  if (FLAGS_method == "recall") {
    Recall(stub);
  } else if (FLAGS_method == "status") {
    Status(stub);
  }

  return 0;
}
