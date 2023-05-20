#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <glog/logging.h>
#include <absl/cleanup/cleanup.h>

#include "feature_service_impl.h"
#include "resource_manager.h"


int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  auto cleanup = absl::MakeCleanup([](){
    google::ShutdownGoogleLogging();
    google::ShutDownCommandLineFlags();
  });

  std::string server_address("0.0.0.0:8080");

  auto resource_manager = std::make_shared<ad::ResourceManager>();
  if (!resource_manager->Init()) {
    LOG(ERROR) << "resource init error";
    return -1;
  }

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  ad::FeatureServiceImpl feature_service(resource_manager);
  builder.RegisterService(&feature_service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
  return 0;
}
