#pragma once

#include <memory>
#include <vector>
#include <any>
#include <enum.h>

BETTER_ENUM(SessionType, uint32_t,
            REQUEST,
            RESPONSE);



template<typename Class>
struct is_shared_ptr : public std::false_type {};

template<typename Class>
struct is_shared_ptr<std::shared_ptr<Class>> : public std::true_type {};

template<typename Class>
constexpr bool is_sptr_v = is_shared_ptr<Class>::value;


class Session {
 public:
  Session();

  template<typename Class, typename = std::enable_if_t<std::is_pointer_v<Class> || is_sptr_v<Class>>>
  void set(SessionType type, Class data) {
    if (content_[+type] != nullptr) {
      // 已经有数据, 不在添加数据
      return;
    }
    content_[+type] = std::make_shared<std::any>(data);
  }

  template<typename Class, typename = std::enable_if_t<std::is_pointer_v<Class> || is_sptr_v<Class>>>
  Class get(SessionType type) {
    if (content_[+type] == nullptr) {
      return nullptr;
    }
    try {
      return std::any_cast<Class>(*content_[+type]);
    } catch (std::bad_any_cast const &) {
      return nullptr;
    }
  }

  void reset();

 private:
  std::vector<std::shared_ptr<std::any>> content_;
};
