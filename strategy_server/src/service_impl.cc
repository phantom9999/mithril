#include "service_impl.h"
#include "session.h"
#include "strategy_base.h"
#include "strategies/strategy_a.h"
#include "strategies/strategy_b.h"
#include "strategies/strategy_c.h"
#include "strategies/strategy_d.h"
#include <unordered_map>

::grpc::Status ServiceImpl::Rank(::grpc::ServerContext *context, const ::StrategyRequest *request,
                    ::StrategyResponse *response) {
  tf::Taskflow taskflow;
  auto session = std::make_shared<Session>();
  {
    std::unordered_map<std::string, tf::Task> taskmap;
    taskmap.insert({"StrategyA", taskflow.emplace(createStrategy<StrategyA>(session))});
    taskmap.insert({"StrategyB", taskflow.emplace(createStrategy<StrategyB>(session))});
    taskmap.insert({"StrategyC", taskflow.emplace(createStrategy<StrategyC>(session))});
    taskmap.insert({"StrategyD", taskflow.emplace(createStrategy<StrategyD>(session))});
    taskmap["StrategyA"].precede(taskmap["StrategyB"]);
    taskmap["StrategyA"].precede(taskmap["StrategyC"]);
    taskmap["StrategyD"].succeed(taskmap["StrategyB"]);
    taskmap["StrategyD"].succeed(taskmap["StrategyC"]);
  }

  session->set(SessionType::REQUEST, request);
  session->set(SessionType::RESPONSE, response);

  executor_.run(taskflow).wait();
  std::cout << std::endl;
  return grpc::Status::OK;
}

ServiceImpl::ServiceImpl() {

}
