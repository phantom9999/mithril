#pragma once

#include "service.grpc.pb.h"

class ServiceImpl final : public StrategyService::Service {
 public:
  ::grpc::Status Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                      ::StrategyResponse *response) override;
};

