#include "common/timer.h"

Timer::Timer() : start_(std::chrono::steady_clock::now()){

}
uint64_t Timer::UsCost() {
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
}
uint64_t Timer::MsCost() {
  auto end = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
}


