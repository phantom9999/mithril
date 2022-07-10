#pragma once

#include <vector>
#include "proto/graph.pb.h"

class OpKernel;

class Creater {
public:
  virtual OpKernel* Create(const std::vector<Session::Type>& inputs, Session::Type output) = 0;
};

template<typename Op>
class OpCreater : public Creater {
public:
  OpKernel* Create(const std::vector<Session::Type>& inputs, Session::Type output) final {
    return new Op(inputs, output);
  }
};