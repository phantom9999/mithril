#pragma once
#include <utility>
#include <string>
#include <memory>

namespace inference {
class ModelInferRequest;
class ModelInferResponse;
}

namespace torch::serving {

struct TimeState {
  int64_t before_queue{0};
  int64_t after_check{0};
  int64_t before_check{0};
  int64_t before_pack{0};
  int64_t before_predict{0};
  int64_t before_unpack{0};
  int64_t after_unpack{0};
  std::string ToString();
};

class PredictContext {
 public:
  PredictContext(const inference::ModelInferRequest* request, inference::ModelInferResponse* response);
  bool Check();

  const inference::ModelInferRequest* request_;
  inference::ModelInferResponse* response_;
  TimeState time_state_;
};

using PredictContextPtr = std::shared_ptr<PredictContext>;

}
