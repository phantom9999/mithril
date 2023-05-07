#pragma once

#include <unordered_map>
#include <string>

#include <absl/time/time.h>


namespace prometheus {
class Summary;
}

namespace torch::serving {

class LatencyGuard {
 public:
  explicit LatencyGuard(prometheus::Summary* summary);

  ~LatencyGuard();

 private:
  absl::Time begin_;
  prometheus::Summary* summary_;
};

}
