#pragma once

#include "if.h"

namespace ch {
namespace internal {

class switch_impl {
public:  
  ~switch_impl();
  
  using func_t = std::function<void ()>;
  
  void eval(const lnode& pred, func_t func);

  void eval(func_t func);
 
protected:
  switch_impl(const lnode& key);
 
  lnode key_;
  
  template <typename K> friend class switch_t;
};

template <typename K>
class switch_t {
public:

  switch_t(const lnode& key) : impl_(key) {}
  
  template <typename T, typename Func,
            CH_REQUIRES(is_cast_convertible<K, T>::value)>
  switch_t& case_(const T& value, const Func& func) {
    impl_.eval(get_lnode<T, K::bitsize>(value), to_function_t<Func>(func));
    return *this;
  }
  
  template <typename Func>
  void default_(const Func& func) {
    impl_.eval(to_function_t<Func>(func));
  }
  
protected:
  
  switch_impl impl_;
};

template <typename K,
          CH_REQUIRES(is_bit_convertible<K>::value)>
auto ch_switch(const K& key) {
  return switch_t<K>(get_lnode(key));
}

}
}

#define CH_SWITCH_BODY(body)    body
#define CH_SWITCH(key)          ch_switch(key) CH_SWITCH_BODY
#define CH_CASE_BODY(value)     value })
#define CH_CASE(pred)           .case_(pred, [&](){ CH_CASE_BODY
#define CH_DEFAULT(value)       .default_([&](){ value })
