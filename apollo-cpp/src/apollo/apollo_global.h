#pragma once

#include <string>
#include <unordered_map>
#include <boost/smart_ptr.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>

namespace google::protobuf {
class Message;
}

namespace apollo {

class Processor {
 public:
  virtual ~Processor() = default;
  virtual bool ParseAndSwap(const std::string &data) = 0;
  virtual bool TryParse(const std::string &data) = 0;
  virtual boost::shared_ptr<google::protobuf::Message> Get() = 0;
 protected:
  bool ParseInner(const std::string &data, google::protobuf::Message *msg);
};

using ProcessorPtr = std::unique_ptr<Processor>;

class NamespaceSet {
  static inline std::unordered_map<std::string, std::unordered_map<std::string, ProcessorPtr>> process_map;
  static inline std::unordered_map<std::string, boost::atomic_shared_ptr<std::string>> data_map;
  friend class Register;
  friend class OtherRegister;
  friend class ApolloDeliverServiceImpl;
  friend class ApolloConfig;
  friend boost::shared_ptr<google::protobuf::Message> GetMsgInner(const std::string& ns, const std::string& key);
  friend boost::shared_ptr<std::string> GetOther(const std::string& ns);
};
}
