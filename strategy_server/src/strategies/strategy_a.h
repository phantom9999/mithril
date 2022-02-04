#pragma once

#include "strategy_base.h"

class StrategyA final : public StaticStrategy {
 public:
  void run() override;
};
