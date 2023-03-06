#include "apollo_config.h"
#include <iostream>
#include <brpc/controller.h>
#include <gflags/gflags.h>
#include <boost/format.hpp>
#include <brpc/channel.h>
#include <glog/logging.h>
#define GLOG_STL_LOGGING_FOR_UNORDERED
#include <glog/stl_logging.h>
#include <google/protobuf/util/json_util.h>

#include "apollo_global.h"

#include "apollo_utils.h"
#include "apollo.pb.h"
#include "apollo_sink.h"
#include "apollo_remote.h"
#include "apollo_deliver.h"
#include "apollo_flags.h"

namespace apollo {
bool ApolloConfig::Init() {
  if (FLAGS_apollo_interval < 10) {
    FLAGS_apollo_interval = 10;
  }
  for (const auto& entry : NamespaceSet::process_map) {
    tasks_.emplace_back(entry.first, true);
  }
  for (const auto& entry : NamespaceSet::data_map) {
    tasks_.emplace_back(entry.first, false);
  }
  LOG(INFO) << "register config: " << std::boolalpha << tasks_;

  remote_handle_ = std::make_unique<ApolloRemote>();
  local_handle_ = std::make_unique<ApolloSink>();
  deliver_handle_ = std::make_unique<ApolloDeliver>();
  if (!remote_handle_->Init()) {
    return false;
  }
  if (FLAGS_apollo_use_sink && !local_handle_->Init()) {
    return false;
  }
  if (FLAGS_apollo_use_deliver && !deliver_handle_->Init()) {
    return false;
  }

  auto UpdateWithSink = [&](const std::string& ns, bool is_prop, const apollo::ApiResponse& response)->bool {
    if (!UpdateConfig(ns, is_prop, response)) {
      return false;
    }
    if (FLAGS_apollo_use_sink) {
      local_handle_->SinkConfig(ns, response);
    }
    return true;
  };

  auto UpdateWithoutSink = [&](const std::string& ns, bool is_prop, const apollo::ApiResponse& response)->bool {
    return UpdateConfig(ns, is_prop, response);
  };

  bool is_fail = false;
  if (!remote_handle_->GetConfig(tasks_, UpdateWithSink)) {
    is_fail = true;
  }
  if (is_fail && FLAGS_apollo_use_deliver) {
    if (deliver_handle_->GetConfig(tasks_, UpdateWithSink)) {
      is_fail = false;
    }
  }
  if (is_fail && FLAGS_apollo_use_sink) {
    using namespace std::placeholders;
    if (local_handle_->GetConfig(tasks_, UpdateWithoutSink)) {
      is_fail = false;
    }
  }
  if (is_fail) {
    return false;
  }

  this->worker_ = std::thread([&](){
    while (running_) {
      std::this_thread::sleep_for(std::chrono::seconds(FLAGS_apollo_interval));
      LOG(INFO) << "try to update";
      remote_handle_->GetConfig(tasks_, UpdateWithSink);
      if (FLAGS_apollo_use_sink) {
        local_handle_->KeepALive();
      }
    }
  });
  return true;
}

void ApolloConfig::Close() {
  running_ = false;
  this->worker_.join();
  remote_handle_->Close();
  local_handle_->Close();
  deliver_handle_->Close();
}

bool ApolloConfig::UpdateConfig(const std::string &ns, bool is_prop, const ApiResponse &response) {
  if (is_prop) {
    // prop类型
    auto it = NamespaceSet::process_map.find(ns);
    if (it == NamespaceSet::process_map.end()) {
      LOG(WARNING) << "namespace[" << ns << "] miss processor";
      return false;
    }
    bool prop_success = true;
    for (const auto& sub_entry : it->second) {
      auto& key = sub_entry.first;
      auto& processor = sub_entry.second;
      auto sub_it = response.configurations().find(key);
      if (sub_it == response.configurations().end()) {
        prop_success = false;
        LOG(WARNING) << ns << " miss " << key;
        break;
      }
      if (!processor->TryParse(sub_it->second)) {
        prop_success = false;
        LOG(WARNING) << "(" << ns << "," << key << ") parse error";
        break;
      }
    }
    if (!prop_success) {
      return false;
    }
    for (const auto& sub_entry : it->second) {
      auto& key = sub_entry.first;
      auto& processor = sub_entry.second;
      processor->ParseAndSwap(response.configurations().at(key));
    }
  } else {
    auto it = response.configurations().find("content");
    if (it == response.configurations().end()) {
      LOG(WARNING) << "(" << ns << ") miss content";
      return false;
    }
    auto data = boost::make_shared<std::string>(it->second);
    NamespaceSet::data_map[ns].store(data);
  }
  return true;
}
}
