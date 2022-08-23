#include "material_service_impl.h"


grpc::Status MaterialServiceImpl::Find(::grpc::ServerContext *context,
                                       const ::protos::FindRequest *request,
                                       ::protos::FindResponse *response) {
  return Service::Find(context, request, response);
}
grpc::Status MaterialServiceImpl::Table(::grpc::ServerContext *context,
                                        const ::protos::TableRequest *request,
                                        ::protos::TableResponse *response) {
  return Service::Table(context, request, response);
}
grpc::Status MaterialServiceImpl::Status(::grpc::ServerContext *context,
                                         const ::protos::StatusRequest *request,
                                         ::protos::StatusResponse *response) {
  return Service::Status(context, request, response);
}
