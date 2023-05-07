#define BOOST_TEST_MODULE torch
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <google/protobuf/util/json_util.h>
#include <glog/logging.h>
#include "mock.pb.h"
#include "utils/pbtext.h"
#include "kserve_predict_v2.pb.h"


BOOST_AUTO_TEST_CASE(request_merge) {
  std::string filename = "../../example/dump.json";
  std::ifstream reader(filename);
  inference::ModelInferRequest request;
  std::vector<int32_t> results;
  std::string line;

  auto* input = request.add_inputs();
  input->set_name("img");
  input->set_datatype(inference::DT_FLOAT);
  while (std::getline(reader, line)) {
    torch::serving::MockRequest mock_request;
    auto status = google::protobuf::util::JsonStringToMessage(line, &mock_request);
    if (!status.ok()) {
      continue;
    }
    results.push_back(mock_request.result());
    input->mutable_contents()->mutable_fp32_contents()->Add(mock_request.data().begin(), mock_request.data().end());
  }
  input->add_shape(results.size());
  input->add_shape(1);
  input->add_shape(28);
  input->add_shape(28);
  LOG(INFO) << "merge " << results.size();

  std::string merge_filename = "merge.bin";
  torch::serving::WritePbBin(merge_filename, request);
}




