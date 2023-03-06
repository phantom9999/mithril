#include "apollo_global.h"
#include <glog/logging.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

namespace apollo {
bool Processor::ParseInner(const std::string& data, google::protobuf::Message *msg) {
  if (msg == nullptr) {
    return false;
  }
  auto status = google::protobuf::util::JsonStringToMessage(data, msg);
  if (status.ok()) {
    return true;
  }
  LOG(WARNING) << "parse error: " << status.message();
  return false;
}
}
