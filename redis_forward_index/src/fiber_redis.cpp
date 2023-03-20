#include "fiber_redis.h"

namespace fiber {

FiberRedis::FiberRedis(const ConnectionOptions &opts, const ConnectionPoolOptions &pool_opts)
  : redis_(opts, pool_opts){

}
Future<long long > FiberRedis::Del(const StringView &key) {
  auto promise = std::make_unique<Promise<long long >>();
  auto future = promise->get_future();
  redis_.del(key, [promise =promise.release()](sw::redis::Future<long long > &&fut) mutable {
    try {
      promise->set_value(fut.get());
    } catch (...) {
      promise->set_exception(std::current_exception());
    }
    delete promise;
  });
  return future;
}
Future<OptionalString> FiberRedis::Get(const StringView &key) {
  auto promise = std::make_unique<Promise<OptionalString>>();
  auto future = promise->get_future();
  redis_.get(key, [promise = promise.release()](sw::redis::Future<OptionalString> &&fut) mutable {
    try {
      promise->set_value(fut.get());
    } catch (...) {
      promise->set_exception(std::current_exception());
    }
    delete promise;
  });
  return future;
}
Future<bool> FiberRedis::Set(const StringView &key, const StringView &val) {
  auto promise = std::make_unique<Promise<bool>>();
  auto future = promise->get_future();
  redis_.set(key, val, [promise = promise.release()](sw::redis::Future<bool> &&fut) mutable {
    try {
      promise->set_value(fut.get());
    } catch (...) {
      promise->set_exception(std::current_exception());
    }
    delete promise;
  });
  return future;
}
}
