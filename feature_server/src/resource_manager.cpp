#include "resource_manager.h"
#include <sw/redis++/async_redis++.h>
#include <glog/logging.h>

namespace ad {

bool ResourceManager::MGet(const std::vector<std::string> &keys, std::vector<std::string> *values) {
  if (values == nullptr) {
    return false;
  }
  std::vector<sw::redis::Future<sw::redis::Optional<std::string>>> fu_list;
  fu_list.reserve(keys.size());
  values->reserve(keys.size());
  for (const auto& key : keys) {
    fu_list.push_back(redis_client_->get(key));
  }
  for (auto&& fu : fu_list) {
    try {
      auto result = fu.get();
      if (result.has_value()) {
        values->push_back(std::move(result.value()));
      }
    } catch (const std::exception& e) {
      LOG(WARNING) << e.what();
    } catch (...) {
      LOG(WARNING) << "unknown error";
    }
  }

  return true;
}
bool ResourceManager::Init() {
  {
    using namespace sw::redis;
    ConnectionOptions opts;
    opts.host = "localhost";
    opts.port = 6379;

    sw::redis::ConnectionPoolOptions pool_opts;
    pool_opts.size = 3;
    redis_client_ = std::make_shared<sw::redis::AsyncRedisCluster>(opts, pool_opts);
  }
  return true;
}
}


