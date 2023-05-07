#include "servable.h"

#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <absl/strings/numbers.h>

#include "model/predict_context.h"
#include "utils/stop_watch.h"
#include "utils/pbtext.h"
#include "model_spec.pb.h"
#include "kserve_predict_v2.pb.h"

namespace torch::serving {

PredictStatus IServable::Check(const std::shared_ptr<PredictContext> &predict_context) {
  if (predict_context == nullptr || !predict_context->Check()) {
    LOG(WARNING) << "module nullptr";
    return {PredictStatus::NULLPTR};
  }

  StopWatch stop_watch;
  predict_context->time_state_.before_check = stop_watch.Current();
  auto status = checker_.Check(*predict_context->request_, GetSpec());
  predict_context->time_state_.after_check = stop_watch.Current();
  return status;
}
PredictStatus IServable::Predict(const std::shared_ptr<PredictContext> &predict_context) {
  auto status = Check(predict_context);
  if (!status.Ok()) {
    return status;
  }
  return PredictWithoutCheck(predict_context);
}


bool IServable::ReadSpec(const std::string& model_dir, inference::ModelSpec* model_spec) {
  if (model_spec == nullptr) {
    return false;
  }
  boost::filesystem::path path(model_dir);
  auto spec_file = path / FEATURE_SPEC_FILE;

  if (!ReadPbText(spec_file.string(), model_spec)) {
    LOG(WARNING) << "parse " << spec_file.string() << " error";
    return false;
  }
  return true;
}
bool IServable::ReadVersion(const std::string& model_dir, int64_t* model_version) {
  if (model_version == nullptr) {
    return false;
  }
  boost::filesystem::path path(model_dir);
  return absl::SimpleAtoi(path.filename().string(), model_version);
}
bool IServable::Warmup(const std::string& model_dir) {
  boost::filesystem::path path(model_dir);
  auto warmup_file = path / WARMUP_FILE;
  inference::ModelInferRequest request;
  inference::ModelInferResponse response;
  if (!ReadPbBin(warmup_file.string(), &request)) {
    LOG(WARNING) << "read warmup file error";
    return false;
  }
  auto context = std::make_shared<PredictContext>(&request, &response);
  auto status = Predict(context);
  if (!status.Ok()) {
    LOG(WARNING) << "warmup error: " << status.Message();
    return false;
  }
  return true;
}



}
