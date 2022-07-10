#pragma once

#include <unordered_map>
#include <vector>
#include <memory>

#include <taskflow/taskflow.hpp>

#include "proto/graph.pb.h"
#include "framework/process_handler.h"
#include "framework/op_creater.h"

class HandlerFactory {
public:
  explicit HandlerFactory(const GraphDef& graph_def);

  std::unique_ptr<ProcessHandler> BuildNew(tf::Executor& executor);

public:
  static bool Register(Session::Type key, Creater creater);

private:
  // <output, [input]>
  std::unordered_map<Session_Type, OpDef> graph_;
  inline static std::unordered_map<Session_Type, Creater> creaters_;
};

#define OP_REGISTER(class_name, key) \
  static const bool register_op_##class_name = HandlerFactory::Register( \
    key, [](const std::vector<Session::Type>& inputs, Session::Type output)->OpKernel* { \
      auto op = new class_name;              \
      op->BindMeta(inputs, output);  \
      return op;                     \
      });
