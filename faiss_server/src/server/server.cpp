
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpc++/server_builder.h>

#include "server_config.pb.h"

#include "server/service_impl.h"
#include "server/server_flags.h"
#include "server/index_manager.h"
#include "common/config_loader.h"

int main(int argc, char** argv) {
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
  // google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  std::string address = "0.0.0.0:" + std::to_string(FLAGS_port);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());

  proto::ServerConfig server_config;
  if (!ConfigLoader::LoadPb(FLAGS_server_config, &server_config)) {
    LOG(WARNING) << "load config error";
    return 0;
  }

  IndexManager index_manager;
  if (!index_manager.Init(server_config.index_config())) {
    LOG(WARNING) << "index manager init error for dir: " << FLAGS_index_path;
    return 0;
  }

  ServiceImpl service{&index_manager};

  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  LOG(INFO) << "start at " << address;

  server->Wait();
  index_manager.Stop();
  return 0;
}



