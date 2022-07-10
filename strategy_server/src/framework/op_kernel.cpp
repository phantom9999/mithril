#include "op_kernel.h"
#include <glog/logging.h>

void OpKernel::BindTask(const tf::Task& task) {
  task_ = task;
  std::stringstream ss;
  ss << "task: " << task.name() << ";inputs: ";
  for (const auto& key : inputs_) {
    ss << Session_Type_Name(key) << ",";
  }
  ss << ";output: " << Session_Type_Name(output_);
  // LOG(INFO) << ss.str();
}

void OpKernel::Run() {
  auto* global_context = static_cast<KernelContext *>(task_.data());
  KernelContext context;
  global_context->BuildSubContext(&context, inputs_);
  auto result = Compute(context);
  global_context->Put(output_, result);
}
void OpKernel::BindMeta(const std::vector<Session::Type> &inputs, Session::Type output) {
  this->inputs_.clear();
  this->inputs_ = inputs;
  this->output_ = output;
}
