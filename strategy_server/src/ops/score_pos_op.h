#pragma once

#include "framework/op_kernel.h"

class ScorePosOp : public OpKernel {
protected:
  std::shared_ptr<std::any> Compute(const KernelContext &context) override;

};


