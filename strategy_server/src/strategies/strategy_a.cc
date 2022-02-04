#include "strategy_a.h"
#include <iostream>
#include "service.pb.h"
#include "session.h"
#include "context_list.h"

void StrategyA::run() {
  auto request = session_->get<const StrategyRequest*>(SessionType::REQUEST);
  std::cout << "Strategy A, get logid " << request->logid() << "\n";
  auto data = std::make_shared<ContextA>();
  data->logid = request->logid() / 2;
  session_->set(SessionType::A, data);
}
