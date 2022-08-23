#pragma once

#include "protos/material_service.grpc.pb.h"

class MaterialServiceImpl : public protos::MaterialService::Service {
public:
  grpc::Status Find(::grpc::ServerContext *context,
                    const ::protos::FindRequest *request,
                    ::protos::FindResponse *response) override;
  grpc::Status Table(::grpc::ServerContext *context,
                     const ::protos::TableRequest *request,
                     ::protos::TableResponse *response) override;
  grpc::Status Status(::grpc::ServerContext *context,
                      const ::protos::StatusRequest *request,
                      ::protos::StatusResponse *response) override;
};


