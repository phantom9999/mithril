#include "torch_servable.h"

#include <numeric>
#include <fstream>

#include <boost/filesystem.hpp>
#include <torch/script.h>
#include <glog/logging.h>
#include <absl/types/span.h>
#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>
#include <google/protobuf/text_format.h>

#include "model_spec.pb.h"
#include "kserve_predict_v2.pb.h"
#include "model/model_define.h"
#include "utils/stop_watch.h"
#include "utils/pbtext.h"
#include "model/predict_context.h"

namespace torch::serving {

bool AddFeature(torch::Dict<std::string, torch::Tensor>* dict, const inference::ModelInferRequest::InferInputTensor& proto) {
  std::vector<int64_t> shape;
  shape.reserve(proto.shape_size());
  for (auto& item : proto.shape()) {
    shape.push_back(item);
  }

  torch::IntArrayRef shape_ref(shape);
  switch (proto.datatype()) {
    case inference::DT_FLOAT: {
      auto option = torch::TensorOptions(torch::ScalarType::Float);
      dict->insert(proto.name(), torch::from_blob(const_cast<float*>(proto.contents().fp32_contents().data()), shape_ref, option));
      break;
    }
    case inference::DT_DOUBLE: {
      auto option = torch::TensorOptions(torch::ScalarType::Double);
      dict->insert(proto.name(), torch::from_blob(const_cast<double*>(proto.contents().fp64_contents().data()), shape_ref, option));
      break;
    }
    case inference::DT_INT32: {
      auto option = torch::TensorOptions(torch::ScalarType::Int);
      dict->insert(proto.name(), torch::from_blob(const_cast<int32_t *>(proto.contents().int_contents().data()), shape_ref, option));
      break;
    }
    case inference::DT_UINT32: {
      auto option = torch::TensorOptions(torch::ScalarType::Int);
      dict->insert(proto.name(), torch::from_blob(const_cast<uint32_t *>(proto.contents().uint_contents().data()), shape_ref, option));
      break;
    }
    case inference::DT_INT64: {
      auto option = torch::TensorOptions(torch::ScalarType::Long);
      dict->insert(proto.name(), torch::from_blob(const_cast<int64_t*>(proto.contents().int64_contents().data()), shape_ref, option));
      break;
    }
    case inference::DT_UINT64: {
      auto option = torch::TensorOptions(torch::ScalarType::Long);
      dict->insert(proto.name(), torch::from_blob(const_cast<uint64_t *>(proto.contents().uint64_contents().data()), shape_ref, option));
      break;
    }
    default: {
      LOG(WARNING) << "feature [" << proto.name() << "] 'type is [" << inference::DataType_Name(proto.datatype()) << "] and does not support now";
      return false;
    }
  }
  return true;
}

bool PackResult(const torch::Tensor& tensor, inference::ModelInferResponse::InferOutputTensor * tensor_proto) {
  int64_t all_size = 1;
  for (const auto& size : tensor.sizes()) {
    tensor_proto->add_shape(size);
    all_size *= size;
  }
  auto* content = tensor_proto->mutable_contents();
  switch (torch::typeMetaToScalarType(tensor.dtype())) {
    case torch::ScalarType::Int:{
      tensor_proto->set_datatype(inference::DT_INT32);
      absl::Span<int32_t> line(tensor.data_ptr<int32_t>(), all_size);
      content->mutable_int_contents()->Add(line.begin(), line.end());
      break;
    }
    case torch::ScalarType::Long:{
      tensor_proto->set_datatype(inference::DT_INT64);
      absl::Span<int64_t> line(tensor.data_ptr<int64_t>(), all_size);
      content->mutable_int64_contents()->Add(line.begin(), line.end());
      break;
    }
    case torch::ScalarType::Float:{
      tensor_proto->set_datatype(inference::DT_FLOAT);
      absl::Span<float> line(tensor.data_ptr<float>(), all_size);
      content->mutable_fp32_contents()->Add(line.begin(), line.end());
      break;
    }
    case torch::ScalarType::Double:{
      tensor_proto->set_datatype(inference::DT_DOUBLE);
      absl::Span<double> line(tensor.data_ptr<double>(), all_size);
      content->mutable_fp64_contents()->Add(line.begin(), line.end());
      break;
    }
    default: {
      return false;
    }
  }
  return true;
}

torch::serving::PredictStatus PrePredict(const inference::ModelInferRequest& request, std::vector<torch::jit::IValue>* inputs) {
  torch::Dict<std::string, torch::Tensor> features;
  for (const auto& entry : request.inputs()) {
    if (!AddFeature(&features, entry)) {
      return {PredictStatus::FEATURE_TYPE_ERROR, entry.name() + " not support"};
    }
  }
  inputs->emplace_back(features);
  return {PredictStatus::OK};
}

PredictStatus PostPredict(const torch::IValue& result, inference::ModelInferResponse* response) {
  if (!result.isGenericDict()) {
    LOG(WARNING) << "result not dict";
    return {PredictStatus::RESULT_TYPE_ERROR, "model result is no dict"};
  }
  auto* result_map = response->mutable_outputs();
  for (const auto& entry : result.toGenericDict()) {
    auto& key = entry.key();
    auto& value = entry.value();
    if (!key.isString()|| !value.isTensor()) {
      LOG(WARNING) << "key value type error";
      continue;
    }
    auto* line_result = response->add_outputs();
    line_result->set_name(key.toStringRef());
    const auto& tensor = value.toTensor();
    if (!PackResult(tensor, line_result)) {
      LOG(WARNING) << "result [" << key.toStringRef() << "] not support";
    }
  }
  return {PredictStatus::OK};
}

struct TorchServable::TorchModule {
  torch::jit::script::Module a_module;
  inference::ModelSpec spec;
  int64_t version{0};
  std::string label;
};

bool TorchServable::Init(const std::string& path) {
  StopWatch stop_watch;
  auto torch_model = std::make_shared<TorchModule>();
  boost::filesystem::path model_dir(path);

  if (!ReadVersion(path, &torch_model->version)) {
    return false;
  }
  if (!ReadSpec(path, &torch_model->spec)) {
    return false;
  }
  LOG(INFO) << "model:" << path << "; version: " << torch_model->version
    << "; spec: " << torch_model->spec.ShortDebugString();

  auto model_file = model_dir / TORCH_MODEL_FILE;
  torch::InferenceMode inference_mode(true);
  try {
    auto a_module = torch::jit::load(model_file.string());
    a_module.eval();
    torch_model->a_module = a_module;
  } catch (const c10::Error& e) {
    LOG(WARNING) << "load " << path << " error: " << e.msg();
    return false;
  }
  torch_module_ = torch_model;
  // 预热
  if (!Warmup(path)) {
    return false;
  }

  LOG(INFO) << "model path:" << path
            << "; version: " << torch_module_->version
            << "; spec: " << torch_module_->spec.ShortDebugString()
            << "; cost " << stop_watch.Elapsed() << "us";
  return true;
}

const std::string& TorchServable::GetLabel() {
  if (torch_module_ == nullptr) {
    static std::string empty;
    return empty;
  }
  return torch_module_->label;
}

const inference::ModelSpec &TorchServable::GetSpec() {
  return torch_module_->spec;
}

PredictStatus TorchServable::PredictWithoutCheck(const PredictContextPtr &predict_context) {
  if (torch_module_ == nullptr || predict_context == nullptr || !predict_context->Check()) {
    LOG(WARNING) << "module nullptr";
    return {PredictStatus::NULLPTR};
  }
  auto* request = predict_context->request_;
  auto* response = predict_context->response_;
  StopWatch stop_watch;
  predict_context->time_state_.before_pack = stop_watch.Current();
  std::vector<torch::jit::IValue> inputs;
  {
    auto status = PrePredict(*request, &inputs);
    if (!status.Ok()) {
      return status;
    }
  }
  predict_context->time_state_.before_predict = stop_watch.Current();
  torch::IValue result;
  try {
    torch::jit::setGraphExecutorOptimize(true);
    torch::InferenceMode inference_mode(true);
    torch::NoGradGuard no_grad;
    result = torch_module_->a_module.forward(inputs);
  } catch (const torch::Error& e) {
    LOG(WARNING) << "predict error: " << e.msg();
    return {PredictStatus::PREDICT_ERROR, e.msg()};
  } catch (...) {
    LOG(WARNING) << "predict error";
    return {PredictStatus::PREDICT_ERROR, "unknonw"};
  }
  predict_context->time_state_.before_unpack = stop_watch.Current();
  {
    auto status = PostPredict(result, response);
    if (!status.Ok()) {
      return status;
    }
    response->set_id(request->id());
    response->set_model_name(request->model_name());
    response->set_model_version(torch_module_->version);
  }
  predict_context->time_state_.after_unpack = stop_watch.Current();

  return {PredictStatus::OK};
}
}
