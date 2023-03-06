#include "apollo_interface.h"

namespace apollo {

boost::shared_ptr<google::protobuf::Message> GetMsgInner(const std::string& ns, const std::string& key) {
  auto& dataset = NamespaceSet::process_map;
  auto it = dataset.find(ns);
  if (it == dataset.end()) {
    return nullptr;
  }
  auto& subdataset = it->second;
  auto subit = subdataset.find(key);
  if (subit == subdataset.end()) {
    return nullptr;
  }
  if (subit->second == nullptr) {
    return nullptr;
  }
  auto data = subit->second->Get();
  if (data == nullptr) {
    return nullptr;
  }
  return data;
}

boost::shared_ptr<std::string> GetOther(const std::string& ns) {
  auto& dataset = NamespaceSet::data_map;
  auto it = dataset.find(ns);
  if (it == dataset.end()) {
    return nullptr;
  }
  return it->second.load();
}
}
