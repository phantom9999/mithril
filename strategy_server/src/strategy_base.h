#pragma once

#include <type_traits>
#include <taskflow/taskflow.hpp>

#include <any>
#include <memory>
#include <type_traits>
#include <utility>
#include "taskflow/taskflow.hpp"



class StrategyBase {
 public:
  StrategyBase() = default;
  ~StrategyBase() = default;

 private:
};

class StaticStrategy : public StrategyBase {
 public:
  virtual void run() = 0;
};

class DynamicStrategy : public StrategyBase {
  virtual void run(tf::Subflow& subflow) = 0;
};

template <typename StrategyName,
    typename ... Args,
    typename std::enable_if_t<std::is_base_of_v<StrategyBase, StrategyName>, int> = 0>
auto CreateStrategy(Args&& ... args) {
  if constexpr (std::is_base_of_v<StaticStrategy, StrategyName>) {
    std::shared_ptr<StrategyName> class_name = std::make_shared<StrategyName>(std::forward<Args>(args)...);
    return [class_name]() {
      class_name->Run();
    };
  } else if constexpr (std::is_base_of_v<DynamicStrategy, StrategyName>) {
    std::shared_ptr<StrategyName> class_name = std::make_shared<StrategyName>(std::forward<Args>(args)...);
    return [class_name](tf::Subflow &subflow) {
      class_name->Run(subflow);
    };
  } else {
    return []() {};
  }
}

