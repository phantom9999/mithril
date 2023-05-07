#include "feature_checker.h"
#include <set>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>
#include "model_spec.pb.h"
#include "kserve_predict_v2.pb.h"

namespace torch::serving {

PredictStatus FeatureChecker::Check(const inference::ModelInferRequest& request, const inference::ModelSpec& model_spec) {
  // 特征数量
  std::set<std::string> input_name;
  std::set<std::string> expect_name;
  for (const auto& entry : request.inputs()) {
    input_name.insert(entry.name());
  }
  for (const auto& entry : model_spec.feature_specs()) {
    expect_name.insert(entry.name());
  }
  if (input_name.size() != expect_name.size()) {
    return {PredictStatus::FEATURE_SIZE_ERROR, absl::StrCat("input:", input_name.size(), " and expect:", expect_name.size())};
  }
  // 特征名
  std::set<std::string> miss_name;
  std::set_difference(expect_name.begin(), expect_name.end(), input_name.begin(), input_name.end(), std::inserter(miss_name, miss_name.end()));
  if (!miss_name.empty()) {
    return {PredictStatus::MISS_FEATURE, absl::StrCat("miss ", absl::StrJoin(miss_name, ","))};
  }
  std::unordered_map<std::string, const inference::ModelInferRequest::InferInputTensor*> input_map;
  for (const auto& entry : request.inputs()) {
    input_map.insert(std::make_pair(entry.name(), &entry));
  }

  int64_t item_size = -1;
  for (const auto& feature : model_spec.feature_specs()) {
    auto it = input_map.find(feature.name());
    if (it == input_map.end()) {
      continue;
    }
    auto& tensor_proto = it->second;
    auto status = FeatureCheck(*it->second, feature);
    if (!status.Ok()) {
      return status;
    }
    if (item_size == -1) {
      item_size = tensor_proto->shape(0);
    } else if (item_size != tensor_proto->shape(0)) {
      return {PredictStatus::ITEM_ERROR, "item size not equal"};
    }
  }
  return {PredictStatus::OK};
}
PredictStatus FeatureChecker::FeatureCheck(const inference::ModelInferRequest_InferInputTensor& tensor_proto, const inference::FeatureSpec& feature) {
  // 特征类型
  if (tensor_proto.datatype() != feature.dtype()) {
    return {PredictStatus::FEATURE_TYPE_ERROR,
            absl::StrCat(feature.name(), " input:", inference::DataType_Name(tensor_proto.datatype()), " and expect:", inference::DataType_Name(feature.dtype()))};
  }
  // 特征维度
  if (tensor_proto.shape_size() != feature.shape_size()+1) {
    return {PredictStatus::SHAPE_ERROR, absl::StrCat(
        feature.name(), " input:", tensor_proto.shape_size(), " and expect:", feature.shape_size())};
  }
  std::vector<int64_t> shape;
  shape.reserve(feature.shape_size());
  for (int i=1;i<tensor_proto.shape_size();++i) {
    shape.push_back(tensor_proto.shape(i));
  }
  for (int i=0;i<shape.size();++i) {
    if (shape[i] != feature.shape(i)) {
      return {PredictStatus::SHAPE_ERROR, absl::StrCat(
          feature.name(), " input:", absl::StrJoin(shape, ","), " and expect:", absl::StrJoin(feature.shape(), ","))};
    }
  }
  int64_t expect_size = 1;
  for (const auto& dim : tensor_proto.shape()) {
    expect_size *= dim;
  }
  int64_t input_size = 0;
  switch (tensor_proto.datatype()) {
    case inference::DT_FLOAT:{
      input_size = tensor_proto.contents().fp32_contents_size();
      break;
    }
    case inference::DT_DOUBLE:{
      input_size = tensor_proto.contents().fp64_contents_size();
      break;
    };
    case inference::DT_INT32: {
      input_size = tensor_proto.contents().int_contents_size();
      break;
    }
    case inference::DT_UINT32: {
      input_size = tensor_proto.contents().uint_contents_size();
      break;
    }
    case inference::DT_INT64: {
      input_size = tensor_proto.contents().int64_contents_size();
      break;
    }
    case inference::DT_UINT64: {
      input_size = tensor_proto.contents().uint64_contents_size();
      break;
    }
    default:break;
  }
  if (input_size != expect_size) {
    return {PredictStatus::SHAPE_ERROR, absl::StrCat(feature.name(), " input:", input_size, " and expect:", expect_size)};
  }
  return {PredictStatus::OK};
}
}


