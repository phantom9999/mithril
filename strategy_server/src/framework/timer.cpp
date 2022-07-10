#include "timer.h"
#include <glog/logging.h>

Timer::Timer(const std::string& msg) : msg_(msg), begin_{std::chrono::system_clock::now()} {

}

Timer::~Timer() {
  using std::chrono::microseconds;
  using std::chrono::duration_cast;
  using std::chrono::system_clock;
  auto time_span = duration_cast<microseconds>(system_clock::now() - begin_);
  LOG(INFO) << msg_ << "; cost " << time_span.count() << " us";
}
