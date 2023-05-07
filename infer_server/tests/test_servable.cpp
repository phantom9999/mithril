#define BOOST_TEST_MODULE torch
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <glog/logging.h>
#include "kserve_predict_v2.pb.h"
#include "servables/servable.h"
#include "servables/torch_servable.h"
#include "mock.pb.h"
#include <google/protobuf/util/json_util.h>
#include "model/predict_context.h"

bool ReadRequest(const std::string& filename, torch::serving::MockRequest* mock_request) {
  std::ifstream reader(filename);
  if (!reader.is_open()) {
    LOG(WARNING) << filename << " not found";
    return false;
  }
  std::string content;
  std::string line;
  while (std::getline(reader, line)) {
    content += line;
  }
  auto status = google::protobuf::util::JsonStringToMessage(content, mock_request);
  if (!status.ok()) {
    LOG(WARNING) << status.message();
    return false;
  }
  return true;
}

void Trans(const torch::serving::MockRequest& mock_request, inference::ModelInferRequest* request) {
  auto* tensor_proto = request->add_inputs();
  tensor_proto->set_name("img");
  tensor_proto->set_datatype(inference::DT_FLOAT);
  tensor_proto->mutable_shape()->Add(mock_request.shape().begin(), mock_request.shape().end());
  tensor_proto->mutable_contents()->mutable_fp32_contents()->Add(mock_request.data().begin(), mock_request.data().end());
}


BOOST_AUTO_TEST_CASE(servable) {
  std::string path = "../data/model_1/20220101";
  torch::serving::TorchServable servable;
  BOOST_ASSERT(servable.Init(path));

  std::string filename = "../../example/request_2.json";
  torch::serving::MockRequest mock_request;
  BOOST_ASSERT(ReadRequest(filename, &mock_request));

  inference::ModelInferRequest request;
  inference::ModelInferResponse response;
  Trans(mock_request, &request);
  // [ -7.2613,  -4.9868,  -2.2618,  -9.7659,   3.9148, -17.1948,   1.5820,
  //         -22.7648,  -4.1574, -14.1905]
  auto context = std::make_shared<torch::serving::PredictContext>(&request, &response);
  BOOST_ASSERT(servable.Predict(context).Ok());
  LOG(INFO) << "expect " << mock_request.result();
  LOG(INFO) << response.DebugString();
  LOG(INFO) << context->time_state_.ToString();
}



