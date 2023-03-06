#pragma once

#include <atomic>
#include <string>
#include <vector>
#include <utility>
#include "apollo_source.h"

namespace apollo {

class ApolloSink : public ApolloSource {
 public:
  bool Init() override;
  void Close() override;
  void KeepALive();
  bool GetConfig(const std::vector<std::pair<std::string, bool>>& tasks, const UpdateCallback& callback) override;
  void SinkConfig(const std::string& ns, const apollo::ApiResponse& response);

 private:
  bool avail_{false};
  std::atomic_uint32_t version_{0};
  std::string alive_file_{};
};
}
