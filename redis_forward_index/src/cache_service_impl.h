#pragma once

#include "service.grpc.pb.h"

class CacheServiceImpl : public meta::CacheService::Service {
 public:
  grpc::Status Get(::grpc::ServerContext *context, const ::meta::Request *request, ::meta::Response *response) override;
};

