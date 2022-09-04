#pragma once

#include <chrono>

class Timer {
 public:
  Timer();
  uint64_t UsCost();
  uint64_t MsCost();

 private:
  const std::chrono::time_point<std::chrono::steady_clock> start_;
};
