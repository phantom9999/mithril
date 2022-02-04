#include "strategy_b.h"
#include <iostream>
#include <sstream>
#include "session.h"
#include "context_list.h"

void StrategyB::run() {
  auto data = session_->get<ContextAPtr>(SessionType::A);
  std::stringstream ss;
  ss << "strategy B , get logid: " << data->logid << "\n";
  std::cout << ss.str();
  auto tmp = std::make_shared<ContextB>();
  tmp->logid = data->logid + 2;
  session_->set(SessionType::B, tmp);
}
