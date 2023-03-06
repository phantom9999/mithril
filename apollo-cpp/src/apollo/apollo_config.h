#pragma once

#include <string>
#include <memory>
#include <thread>

#include <boost/noncopyable.hpp>

#include "apollo_source.h"
#include "apollo_sink.h"

namespace brpc {
class Channel;
}

namespace apollo {

class ApolloConfig : boost::noncopyable {
 public:
  bool Init();
  void Close();
 public:
  static ApolloConfig& Get() {
    static ApolloConfig config;
    return config;
  }

 private:
  ApolloConfig() = default;
  bool UpdateConfig(const std::string& ns, bool is_prop, const apollo::ApiResponse& response);

 private:
  std::thread worker_;
  bool running_{true};
  std::string hostname_;
  std::shared_ptr<brpc::Channel> channel_{nullptr};
  std::unique_ptr<ApolloSource> remote_handle_;
  std::unique_ptr<ApolloSink> local_handle_;
  std::unique_ptr<ApolloSource> deliver_handle_;
  std::vector<ApolloTask> tasks_;
};
}
