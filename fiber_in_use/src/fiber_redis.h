#pragma once

#include <memory>
#include <sw/redis++/async_redis++.h>
#include <boost/fiber/all.hpp>

namespace fiber {

template<typename Type>
using Promise = boost::fibers::promise<Type>;

template<typename Type>
using Future = boost::fibers::future<Type>;

using sw::redis::ConnectionOptions;
using sw::redis::ConnectionPoolOptions;
using sw::redis::OptionalDouble;
using sw::redis::OptionalLongLong;
using sw::redis::OptionalString;
using sw::redis::StringView;

class FiberRedis {
 public:
  FiberRedis(const ConnectionOptions &opts, const ConnectionPoolOptions &pool_opts);

  Future<long long > Del(const StringView &key);

  Future<OptionalString> Get(const StringView &key);

  Future<bool> Set(const StringView &key, const StringView &val);

 private:
  sw::redis::AsyncRedisCluster redis_;
};
}
