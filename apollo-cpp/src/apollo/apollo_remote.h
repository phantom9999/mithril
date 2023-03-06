#pragma once

#include "apollo_source.h"

#include <memory>
#include <unordered_map>

namespace brpc {
class Channel;
}

namespace apollo {

class ApolloRemote : public ApolloSource {
 public:
  bool Init() override;
  void Close() override;
  bool GetConfig(const std::vector<ApolloTask> &tasks, const UpdateCallback &callback) override;

 private:
  std::shared_ptr<brpc::Channel> channel_;
  std::string hostname_;
  std::unordered_map<std::string, std::string> token_map{};
};
}
