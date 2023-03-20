#pragma once

#include <unordered_map>
#include <boost/noncopyable.hpp>
#include "flow_context.h"

namespace flow {

class OpAttr;
using OpAttrPtr = std::shared_ptr<const OpAttr>;

class Operator {
 public:
  explicit Operator(const OpAttrPtr& attr) { }
  virtual ~Operator() = default;
  virtual bool Compute(const FlowContextPtr & data) = 0;
};

using Creator = std::function<std::unique_ptr<Operator>(const OpAttrPtr&)>;

class OperatorCollector : public boost::noncopyable {
 public:
  static OperatorCollector& Instance() {
    static OperatorCollector a_register;
    return a_register;
  }

  void Register(const std::string& name, Creator&& creator) {
    op_map[name] = std::move(creator);
  }

  bool Has(const std::string& name) {
    return op_map.find(name) != op_map.end();
  }

 private:
  OperatorCollector() = default;

 private:
  std::unordered_map<std::string, Creator> op_map;

  friend class GraphExecutor;
};

template<typename Type>
struct OperatorRegister {
  static_assert((std::is_base_of<Operator, Type>::value), "Type must be subclass of Operator");
  explicit OperatorRegister(const std::string& name) {
    OperatorCollector::Instance().Register(name, [](const OpAttrPtr& attr) -> std::unique_ptr<Operator>{
      return std::make_unique<Type>(attr);
    });
  }
};

#define OP_REGISTER(name, Op) \
static OperatorRegister<Op> register_ ##Op(name)


}
