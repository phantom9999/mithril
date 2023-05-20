#pragma once

#include <string>
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>

namespace sw::redis {
class AsyncRedisCluster;
}

namespace RdKafka {
class Producer;
class Topic;
class DeliveryReportCb;
class EventCb;
}

using RedisClientPtr = std::shared_ptr<sw::redis::AsyncRedisCluster>;
using KafkaClientPtr = std::shared_ptr<RdKafka::Producer>;
using TopicPtr = std::shared_ptr<RdKafka::Topic>;
using DeliveryReportCbPtr = std::shared_ptr<RdKafka::DeliveryReportCb>;
using EventCbPtr = std::shared_ptr<RdKafka::EventCb>;

namespace ad {

class ResourceManager : public boost::noncopyable {
 public:
  bool Init();

  bool MGet(const std::vector<std::string>& keys, std::vector<std::string>* values);

  bool Sink(const std::string& msg);

  static const std::string& GetHost();

 private:
  RedisClientPtr redis_client_;
  KafkaClientPtr kafka_client_;
  TopicPtr topic_;
  DeliveryReportCbPtr delivery_report_cb_;
  EventCbPtr event_cb_;
};

}


