#include "framework/flow_op.h"
#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/fiber/all.hpp>
#include "protos/graph.pb.h"

namespace flow {

#define DEFINE_OP(Name) \
class Name : public Operator { \
 public:                \
  Name(const OpAttrPtr& attr) : Operator(attr), attr_(attr) { } \
  bool Compute(const FlowContextPtr& data) override { \
    BOOST_LOG_TRIVIAL(info) << "run " << #Name << " attr: " << attr_->ShortDebugString()  ; \
    boost::this_fiber::sleep_for(std::chrono::seconds(1));                  \
    return true; \
  }                     \
  private:              \
  OpAttrPtr attr_;   \
  };                       \
OP_REGISTER(#Name, Name)

DEFINE_OP(Op1);
DEFINE_OP(Op2);
DEFINE_OP(Op3);
DEFINE_OP(Op4);
DEFINE_OP(Op5);
DEFINE_OP(Op6);
DEFINE_OP(Op7);
DEFINE_OP(Op8);
DEFINE_OP(Op9);
DEFINE_OP(Op10);
DEFINE_OP(Op11);
DEFINE_OP(Op12);
DEFINE_OP(Op13);
DEFINE_OP(Op14);
DEFINE_OP(Op15);
DEFINE_OP(Op16);
DEFINE_OP(Op17);
DEFINE_OP(Op18);
}
