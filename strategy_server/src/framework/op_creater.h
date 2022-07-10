#pragma once

#include <vector>
#include <functional>
#include "proto/graph.pb.h"

/*
class OpKernel;

class Creater {
public:
  virtual ~Creater() = default;
  virtual OpKernel* Create(const std::vector<Session::Type>& inputs, Session::Type output) = 0;
};

template<typename Op>
class OpCreater : public Creater {
public:
  OpKernel* Create(const std::vector<Session::Type>& inputs, Session::Type output) final {
    auto op = new Op;
    op->BindMeta(inputs, output);
    return op;
  }
};
*/
class OpKernel;

using Creater = std::function<OpKernel*(const std::vector<Session::Type>& inputs, Session::Type output)>;
