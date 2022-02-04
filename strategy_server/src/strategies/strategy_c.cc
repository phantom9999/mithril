#include "strategy_c.h"
#include <iostream>
#include "session.h"
#include "context_list.h"

void StrategyC::run() {
  auto data = session_->get<ContextAPtr>(SessionType::A);
  std::stringstream ss;
  ss << "strategy C , get logid: " << data->logid << "\n";
  std::cout << ss.str();
  auto tmp = std::make_shared<ContextC>();
  tmp->logid = data->logid - 2;
  session_->set(SessionType::C, tmp);
}
