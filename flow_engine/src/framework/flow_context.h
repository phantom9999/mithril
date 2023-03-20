#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <type_traits>
#include <boost/any.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>

namespace flow {

template<typename T> struct is_shared_ptr_helper : public std::false_type { };

template <typename T>
struct is_shared_ptr_helper<std::shared_ptr<T>> : public std::true_type { };

template <typename T>
struct is_shared_ptr : public is_shared_ptr_helper<typename std::remove_cv<T>::type> { };

template<typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;



class FlowContext {
 public:
  explicit FlowContext(uint64_t size = 10) : session_(size) { }

  template<typename Type>
  void Put(int index, Type data) {
    static_assert(is_shared_ptr_v<Type> || std::is_pointer_v<Type>, "type must be row point or shared_ptr");
    if (index < 0 || index >= session_.size()) {
      return;
    }
    session_[index].store(boost::make_shared<boost::any>(data));
  }

  template<typename Type>
  Type Get(int index) {
    static_assert(is_shared_ptr_v<Type> || std::is_pointer_v<Type>, "type must be row point or shared_ptr");
    if (index < 0 || index >= session_.size()) {
      return nullptr;
    }
    auto item = session_[index].load();
    try {
      return boost::any_cast<Type>(*item);
    } catch (const boost::bad_any_cast& e) {
      std::cout << e.what() << std::endl;
      return nullptr;
    }
  }

 private:
  std::vector<boost::atomic_shared_ptr<boost::any>> session_;
};

using FlowContextPtr = std::shared_ptr<FlowContext>;

}
