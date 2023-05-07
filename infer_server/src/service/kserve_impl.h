#pragma once

#include "kserve_predict_v2.grpc.pb.h"

namespace prometheus {
class Summary;
}

namespace torch::serving {
class Metrics;
class ModelManager;

class KServeImpl : public inference::GRPCInferenceService::Service {
 public:
  KServeImpl(const std::shared_ptr<ModelManager>& model_manager, const std::shared_ptr<Metrics>& metrics);
  grpc::Status ServerLive(::grpc::ServerContext *context,
                          const ::inference::ServerLiveRequest *request,
                          ::inference::ServerLiveResponse *response) override;
  grpc::Status ServerReady(::grpc::ServerContext *context,
                           const ::inference::ServerReadyRequest *request,
                           ::inference::ServerReadyResponse *response) override;
  grpc::Status ModelReady(::grpc::ServerContext *context,
                          const ::inference::ModelReadyRequest *request,
                          ::inference::ModelReadyResponse *response) override;
  grpc::Status ServerMetadata(::grpc::ServerContext *context,
                              const ::inference::ServerMetadataRequest *request,
                              ::inference::ServerMetadataResponse *response) override;
  grpc::Status ModelMetadata(::grpc::ServerContext *context,
                             const ::inference::ModelMetadataRequest *request,
                             ::inference::ModelMetadataResponse *response) override;
  grpc::Status ModelInfer(::grpc::ServerContext *context,
                          const ::inference::ModelInferRequest *request,
                          ::inference::ModelInferResponse *response) override;
  grpc::Status RepositoryModelLoad(::grpc::ServerContext *context,
                                   const ::inference::RepositoryModelLoadRequest *request,
                                   ::inference::RepositoryModelLoadResponse *response) override;
  grpc::Status RepositoryModelUnload(::grpc::ServerContext *context,
                                     const ::inference::RepositoryModelUnloadRequest *request,
                                     ::inference::RepositoryModelUnloadResponse *response) override;

 private:
  const std::shared_ptr<ModelManager> model_manager_;
  std::unordered_map<std::string, prometheus::Summary*> model_metrics_;
  prometheus::Summary* service_metrics_;
};


}
