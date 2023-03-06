#pragma once

#include <string>
#include <boost/smart_ptr.hpp>
#include <google/protobuf/message.h>
#include "apollo_define.h"

namespace apollo {

boost::shared_ptr<google::protobuf::Message> GetMsgInner(const std::string& ns, const std::string& key);

template<typename Msg>
boost::shared_ptr<Msg> GetMsg(const std::string& ns, const std::string& key) {
  auto data = GetMsgInner(ns, key);
  if (data == nullptr) {
    return nullptr;
  }
  if (data->GetDescriptor() != Msg::GetDescriptor()) {
    return nullptr;
  }
  return boost::dynamic_pointer_cast<Msg>(data);
}

boost::shared_ptr<std::string> GetOther(const std::string& ns);
}
