#include "op_kernel.h"

OpKernel::OpKernel(const std::vector<Session::Type>& inputs, Session::Type output) : inputs_(inputs), output_(output) {

}

void OpKernel::BindTask(const tf::Task& task) {
  task_ = task;
}

void OpKernel::Run() {
  auto* global_context = static_cast<KernelContext *>(task_.data());
  KernelContext context;
  global_context->BuildSubContext(&context, inputs_);
  auto result = Compute(context);
  global_context->Put(output_, result);
}
