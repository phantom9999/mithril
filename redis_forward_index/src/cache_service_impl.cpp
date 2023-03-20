#include "cache_service_impl.h"

grpc::Status CacheServiceImpl::Get(::grpc::ServerContext *context,
                                   const ::meta::Request *request,
                                   ::meta::Response *response) {
  return Service::Get(context, request, response);
}
