#pragma once

#include <memory>
#include <any>
#include <glog/logging.h>
#include "proto/graph.pb.h"

class KernelContext {
public:
  KernelContext();

  void Put(Session::Type key, const std::shared_ptr<std::any>& value);

  bool BuildSubContext(KernelContext* context, const std::vector<Session::Type>& inputs);

  template<typename Type>
  std::shared_ptr<Type> AnyCast(Session_Type index) const {
    auto data = data_[index];
    if (data == nullptr || !data->has_value()) {
      return nullptr;
    }
    try {
      return std::any_cast<std::shared_ptr<Type>>(*data);
    } catch (std::bad_any_cast const & e) {
      LOG(WARNING) << Session_Type_Name(index) << std::boolalpha
        << "; has " << data->has_value()
        << "; type " << data->type().name()
        << "; but " << e.what();
      return nullptr;
    }
  }

  void Clear();

private:
  std::vector<std::shared_ptr<std::any>> data_;
};


