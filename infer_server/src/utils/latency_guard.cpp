#include "latency_guard.h"

#include <absl/time/clock.h>
#include <prometheus/summary.h>

namespace torch::serving {

LatencyGuard::LatencyGuard(prometheus::Summary *summary) : begin_(absl::Now()), summary_{summary} {

}
LatencyGuard::~LatencyGuard() {
  if (summary_ == nullptr) {
    return;
  }
  summary_->Observe(absl::ToDoubleMicroseconds(absl::Now() - begin_));
}

}


