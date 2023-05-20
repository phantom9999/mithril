#include "resource_manager.h"
#include <mutex>
#include <sw/redis++/async_redis++.h>
#include <glog/logging.h>
#include <librdkafka/rdkafkacpp.h>

namespace {
class ProducerDeliveryReportCb : public RdKafka::DeliveryReportCb {
 public:
  void dr_cb(RdKafka::Message &message) override {
    if (message.err()) {
      LOG(WARNING) << "Message delivery failed: " << message.errstr();
    } else {
      LOG_EVERY_N(INFO, 1000) << "Message delivered to topic " << message.topic_name()
                << " [" << message.partition() << "] at offset "
                << message.offset();
    }
  }
};

class ProducerEventCb : public RdKafka::EventCb {
 public:
  void event_cb(RdKafka::Event &event) override {
    switch (event.type()) {
      case RdKafka::Event::EVENT_ERROR: {
        LOG(WARNING) << "RdKafka::Event::EVENT_ERROR: " << RdKafka::err2str(event.err());
        break;
      }
      case RdKafka::Event::EVENT_STATS: {
        LOG(INFO) << "RdKafka::Event::EVENT_STATS: " << event.str();
        break;
      }
      case RdKafka::Event::EVENT_LOG: {
        LOG(INFO) << "RdKafka::Event::EVENT_LOG " << event.fac();
        break;
      }
      case RdKafka::Event::EVENT_THROTTLE: {
        LOG(WARNING) << "RdKafka::Event::EVENT_THROTTLE " << event.broker_name();
        break;
      }
    }
  }
};
}


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
  {
    std::string topic;
    std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    std::string err;
    if (conf->set("bootstrap.servers", "", err) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << err;
      return false;
    }
    if (conf->set("compression.type", "gzip", err) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << err;
      return false;
    }
    if (conf->set("message.max.bytes", std::to_string(2*1024*1024), err) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << err;
      return false;
    }
    delivery_report_cb_ = std::make_shared<ProducerDeliveryReportCb>();
    if (conf->set("dr_cb", delivery_report_cb_.get(), err) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << err;
      return false;
    }
    event_cb_ = std::make_shared<ProducerEventCb>();
    if (conf->set("event_cb", event_cb_.get(), err) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << err;
      return false;
    }
    if (conf->set("statistics.interval.ms", std::to_string(10*1000), err) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << err;
      return false;
    }

    auto* producer = RdKafka::Producer::create(conf.get(), err);
    if (producer == nullptr) {
      LOG(ERROR) << err;
      return false;
    }
    kafka_client_.reset(producer, [](RdKafka::Producer* p){
      for (int i=0;i<10 && p->outq_len() > 0;++i) {
        p->flush(5 * 1000);
      }
    });
    topic_.reset(RdKafka::Topic::create(producer, topic, nullptr, err));
    if (topic_ == nullptr) {
      LOG(ERROR) << err;
      return false;
    }
  }
  return true;
}
bool ResourceManager::Sink(const std::string &msg) {
  for (int i=0;i<3;++i) {
    auto err = kafka_client_->produce(
        topic_.get(),
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(msg.c_str()),
        msg.size(),
        nullptr,
        nullptr);
    if (err == RdKafka::ERR_NO_ERROR) {
      break;
    }
    LOG(WARNING) << RdKafka::err2str(err);
  }
  kafka_client_->poll(0);
  return true;
}

static std::once_flag flag1;

const std::string &ResourceManager::GetHost() {
  static std::string host;
  std::call_once(flag1, [&](){
    std::string hostname;
    hostname.resize(128);
    if (gethostname(hostname.data(), hostname.size()) != 0) {
      LOG(WARNING) << "get hostname error: " << strerror(errno);
      host = "unknown";
    }
    host = {hostname.c_str()};
  });
  return host;
}
}


