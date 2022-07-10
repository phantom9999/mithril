#include "kernel_context.h"
#include <glog/logging.h>
#include <glog/logging.h>

KernelContext::KernelContext() {
  data_.resize(Session_Type_Type_MAX);
}

void KernelContext::Put(Session::Type key, const std::shared_ptr<std::any>& value) {
  data_[key] = value;
}

bool KernelContext::BuildSubContext(KernelContext* context, const std::vector<Session::Type>& inputs) {
  for (const auto& key : inputs) {
    context->Put(key, data_[key]);
  }
  return true;
}

void KernelContext::Clear() {
  data_.clear();
  data_.resize(Session_Type_Type_MAX);
}
