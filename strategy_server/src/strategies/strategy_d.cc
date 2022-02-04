#include "strategy_d.h"
#include "session.h"
#include "context_list.h"
#include "service.pb.h"

void StrategyD::run() {
  auto b = session_->get<ContextBPtr>(SessionType::B);
  auto c = session_->get<ContextCPtr>(SessionType::C);
  std::stringstream ss;
  ss << "strategy C , get logid: " << b->logid << " and " << c->logid << "\n";
  std::cout << ss.str();
  auto response = session_->get<StrategyResponse*>(SessionType::RESPONSE);
  response->set_result(std::to_string(b->logid + c->logid));
}
