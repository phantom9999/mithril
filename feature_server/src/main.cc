#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <sw/redis++/async_redis++.h>

#include "feature_service_impl.h"



int main(int argc, char** argv) {
  std::string server_address("0.0.0.0:8080");

  std::shared_ptr<sw::redis::AsyncRedisCluster> redis_cluster;
  {
    using namespace sw::redis;
    ConnectionOptions opts;
    opts.host = "localhost";
    opts.port = 6379;

    sw::redis::ConnectionPoolOptions pool_opts;
    pool_opts.size = 3;
    redis_cluster = std::make_shared<sw::redis::AsyncRedisCluster>(opts, pool_opts);
  }

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  ad::FeatureServiceImpl feature_service(redis_cluster);
  builder.RegisterService(&feature_service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
  return 0;
}
