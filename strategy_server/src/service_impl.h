#pragma once

#include <taskflow/taskflow.hpp>
#include "service.grpc.pb.h"


class ServiceImpl final : public StrategyService::Service {
 public:
  ServiceImpl();

  ::grpc::Status Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                      ::StrategyResponse *response) override;

 private:
  tf::Executor executor_{10};
};

