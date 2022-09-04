#pragma once

#include "service.grpc.pb.h"

class IndexManager;

class ServiceImpl : public proto::IndexService::Service {
 public:
  ServiceImpl(IndexManager* index_manager);

  ~ServiceImpl() override = default;

  grpc::Status Retrieval(::grpc::ServerContext *context,
                         const ::proto::RetrievalRequest *request,
                         ::proto::RetrievalResponse *response) override;
  grpc::Status Status(::grpc::ServerContext *context,
                      const ::proto::StatusRequest *request,
                      ::proto::StatusResponse *response) override;
 private:
  IndexManager* index_manager_{nullptr};
};


