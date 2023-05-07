#include <fstream>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "mock.pb.h"
#include "kserve_predict_v2.pb.h"
#include "utils/pbtext.h"

DEFINE_string(input, "../example/request_2.json", "");
DEFINE_string(output, "warmup.bin", "");

void Trans(const torch::serving::MockRequest& mock_request, inference::ModelInferRequest* request) {
  auto* tensor_proto = request->add_inputs();
  tensor_proto->set_name("img");
  tensor_proto->set_datatype(inference::DT_FLOAT);
  tensor_proto->mutable_shape()->Add(mock_request.shape().begin(), mock_request.shape().end());
  tensor_proto->mutable_contents()->mutable_fp32_contents()->Add(mock_request.data().begin(), mock_request.data().end());
}


int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  torch::serving::MockRequest mock_request;
  torch::serving::ReadPbJson(FLAGS_input, &mock_request);

  inference::ModelInferRequest request;
  Trans(mock_request, &request);

  torch::serving::WritePbBin(FLAGS_output, request);
  LOG(INFO) << FLAGS_input << " -> " << FLAGS_output;
  return 0;
}
