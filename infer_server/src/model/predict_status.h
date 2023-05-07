#pragma once

#include <string>
#include <utility>

namespace torch::serving {


class PredictStatus {
 public:
  enum Status {
    OK = 0,
    MISS_FEATURE,
    SHAPE_ERROR,
    ITEM_ERROR,
    NULLPTR,
    PREDICT_ERROR,
    RESULT_TYPE_ERROR,
    FEATURE_TYPE_ERROR,
    FEATURE_SIZE_ERROR,
    MISS_SERVABLE,
    RESULT_SIZE_ERROR,
  };

  PredictStatus() = default;
  PredictStatus(Status status): status_(status) { }
  PredictStatus(Status status, std::string msg): status_(status), msg_(std::move(msg)) { }

  bool Ok() {
    return status_ == Status::OK;
  }

  const std::string& Message() {
    return msg_;
  }

 private:
  Status status_{Status::OK};
  std::string msg_;
};

}
