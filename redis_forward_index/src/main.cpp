#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <absl/cleanup/cleanup.h>

#include "cache_service_impl.h"

DEFINE_string(address, "0.0.0.0:8080", "");

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  auto cleaner = absl::MakeCleanup([](){
    google::ShutdownGoogleLogging();
    google::ShutDownCommandLineFlags();
  });

  CacheServiceImpl cache_service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;
  builder.AddListeningPort(FLAGS_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&cache_service);
  auto server = builder.BuildAndStart();
  LOG(INFO) << "Server start on " << FLAGS_address;
  google::FlushLogFiles(0);
  server->Wait();
  return 0;
}


