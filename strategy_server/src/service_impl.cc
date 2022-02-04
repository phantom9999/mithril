#include "service_impl.h"
#include "session.h"
#include "strategy_base.h"
#include "strategies/strategy_a.h"
#include "strategies/strategy_b.h"
#include "strategies/strategy_c.h"
#include "strategies/strategy_d.h"


::grpc::Status ServiceImpl::Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                    ::StrategyResponse *response) {
  Session session;
  session.set(SessionType::REQUEST, request);
  session.set(SessionType::RESPONSE, response);
  response->set_result(std::to_string(request->logid()));
  executor_.run(taskflow_).wait();
  std::cout << std::endl;
  return grpc::Status::OK;
}

ServiceImpl::ServiceImpl() {
  auto [A, B, C, D] = taskflow_.emplace(  // create four tasks
      createStrategy<StrategyA>(),
      createStrategy<StrategyB>(),
      createStrategy<StrategyC>(),
      createStrategy<StrategyD>()
  );

  A.precede(B, C);  // A runs before B and C
  D.succeed(B, C);  // D runs after  B and C
}
