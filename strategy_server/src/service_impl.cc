#include "service_impl.h"
#include "session.h"


::grpc::Status ServiceImpl::Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                    ::StrategyResponse *response) {
  Session session;
  session.set(SessionType::REQUEST, request);
  session.set(SessionType::RESPONSE, response);
  response->set_result(std::to_string(request->logid()));
  executor_.run(taskflow_).wait();
  return grpc::Status::OK;
}
ServiceImpl::ServiceImpl() {
  auto [A, B, C, D] = taskflow_.emplace(  // create four tasks
      [] () { std::cout << "TaskA\n"; },
      [] () { std::cout << "TaskB\n"; },
      [] () { std::cout << "TaskC\n"; },
      [] () { std::cout << "TaskD\n"; }
  );

  A.precede(B, C);  // A runs before B and C
  D.succeed(B, C);  // D runs after  B and C
}

