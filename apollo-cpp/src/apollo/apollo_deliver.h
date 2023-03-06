#pragma once

#include <memory>
#include <vector>
#include <utility>
#include "apollo_source.h"

namespace brpc {
class Channel;
}

namespace apollo {
// 从其他集群获取
class ApolloDeliver : public ApolloSource {
 public:
  bool GetConfig(const std::vector<std::pair<std::string, bool>>& tasks, const UpdateCallback& callback) override;
  bool Init() override;
  void Close() override;
 private:
  std::shared_ptr<brpc::Channel> channel_;
};
}
