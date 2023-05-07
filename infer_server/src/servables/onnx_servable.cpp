#include "onnx_servable.h"
#include <mutex>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <absl/types/span.h>
#include <glog/logging.h>
#include <absl/strings/str_format.h>
#include <boost/filesystem.hpp>
#include <absl/strings/str_cat.h>

#include "model_spec.pb.h"
#include "kserve_predict_v2.pb.h"
#include "model/predict_context.h"
#include "utils/stop_watch.h"

namespace torch::serving {

struct OnnxServable::GlobalEnv {
  std::unique_ptr<Ort::Env> env;
  std::unique_ptr<Ort::SessionOptions> session_options;
};

struct OnnxServable::Module {
  std::shared_ptr<Ort::Session> session_;
  inference::ModelSpec spec;
  int64_t version{0};
  std::string label;
  Ort::AllocatorWithDefaultOptions allocator;
  Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
  std::vector<Ort::AllocatedStringPtr> input_names;
  std::vector<Ort::AllocatedStringPtr> output_names;
};


std::shared_ptr<OnnxServable::GlobalEnv> OnnxServable::global_env_ = nullptr;

bool OnnxServable::Init(const std::string &path) {
  StopWatch stop_watch;
  static std::once_flag flag;
  std::call_once(flag, [](){
    auto env = std::make_shared<OnnxServable::GlobalEnv>();
    try {
      env->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "kserve");
      env->session_options = std::make_unique<Ort::SessionOptions>();
    } catch (const Ort::Exception& e) {
      LOG(WARNING) << "can't create onnx Env: " << e.what();
      return;
    }

    OnnxServable::global_env_ = env;
  });
  if (global_env_ == nullptr) {
    LOG(WARNING) << "onnx env nullptr";
    return false;
  }

  auto model = std::make_shared<OnnxServable::Module>();
  boost::filesystem::path model_dir(path);

  if (!ReadVersion(path, &model->version)) {
    return false;
  }
  if (!ReadSpec(path, &model->spec)) {
    return false;
  }
  LOG(INFO) ;

  auto model_file = model_dir / ONNX_MODEL_FILE;
  try {
    auto options = global_env_->session_options->Clone();
    auto& env = *global_env_->env;
    model->session_ = std::make_shared<Ort::Session>(env, model_file.string().c_str(), options);
    model->input_names.reserve(model->session_->GetInputCount());
    model->input_names.reserve(model->session_->GetOutputCount());
    for (size_t i=0; i< model->session_->GetInputCount(); ++i) {
      model->input_names.push_back(model->session_->GetInputNameAllocated(i, model->allocator));
    }
    for (size_t i=0; i<model->session_->GetOutputCount(); ++i) {
      model->output_names.push_back(model->session_->GetOutputNameAllocated(i, model->allocator));
    }
  } catch (const Ort::Exception& e) {
    LOG(WARNING) << "onnx create servable error: " << e.what();
    return false;
  }

  a_module_ = model;
  if (!Warmup(path)) {
    return false;
  }

  LOG(INFO) << "model path:" << path
    << "; version: " << a_module_->version
    << "; spec: " << a_module_->spec.ShortDebugString()
    << "; cost " << stop_watch.Elapsed() << "us";
  return true;
}
PredictStatus OnnxServable::PredictWithoutCheck(const std::shared_ptr<PredictContext> &predict_contexts) {
  std::vector<Ort::Value> inputs;
  auto& session = a_module_->session_;
  std::vector<Ort::Value> input_tensors;
  std::vector<const char*> input_names;
  input_names.reserve(a_module_->input_names.size());
  for (const auto& item : a_module_->input_names) {
    input_names.push_back(item.get());
  }
  std::vector<const char*> output_names;
  output_names.reserve(a_module_->output_names.size());
  for (const auto& item : a_module_->output_names) {
    output_names.push_back(item.get());
  }

  {
    auto status = BuildInputTensor(*predict_contexts->request_, &input_tensors);
    if (!status.Ok()) {
      return status;
    }
  }

  std::vector<Ort::Value> result;
  try {
    result = session->Run(Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(), input_tensors.size(), output_names.data(), output_names.size());
  } catch (const Ort::Exception& e) {
    LOG(WARNING) << e.what();
    return {PredictStatus::Status::PREDICT_ERROR, e.what()};
  }

  {
    auto status = ParseOutputTensor(result, predict_contexts->response_);
    if (!status.Ok()) {
      return status;
    }
  }
  return {PredictStatus::OK};
}
const std::string &OnnxServable::GetLabel() {
  return a_module_->label;
}
const inference::ModelSpec &OnnxServable::GetSpec() {
  return a_module_->spec;
}
PredictStatus OnnxServable::BuildInputTensor(const inference::ModelInferRequest &request,
                                             std::vector<Ort::Value> *input_tensors) {
  std::unordered_map<std::string, const inference::ModelInferRequest::InferInputTensor*> tensor_map;
  for (const auto& item : request.inputs()) {
    tensor_map[item.name()] = &item;
  }
  const size_t input_count = a_module_->session_->GetInputCount();
  for (size_t i=0;i< input_count; ++i) {
    auto& name = a_module_->input_names[i];
    auto it = tensor_map.find(name.get());
    if (it == tensor_map.end()) {
      return {PredictStatus::Status::MISS_FEATURE, absl::StrCat("miss ", name.get())};
    }
    auto& input = it->second;

    switch (input->datatype()) {
      case inference::DT_INVALID: continue;
      case inference::DT_BOOL: {
        input_tensors->push_back(Ort::Value::CreateTensor<bool>(
            a_module_->memoryInfo.GetConst(),
            const_cast<bool*>(input->contents().bool_contents().data()), input->contents().bool_contents_size(),
            input->shape().data(), input->shape_size()));
        break;
      }
      case inference::DT_INT32: {
        input_tensors->push_back(Ort::Value::CreateTensor<int32_t>(
            a_module_->memoryInfo.GetConst(),
            const_cast<int32_t*>(input->contents().int_contents().data()), input->contents().int_contents_size(),
            input->shape().data(), input->shape_size()));
        break;
      }
      case inference::DT_UINT32: {
        input_tensors->push_back(Ort::Value::CreateTensor<uint32_t>(
            a_module_->memoryInfo.GetConst(),
            const_cast<uint32_t*>(input->contents().uint_contents().data()), input->contents().uint_contents_size(),
            input->shape().data(), input->shape_size()));
        break;
      }
      case inference::DT_INT64: {
        input_tensors->push_back(Ort::Value::CreateTensor<int64_t>(
            a_module_->memoryInfo.GetConst(),
            const_cast<int64_t*>(input->contents().int64_contents().data()), input->contents().int64_contents_size(),
            input->shape().data(), input->shape_size()));
        break;
      }
      case inference::DT_UINT64: {
        input_tensors->push_back(Ort::Value::CreateTensor<uint64_t>(
            a_module_->memoryInfo.GetConst(),
            const_cast<uint64_t*>(input->contents().uint64_contents().data()), input->contents().uint64_contents_size(),
            input->shape().data(), input->shape_size()));
        break;
      }
      case inference::DT_FLOAT: {
        input_tensors->push_back(Ort::Value::CreateTensor<float>(
            a_module_->memoryInfo.GetConst(),
            const_cast<float*>(input->contents().fp32_contents().data()), input->contents().fp32_contents_size(),
            input->shape().data(), input->shape_size()));
        break;
      }
      case inference::DT_DOUBLE: {
        input_tensors->push_back(Ort::Value::CreateTensor<double>(
            a_module_->memoryInfo.GetConst(),
            const_cast<double*>(input->contents().fp64_contents().data()), input->contents().fp64_contents_size(),
            input->shape().data(), input->shape_size()));
        break;
      }
      default: {
        return {PredictStatus::FEATURE_TYPE_ERROR, absl::StrFormat("feature:%s; type:%s, not support", name.get(), inference::DataType_Name(input->datatype()).c_str())};
        break;
      };
    }
  }
  return {PredictStatus::OK};
}
PredictStatus OnnxServable::ParseOutputTensor(const std::vector<Ort::Value> &output_tensors,
                                              inference::ModelInferResponse *response) {
  auto& session = a_module_->session_;
  const size_t output_count = std::min(output_tensors.size(), a_module_->output_names.size());

  for (size_t i=0; i<output_count; ++i) {
    auto& name = a_module_->output_names[i];
    auto type_info = session->GetOutputTypeInfo(i);
    auto type_shape = type_info.GetTensorTypeAndShapeInfo();
    auto& line = output_tensors[i];
    auto* output = response->add_outputs();
    output->set_name(name.get());
    auto shape = type_shape.GetShape();
    output->mutable_shape()->Add(shape.begin(), shape.end());
    switch (type_shape.GetElementType()) {
      case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: {
        output->set_datatype(inference::DT_FLOAT);
        absl::Span<float> data(const_cast<float*>(line.GetTensorData<float>()), type_shape.GetElementCount());
        output->mutable_contents()->mutable_fp32_contents()->Add(data.begin(), data.end());
        break;
      }
      case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE: {
        output->set_datatype(inference::DT_DOUBLE);
        absl::Span<double> data(const_cast<double*>(line.GetTensorData<double>()), type_shape.GetElementCount());
        output->mutable_contents()->mutable_fp64_contents()->Add(data.begin(), data.end());
        break;
      }
      case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32: {
        output->set_datatype(inference::DT_INT32);
        absl::Span<int32_t> data(const_cast<int32_t*>(line.GetTensorData<int32_t>()), type_shape.GetElementCount());
        output->mutable_contents()->mutable_int_contents()->Add(data.begin(), data.end());
        break;
      }
      case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: {
        output->set_datatype(inference::DT_INT64);
        absl::Span<int64_t> data(const_cast<int64_t*>(line.GetTensorData<int64_t>()), type_shape.GetElementCount());
        output->mutable_contents()->mutable_int64_contents()->Add(data.begin(), data.end());
        break;
      }
      case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32: {
        output->set_datatype(inference::DT_UINT32);
        absl::Span<uint32_t> data(const_cast<uint32_t*>(line.GetTensorData<uint32_t>()), type_shape.GetElementCount());
        output->mutable_contents()->mutable_uint_contents()->Add(data.begin(), data.end());
        break;
      }
      case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64: {
        output->set_datatype(inference::DT_UINT64);
        absl::Span<uint64_t> data(const_cast<uint64_t*>(line.GetTensorData<uint64_t>()), type_shape.GetElementCount());
        output->mutable_contents()->mutable_uint64_contents()->Add(data.begin(), data.end());
        break;
      }
      case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL: {
        output->set_datatype(inference::DT_BOOL);
        absl::Span<bool> data(const_cast<bool*>(line.GetTensorData<bool>()), type_shape.GetElementCount());
        output->mutable_contents()->mutable_bool_contents()->Add(data.begin(), data.end());
        break;
      }
      default: break;
    }
  }

  return {PredictStatus::OK};
}
}

