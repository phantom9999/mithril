#pragma once

#include "strategy_base.h"

class StrategyC final : public StaticStrategy {
 public:
  void run() override;
};
