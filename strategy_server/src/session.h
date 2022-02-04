#pragma once

#include <memory>
#include <vector>
#include <any>
#include <enum.h>

BETTER_ENUM(SessionType, uint32_t,
            REQUEST,
            RESPONSE);



class Session {
 public:
  Session();
  void set(SessionType type, std::any data);

  template<typename aClass>
  aClass get(SessionType type) {
    if (content_[+type] == nullptr) {
      return nullptr;
    }
    try {
      return std::any_cast<aClass>(*content_[+type]);
    } catch (std::bad_any_cast& e) {
      return nullptr;
    }
  }

  void reset();

 private:
  std::vector<std::shared_ptr<std::any>> content_;
};
