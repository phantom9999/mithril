#include "service_impl.h"
#include "session.h"

::grpc::Status ServiceImpl::Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                    ::StrategyResponse *response) {
  Session session;
  session.set(SessionType::REQUEST, request);
  session.set(SessionType::RESPONSE, response);
  response->set_result(std::to_string(request->logid()));
  return grpc::Status::OK;
}

