#pragma once

#include <memory>

namespace grpc {
class Server;
}
namespace prometheus {
class Exposer;
}

namespace torch::serving {
class Metrics;
class ModelManager;
class KServeImpl;

class TorchServer {
 public:
  bool Init();
  void WaitUntilStop();

 private:
  std::shared_ptr<grpc::Server> grpc_server_{nullptr};
  std::shared_ptr<KServeImpl> kserve_{nullptr};
  std::shared_ptr<prometheus::Exposer> metrics_server_{nullptr};
  std::shared_ptr<Metrics> metrics_{nullptr};
  std::shared_ptr<ModelManager> model_manager_{nullptr};
};
}
