#include "torch_server.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/server.h>
#include <absl/strings/str_format.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "server_config.pb.h"
#include "utils/pbtext.h"
#include "model/model_manager.h"
#include "service/kserve_impl.h"
#include "service/metrics.h"

DEFINE_string(config, "config/server.txt", "");

namespace torch::serving {

bool TorchServer::Init() {
  ServerConfig server_config;
  if (!ReadPbText(FLAGS_config, &server_config)) {
    LOG(WARNING) << "parse " << FLAGS_config << " error";
    return false;
  }

  auto registry = std::make_shared<prometheus::Registry>();

  if (server_config.has_metrics_config() && server_config.metrics_config().enable()) {
    metrics_ = std::make_shared<Metrics>(registry, server_config.metrics_config());
    metrics_server_ = std::make_shared<prometheus::Exposer>(absl::StrFormat("0.0.0.0:%d", server_config.metrics_config().port()));
    metrics_server_->RegisterCollectable(registry);
  } else {
    metrics_ = std::make_shared<Metrics>(registry);
  }
  model_manager_ = std::make_shared<ModelManager>();

  if (!model_manager_->Init(server_config.model_manager_config())) {
    LOG(WARNING) << "model manager init error";
    return false;
  }

  std::string server_address(absl::StrFormat("0.0.0.0:%d", server_config.grpc_port()));

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  kserve_ = std::make_shared<KServeImpl>(model_manager_, metrics_);
  builder.RegisterService(kserve_.get());

  grpc_server_ = builder.BuildAndStart();

  LOG(INFO) << "Server listening on " << server_address;
  return true;
}
void TorchServer::WaitUntilStop() {
  if (grpc_server_ != nullptr) {
    grpc_server_->Wait();
  }

  if (model_manager_ != nullptr) {
    model_manager_->Stop();
  }
}
}
