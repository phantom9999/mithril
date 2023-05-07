#include "stop_watch.h"
#include <absl/time/clock.h>

namespace torch::serving {

StopWatch::StopWatch() : check_point_(absl::Now()) {

}
void StopWatch::Reset() {
  check_point_ = absl::Now();
}
int64_t StopWatch::Elapsed() {
  return absl::ToInt64Microseconds(absl::Now() - check_point_);
}
int64_t StopWatch::Current() {
  return ToUnixMicros(check_point_);
}
}

