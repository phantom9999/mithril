#include "session.h"

Session::Session() {
  reset();
}
void Session::set(SessionType type, std::any data) {
  if (content_[+type] != nullptr) {
    // 已经有数据, 不在添加数据
    return;
  }
  content_[+type] = std::make_shared<std::any>(std::move(data));
}
void Session::reset() {
  content_.clear();
  content_.resize(SessionType::_size());
}


