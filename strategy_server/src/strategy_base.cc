#include "strategy_base.h"

void StrategyBase::bindSession(const SessionPtr &session) {
  this->session_ = session;
}
