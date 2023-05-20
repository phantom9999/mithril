#pragma once

#include <memory>

#include "proto/service.grpc.pb.h"

namespace ad {
class ResourceManager;

class FeatureServiceImpl : public proto::FeatureService::Service {
 public:
  explicit FeatureServiceImpl(const std::shared_ptr<ResourceManager>& resource_manager);
  ~FeatureServiceImpl() override = default;
  grpc::Status GetFeature(::grpc::ServerContext *context,
                          const ::proto::FeatureRequest *request,
                          ::proto::FeatureResponse *response) override;

 private:
  const std::shared_ptr<ResourceManager> resource_manager_;
};



}


