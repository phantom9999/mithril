#pragma once

#include "apollo.pb.h"

namespace apollo {
class ApolloDeliverServiceImpl : public ApolloDeliverService {
 public:
  ApolloDeliverServiceImpl() = default;
  ~ApolloDeliverServiceImpl() override = default;
  void GetConfig(::google::protobuf::RpcController *controller,
                 const ::apollo::DeliverRequest *request,
                 ::apollo::DeliverResponse *response,
                 ::google::protobuf::Closure *done) override;
};
}
