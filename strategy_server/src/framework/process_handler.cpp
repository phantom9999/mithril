#include "process_handler.h"

bool ProcessHandler::Run(const StrategyRequest* request, Session::Type requestType, StrategyResponse* response, Session_Type response_type) {
  global_context_.Clear();
  auto requestPtr = std::make_shared<StrategyRequest>();
  requestPtr->CopyFrom(*request);
  global_context_.Put(requestType, std::make_shared<std::any>(request));
  executor_.run(taskflow_).get();
  auto responsePtr = global_context_.AnyCast<StrategyResponse>(response_type);
  if (responsePtr == nullptr) {
    return false;
  }
  response->Swap(responsePtr.get());
  return true;
}

