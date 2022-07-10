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
  static void Register(Session::Type key, Creater* creater) {
    creaters_[key] = creater;
  }

private:
  // <output, [input]>
  std::unordered_map<Session_Type, std::vector<Session_Type>> graph_;
  inline static std::vector<Creater*> creaters_{};
};