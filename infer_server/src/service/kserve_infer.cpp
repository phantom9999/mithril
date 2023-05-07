#include "kserve_impl.h"

#include <unordered_set>

#include <glog/logging.h>
#include <absl/strings/str_join.h>

#include "model/model_manager.h"
#include "servables/servable.h"
#include "model/predict_context.h"
#include "service/metrics.h"
#include "utils/latency_guard.h"


namespace {

std::unique_ptr<torch::serving::LatencyGuard> MakeModelLr(const std::unordered_map<std::string, prometheus::Summary*>& metrics, const std::string& name) {
  auto it = metrics.find(name);
  if (it == metrics.end()) {
    return std::make_unique<torch::serving::LatencyGuard>(nullptr);
  }
  return std::make_unique<torch::serving::LatencyGuard>(it->second);
}


}

namespace torch::serving {

std::shared_ptr<IServable> GetServableBySpec(const std::shared_ptr<ModelManager>& model_manager, const inference::ModelInferRequest& request) {
  if (model_manager == nullptr) {
    return nullptr;
  }
  switch (request.version_choice_case()) {
    case inference::ModelInferRequest::kModelVersion: {
      return model_manager->GetServableByVersion(request.model_name(), request.model_version());
    }
    case inference::ModelInferRequest::kVersionLabel: {
      return model_manager->GetServableByLabel(request.model_name(), request.version_label());
    }
    case inference::ModelInferRequest::VERSION_CHOICE_NOT_SET: {
      return model_manager->GetServableByVersion(request.model_name());
    }
  }
  return nullptr;
}

PredictStatus PredictInner(const std::shared_ptr<IServable>& servable, const std::shared_ptr<PredictContext>& predict_context) {
  auto status = servable->Predict(predict_context);
  if (!status.Ok()) {
    return status;
  }

  int64_t item_size = 0;
  std::unordered_set<std::string> feature_name;
  for (const auto& entry : predict_context->request_->inputs()) {
    item_size = entry.shape(0);
    feature_name.insert(entry.name());
  }
  auto* response = predict_context->response_;
  LOG(INFO) << response->model_name() << ":" << response->model_version() << "; feature:" << absl::StrJoin(feature_name, ",")
            << "; item:" << item_size << "; " << predict_context->time_state_.ToString();
  return {PredictStatus::OK};
}


grpc::Status KServeImpl::ModelInfer(::grpc::ServerContext *context,
                                    const ::inference::ModelInferRequest *request,
                                    ::inference::ModelInferResponse *response) {
  auto service_lr = std::make_unique<LatencyGuard>(service_metrics_);
  if (!request->model_name().empty()) {
    return {grpc::INVALID_ARGUMENT, "miss model spec"};
  }
  auto model_lr = MakeModelLr(model_metrics_, request->model_name());

  std::shared_ptr<IServable> servable = GetServableBySpec(model_manager_, *request);
  if (servable == nullptr) {
    return {grpc::INTERNAL, absl::StrCat(request->model_name(), "'s servable not found")};
  }

  auto predict_context = std::make_shared<PredictContext>(request, response);
  auto status = PredictInner(servable, predict_context);

  if (!status.Ok()) {
    return {grpc::INTERNAL, status.Message()};
  }

  return grpc::Status::OK;
}

}


