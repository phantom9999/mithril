#pragma once

#include <absl/time/time.h>

namespace torch::serving {

class StopWatch {
 public:
  StopWatch();

  void Reset();

  int64_t Elapsed();

  int64_t Current();

 private:
  absl::Time check_point_;
};

}





