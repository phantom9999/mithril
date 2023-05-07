#include "item_utils.h"
#include "kserve_predict_v2.pb.h"

namespace torch::serving {

size_t CountItem(const inference::ModelInferRequest& request) {
  for (const auto& entry : request.inputs()) {
    for (const auto& num : entry.shape()) {
      return num;
    }
  }
  return 0;
}


}

