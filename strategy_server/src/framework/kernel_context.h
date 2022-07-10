#pragma once

#include <memory>
#include <any>
#include "proto/graph.pb.h"

class KernelContext {
public:
  KernelContext();

  void Put(Session::Type key, const std::shared_ptr<std::any>& value);

  bool BuildSubContext(KernelContext* context, const std::vector<Session::Type>& inputs);

  template<typename Type>
  std::shared_ptr<Type> AnyCast(Session_Type index) {
    auto data = data_[index];
    if (data == nullptr) {
      return nullptr;
    }
    try {
      return std::any_cast<std::shared_ptr<Type>>(*data);
    } catch (std::bad_any_cast const & e) {
      return nullptr;
    }
  }

  void Clear();

private:
  std::vector<std::shared_ptr<std::any>> data_;
};


