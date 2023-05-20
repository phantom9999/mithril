#pragma once

#include <memory>

#include "proto/service.grpc.pb.h"


namespace sw::redis {
class AsyncRedisCluster;
}

namespace ad {

using RedisClientPtr = std::shared_ptr<sw::redis::AsyncRedisCluster>;

class FeatureServiceImpl : public proto::FeatureService::Service {
 public:
  explicit FeatureServiceImpl(const RedisClientPtr& redis_client_ptr);
  ~FeatureServiceImpl() override;
  grpc::Status GetFeature(::grpc::ServerContext *context,
                          const ::proto::FeatureRequest *request,
                          ::proto::FeatureResponse *response) override;

 private:
  std::shared_ptr<sw::redis::AsyncRedisCluster> client_;
};



}


