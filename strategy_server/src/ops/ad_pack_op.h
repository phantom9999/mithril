#pragma once

#include "framework/op_kernel.h"

class AdPackOp : public OpKernel {
public:
protected:
  std::shared_ptr<std::any> Compute(const KernelContext &context) override;
};





