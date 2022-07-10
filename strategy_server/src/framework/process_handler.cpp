#include "process_handler.h"

bool ProcessHandler::Run(const StrategyRequest* request, Session::Type request_type, StrategyResponse* response, Session_Type response_type) {
  global_context_.Clear();
  auto requestPtr = std::make_shared<StrategyRequest>();
  requestPtr->CopyFrom(*request);
  global_context_.Put(request_type, std::make_shared<std::any>(requestPtr));
  executor_.run(taskflow_).get();
  auto responsePtr = global_context_.AnyCast<StrategyResponse>(response_type);
  if (responsePtr == nullptr) {
    return false;
  }
  response->Swap(responsePtr.get());
  return true;
}
std::string ProcessHandler::Dump() {
  std::ostringstream oss;
  Dump(oss);
  return oss.str();
}
void ProcessHandler::Dump(std::ostream &os) {
  taskflow_.dump(os);
}

