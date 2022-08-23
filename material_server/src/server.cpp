#include <gflags/gflags.h>
#include <glog/logging.h>
#include <absl/cleanup/cleanup.h>
#include <grpc++/grpc++.h>
#include "service/material_service_impl.h"

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  auto cleanup = absl::MakeCleanup([](){
    google::ShutdownGoogleLogging();
    gflags::ShutDownCommandLineFlags();
  });

  std::string server_address("0.0.0.0:8000");
  MaterialServiceImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

