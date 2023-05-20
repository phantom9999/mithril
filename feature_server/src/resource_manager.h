#pragma once

#include <string>
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>

namespace sw::redis {
class AsyncRedisCluster;
}

using RedisClientPtr = std::shared_ptr<sw::redis::AsyncRedisCluster>;

namespace ad {

class ResourceManager : public boost::noncopyable {
 public:
  bool Init();

  bool MGet(const std::vector<std::string>& keys, std::vector<std::string>* values);


 private:
  RedisClientPtr redis_client_;
};

}


