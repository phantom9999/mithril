#pragma once

#include <string>

#include <google/protobuf/message.h>
#include <boost/smart_ptr.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include <glog/logging.h>
#include "apollo_global.h"

namespace apollo {

struct Register {
  Register(const std::string &ns, const std::string &key, ProcessorPtr process_base);
};

struct OtherRegister {
  explicit OtherRegister(const std::string &ns);
};
}

#define APOLLO_DEFINE(NS, KEY, MSG) \
namespace apollo {                  \
boost::shared_ptr<MSG> Get ##NS ##KEY(); \
namespace inner {                   \
class Process##NS ##KEY : public Processor { \
public:                             \
  bool ParseAndSwap(const std::string &data) final { \
    auto msg = boost::make_shared<MSG>();\
    if (!ParseInner(data, msg.get())) { \
      return false;                 \
    }                               \
    data_.store(msg);               \
    LOG(INFO) << "(" #NS "," #KEY "): " << msg->ShortDebugString(); \
    return true;                    \
  }                                 \
  boost::shared_ptr<google::protobuf::Message> Get() final {        \
    return data_.load(std::memory_order_acquire);    \
  }                                 \
  bool TryParse(const std::string& data) final {     \
    auto msg = std::make_unique<MSG>();  \
    return ParseInner(data, msg.get());  \
  }                                 \
 private:                           \
  static inline boost::atomic_shared_ptr<MSG> data_; \
  friend boost::shared_ptr<MSG> apollo::Get##NS ##KEY();            \
};                                  \
static ::apollo::Register register_ ##NS ##KEY (#NS, #KEY, std::make_unique<Process##NS ##KEY>()); \
}                                   \
boost::shared_ptr<MSG> Get##NS ##KEY() { \
  return inner::Process##NS ##KEY::data_.load(std::memory_order_acquire);                          \
}                                   \
}

#define APOLLO_DECLARE(NS, KEY, MSG) \
namespace apollo {                   \
boost::shared_ptr<MSG> Get ##NS ##KEY(); \
}

#define APOLLO_TYPE_JSON ".json"
#define APOLLO_TYPE_YAML ".yaml"
#define APOLLO_TYPE_YML ".yml"
#define APOLLO_TYPE_XML ".xml"
#define APOLLO_TYPE_TXT ".txt"

#define APOLLO_OTHER_DEFINE(NS, SUFFIX) \
namespace apollo { \
static ::apollo::OtherRegister register_other_ ##NS(#NS SUFFIX); \
const char* k ##NS = #NS SUFFIX;     \
}

#define APOLLO_DEFINE_JSON(NS) APOLLO_OTHER_DEFINE(NS, APOLLO_TYPE_JSON)
#define APOLLO_DEFINE_YAML(NS) APOLLO_OTHER_DEFINE(NS, APOLLO_TYPE_YAML)
#define APOLLO_DEFINE_YML(NS) APOLLO_OTHER_DEFINE(NS, APOLLO_TYPE_YML)
#define APOLLO_DEFINE_XML(NS) APOLLO_OTHER_DEFINE(NS, APOLLO_TYPE_XML)
#define APOLLO_DEFINE_TXT(NS) APOLLO_OTHER_DEFINE(NS, APOLLO_TYPE_TXT)

#define APOLLO_OTHER_DECLARE(NS, SUFFIX) \
namespace apollo {                       \
extern const char* k ##NS;                      \
}

#define APOLLO_DECLARE_JSON(NS) APOLLO_OTHER_DECLARE(NS, APOLLO_TYPE_JSON)
