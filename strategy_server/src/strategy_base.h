#pragma once

#include <any>
#include <memory>
#include <utility>
#include <type_traits>
#include <taskflow/taskflow.hpp>
#include "session.h"

class StrategyBase {
 public:
  StrategyBase() = default;
  virtual ~StrategyBase() = default;

  void bindSession(const SessionPtr& session);

 protected:
  SessionPtr session_{nullptr};
};

class StaticStrategy : public StrategyBase {
 public:
  virtual void run() = 0;
};

class DynamicStrategy : public StrategyBase {
 public:
  virtual void run(tf::Subflow& subflow) = 0;
};

template <typename StrategyName,
    typename ... Args,
    typename std::enable_if_t<std::is_base_of_v<StrategyBase, StrategyName>, int> = 0>
auto createStrategy(const SessionPtr session, Args&& ... args) {
  std::shared_ptr<StrategyName> class_name = std::make_shared<StrategyName>(std::forward<Args>(args)...);
  class_name->bindSession(session);
  if constexpr (std::is_base_of_v<StaticStrategy, StrategyName>) {
    return [class_name]() {
      class_name->run();
    };
  } else if constexpr (std::is_base_of_v<DynamicStrategy, StrategyName>) {
    return [class_name](tf::Subflow &subflow) {
      class_name->run(subflow);
    };
  } else {
    return []() {};
  }
}

