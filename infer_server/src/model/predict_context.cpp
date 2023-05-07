#include "predict_context.h"
#include <absl/time/clock.h>
#include <absl/strings/str_format.h>

namespace torch::serving {

PredictContext::PredictContext(const inference::ModelInferRequest* request, inference::ModelInferResponse* response) : request_(request), response_(response) {
  time_state_.before_queue = absl::ToUnixMicros(absl::Now());
}
bool PredictContext::Check() {
  return request_ != nullptr && response_ != nullptr;
}


std::string TimeState::ToString() {
  if (before_queue == 0 || before_check == 0 || after_check == 0 || before_pack == 0 || before_predict == 0 || before_unpack == 0 || after_unpack == 0) {
    return "miss time state";
  }
  return absl::StrFormat("all:%dus; queue:%dus; check:%dus; pack:%dus; predict:%dus; unpack:%dus",
                         after_unpack - before_queue,
                         before_queue - before_pack - (after_check - before_check),
                         after_check - before_check,
                         before_predict - before_pack,
                         before_unpack - before_predict,
                         after_unpack - before_unpack
                         );
}
}

