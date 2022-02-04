#include "session.h"

Session::Session() {
  reset();
}

void Session::reset() {
  content_.clear();
  content_.resize(SessionType::_size());
}


