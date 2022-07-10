#pragma once

#include <chrono>
#include <string>

class Timer {
public:
  explicit Timer(const std::string& msg);
  ~Timer();

private:
  std::string msg_;
  std::chrono::system_clock::time_point begin_;
};
