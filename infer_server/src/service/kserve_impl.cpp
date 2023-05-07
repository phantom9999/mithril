#include "kserve_impl.h"
#include "service/metrics.h"
#include "model/model_manager.h"
#include "model/servable_model.h"
#include "servables/servable.h"
#include "model_spec.pb.h"
#include "version.h"
#include <absl/strings/str_format.h>

namespace torch::serving {

KServeImpl::KServeImpl(const std::shared_ptr<ModelManager>& model_manager, const std::shared_ptr<Metrics>& metrics)
  : model_manager_(model_manager) {
  std::vector<std::string> model_list;
  model_manager->GetModelList(&model_list);
  for (const auto& item : model_list) {
    model_metrics_.insert(std::make_pair(item, metrics->GetModelSummary(item)));
  }
  service_metrics_ = metrics->GetServiceSummary(inference::GRPCInferenceService::service_full_name());
}
grpc::Status KServeImpl::ServerLive(::grpc::ServerContext *context,
                                    const ::inference::ServerLiveRequest *request,
                                    ::inference::ServerLiveResponse *response) {
  response->set_live(model_manager_ != nullptr);
  return grpc::Status::OK;
}
grpc::Status KServeImpl::ServerReady(::grpc::ServerContext *context,
                                     const ::inference::ServerReadyRequest *request,
                                     ::inference::ServerReadyResponse *response) {
  response->set_ready(model_manager_ != nullptr && model_manager_->Ready());
  return grpc::Status::OK;
}
grpc::Status KServeImpl::ModelReady(::grpc::ServerContext *context,
                                    const ::inference::ModelReadyRequest *request,
                                    ::inference::ModelReadyResponse *response) {
  if (model_manager_ == nullptr || !model_manager_->Ready()) {
    response->set_ready(false);
  } else {
    auto servable = model_manager_->GetServableByVersion(request->name(), request->version());
    response->set_ready(servable != nullptr);
  }
  return grpc::Status::OK;
}
grpc::Status KServeImpl::ServerMetadata(::grpc::ServerContext *context,
                                        const ::inference::ServerMetadataRequest *request,
                                        ::inference::ServerMetadataResponse *response) {
  response->set_name("model-server");
  response->set_version(absl::StrFormat("%s:%s", branch_name.c_str(), commit_hash.c_str()));
  return grpc::Status::OK;
}
grpc::Status KServeImpl::ModelMetadata(::grpc::ServerContext *context,
                                       const ::inference::ModelMetadataRequest *request,
                                       ::inference::ModelMetadataResponse *response) {
  auto model = model_manager_->GetModel(request->name());
  if (model != nullptr) {
    response->set_name(request->name());
    std::unordered_set<ModelVersion> versions;
    model->GetAllVersion(&versions);
    response->mutable_versions()->Add(versions.begin(), versions.end());
    auto servable = model->GetServableByVersion(request->version());

    if (servable != nullptr) {
      auto& spec = servable->GetSpec();
      for (const auto& sub : spec.feature_specs()) {
        auto* input = response->add_inputs();
        input->set_name(sub.name());
        input->set_datatype(sub.dtype());
        input->mutable_shape()->Add(sub.shape().begin(), sub.shape().end());
      }
    }
  }

  return grpc::Status::OK;
}

grpc::Status KServeImpl::RepositoryModelLoad(::grpc::ServerContext *context,
                                             const ::inference::RepositoryModelLoadRequest *request,
                                             ::inference::RepositoryModelLoadResponse *response) {
  return Service::RepositoryModelLoad(context, request, response);
}
grpc::Status KServeImpl::RepositoryModelUnload(::grpc::ServerContext *context,
                                               const ::inference::RepositoryModelUnloadRequest *request,
                                               ::inference::RepositoryModelUnloadResponse *response) {
  return Service::RepositoryModelUnload(context, request, response);
}
}

