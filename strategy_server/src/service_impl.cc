#include "service_impl.h"
#include "session.h"
#include "strategy_base.h"
#include "strategies/strategy_a.h"
#include "strategies/strategy_b.h"
#include "strategies/strategy_c.h"
#include "strategies/strategy_d.h"

::grpc::Status ServiceImpl::Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                    ::StrategyResponse *response) {
  tf::Taskflow taskflow;
  auto session = std::make_shared<Session>();
  {
    auto [A, B, C, D] = taskflow.emplace(  // create four tasks
        createStrategy<StrategyA>(session),
        createStrategy<StrategyB>(session),
        createStrategy<StrategyC>(session),
        createStrategy<StrategyD>(session)
    );

    A.precede(B, C);  // A runs before B and C
    D.succeed(B, C);  // D runs after  B and C
  }

  session->set(SessionType::REQUEST, request);
  session->set(SessionType::RESPONSE, response);

  executor_.run(taskflow).wait();
  std::cout << std::endl;
  return grpc::Status::OK;
}

ServiceImpl::ServiceImpl() {

}
