#pragma once

#include "logic.h"

#define CH_STRUCT_SIZE_EACH(i, x) \
  ch_width_v<ch::internal::identity_t<CH_PAIR_L(x)>>

#define CH_STRUCT_SIZE(...) \
  (CH_FOR_EACH(CH_STRUCT_SIZE_EACH, CH_SEP_PLUS, __VA_ARGS__))

#define CH_STRUCT_FIELD_OFFSET(i, x) \
  CH_CONCAT(__field_offset, CH_INC(i)) = __field_offset##i + ch_width_v<ch::internal::identity_t<CH_PAIR_L(x)>>

#define CH_STRUCT_SCALAR_FIELD(i, x) \
  ch_scalar_t<ch::internal::identity_t<CH_PAIR_L(x)>> CH_PAIR_R(x)

#define CH_STRUCT_LOGIC_FIELD(i, x) \
  ch_logic_t<ch::internal::identity_t<CH_PAIR_L(x)>> CH_PAIR_R(x)

#define CH_STRUCT_SCALAR_CTOR(i, x) \
  CH_PAIR_R(x)(std::make_shared<ch::internal::type_buffer_t<traits>>( \
    ch_width_v<ch::internal::identity_t<CH_PAIR_L(x)>>, buffer, __field_offset##i))

#define CH_STRUCT_LOGIC_CTOR(i, x) \
  CH_PAIR_R(x)(std::make_shared<ch::internal::type_buffer_t<traits>>( \
    ch_width_v<ch::internal::identity_t<CH_PAIR_L(x)>>, buffer, __field_offset##i, \
    CH_STRINGIZE(CH_PAIR_R(x))))

#define CH_STRUCT_MOVE_CTOR(i, x) \
  CH_PAIR_R(x)(std::move(rhs.CH_PAIR_R(x)))

#define CH_STRUCT_FIELD_CTOR_TMPL(i, x) \
  typename __T##i

#define CH_STRUCT_FIELD_CTOR_REQUIRES(i, x) \
  CH_REQUIRE_0((std::is_integral_v<__T##i> || std::is_enum_v<__T##i> || ch::internal::has_bitwidth_v<__T##i>) \
           && std::is_constructible_v<decltype(CH_PAIR_R(x)), __T##i>)

#define CH_STRUCT_FIELD_CTOR_ARGS(i, x) \
  const __T##i& CH_CONCAT(_,CH_PAIR_R(x))

#define CH_STRUCT_FIELD_CTOR_BODY(i, x) \
  CH_CONCAT(_,CH_PAIR_R(x))

#define CH_STRUCT_INIT_FIELD(i, x) \
  ch::internal::type_accessor_t<traits>::copy(CH_PAIR_R(x), \
    static_cast<ch::internal::aggregate_init_cast_t<decltype(CH_PAIR_R(x)), __T##i>>(CH_CONCAT(_,CH_PAIR_R(x))))

#define CH_STRUCT_GETBUFFER(i, x) \
  return ch::internal::type_accessor_t<traits>::buffer(CH_PAIR_R(x))

#define CH_STRUCT_CLONE(i, x) \
  CH_PAIR_R(x).clone()

#define CH_STRUCT_SCALAR_IMPL2(struct_name, field_body, ...) \
private: \
  enum { __field_offset0 = 0, \
           CH_FOR_EACH(CH_STRUCT_FIELD_OFFSET, CH_SEP_COMMA, __VA_ARGS__) }; \
public: \
  CH_FOR_EACH(field_body, CH_SEP_SEMICOLON, __VA_ARGS__); \
  struct_name(const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer = \
    std::make_shared<ch::internal::type_buffer_t<traits>>(traits::bitwidth)) \
    : CH_FOR_EACH(CH_STRUCT_SCALAR_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(const struct_name& rhs) \
    : struct_name(ch::internal::type_accessor_t<traits>::copy_buffer(rhs)) {} \
  struct_name(struct_name&& rhs) : \
    CH_FOR_EACH(CH_STRUCT_MOVE_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  template <typename __T__, CH_REQUIRE_0(std::is_integral_v<__T__> || std::is_enum_v<__T__>)> \
  explicit struct_name(__T__ rhs) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::bitvector(traits::bitwidth, rhs))) {} \
  explicit struct_name(const ch_scalar<traits::bitwidth>& rhs) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::scalar_accessor::data(rhs))) {} \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__)) \
    : struct_name() { \
    this->init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_BODY, CH_SEP_COMMA, __VA_ARGS__)); \
  } \
protected: \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__)> \
  void init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__)) { \
    CH_REVERSE_FOR_EACH(CH_STRUCT_INIT_FIELD, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() const { \
    CH_STRUCT_GETBUFFER(0, CH_FIRST_ARG(__VA_ARGS__))->source(); \
  } \
  std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() { \
    CH_STRUCT_GETBUFFER(0, CH_FIRST_ARG(__VA_ARGS__))->source(); \
  } \
public:

#define CH_STRUCT_LOGIC_IMPL2(struct_name, name, field_body, ...) \
private: \
  enum { __field_offset0 = 0, \
           CH_FOR_EACH(CH_STRUCT_FIELD_OFFSET, CH_SEP_COMMA, __VA_ARGS__) }; \
public: \
  CH_FOR_EACH(field_body, CH_SEP_SEMICOLON, __VA_ARGS__); \
  struct_name(const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer = \
    std::make_shared<ch::internal::type_buffer_t<traits>>(traits::bitwidth, CH_SRC_LOCATION, CH_STRINGIZE(name))) \
    : CH_FOR_EACH(CH_STRUCT_LOGIC_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(const struct_name& rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(ch::internal::type_accessor_t<traits>::copy_buffer(rhs, sloc, CH_STRINGIZE(name))) {} \
  struct_name(struct_name&& rhs) : \
    CH_FOR_EACH(CH_STRUCT_MOVE_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  template <typename __T__, CH_REQUIRE_0(std::is_integral_v<__T__> || std::is_enum_v<__T__>)> \
  explicit struct_name(__T__ rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::bitvector(traits::bitwidth, rhs), sloc, CH_STRINGIZE(name))) {} \
  explicit struct_name(const ch_bit<traits::bitwidth>& rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(ch::internal::type_accessor_t<traits>::copy_buffer(rhs, sloc, CH_STRINGIZE(name))) {} \
  explicit struct_name(const ch_scalar<traits::bitwidth>& rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::scalar_accessor::data(rhs), sloc, CH_STRINGIZE(name))) {} \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), \
                       const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(traits::bitwidth, sloc, CH_STRINGIZE(name))) { \
    this->init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_BODY, CH_SEP_COMMA, __VA_ARGS__)); \
  } \
protected: \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__)> \
  void init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__)) { \
    CH_REVERSE_FOR_EACH(CH_STRUCT_INIT_FIELD, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() const { \
    CH_STRUCT_GETBUFFER(0, CH_FIRST_ARG(__VA_ARGS__))->source(); \
  } \
  std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() { \
    CH_STRUCT_GETBUFFER(0, CH_FIRST_ARG(__VA_ARGS__))->source(); \
  } \
public:

#define CH_STRUCT_SCALAR_IMPL3(struct_name, field_body, ...) \
private: \
  enum { __field_offset0 = ch_width_v<base>, \
         CH_FOR_EACH(CH_STRUCT_FIELD_OFFSET, CH_SEP_COMMA, __VA_ARGS__) }; \
public: \
  CH_FOR_EACH(field_body, CH_SEP_SEMICOLON, __VA_ARGS__); \
  struct_name(const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer = \
    std::make_shared<ch::internal::type_buffer_t<traits>>(traits::bitwidth)) \
    : base(buffer) \
    , CH_FOR_EACH(CH_STRUCT_SCALAR_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(const struct_name& rhs) \
    : struct_name(ch::internal::type_accessor_t<traits>::copy_buffer(rhs)) {} \
  struct_name(struct_name&& rhs) \
    : base(std::move(rhs))\
    , CH_FOR_EACH(CH_STRUCT_MOVE_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  template <typename __T__, CH_REQUIRE_0(std::is_integral_v<__T__> || std::is_enum_v<__T__>)> \
  explicit struct_name(__T__ rhs) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::bitvector(traits::bitwidth, rhs))) {} \
  explicit struct_name(const ch_scalar<traits::bitwidth>& rhs) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::scalar_accessor::data(rhs))) {} \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), typename... __Ts__, \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), __Ts__&&... args) \
    : struct_name() { \
    this->init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_BODY, CH_SEP_COMMA, __VA_ARGS__), args...); \
  } \
protected: \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), typename... __Ts__> \
  void init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), __Ts__&&... args) {\
    base::init_fields(args...); \
    CH_REVERSE_FOR_EACH(CH_STRUCT_INIT_FIELD, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() const { \
    return base::buffer(); \
  } \
  std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() { \
    return base::buffer(); \
  } \
public:

#define CH_STRUCT_LOGIC_IMPL3(struct_name, name, field_body, ...) \
private: \
  enum { __field_offset0 = ch_width_v<base>, \
         CH_FOR_EACH(CH_STRUCT_FIELD_OFFSET, CH_SEP_COMMA, __VA_ARGS__) }; \
public: \
  CH_FOR_EACH(field_body, CH_SEP_SEMICOLON, __VA_ARGS__); \
  struct_name(const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer = \
    std::make_shared<ch::internal::type_buffer_t<traits>>(traits::bitwidth, CH_SRC_LOCATION, CH_STRINGIZE(name))) \
    : base(buffer) \
    , CH_FOR_EACH(CH_STRUCT_LOGIC_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(const struct_name& rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(ch::internal::type_accessor_t<traits>::copy_buffer(rhs, sloc, CH_STRINGIZE(name))) {} \
  struct_name(struct_name&& rhs) \
    : base(std::move(rhs))\
    , CH_FOR_EACH(CH_STRUCT_MOVE_CTOR, CH_SEP_COMMA, __VA_ARGS__) {} \
  template <typename __T__, CH_REQUIRE_0(std::is_integral_v<__T__> || std::is_enum_v<__T__>)> \
  explicit struct_name(__T__ rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::bitvector(traits::bitwidth, rhs), sloc, CH_STRINGIZE(name))) {} \
  explicit struct_name(const ch_bit<traits::bitwidth>& rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(ch::internal::type_accessor_t<traits>::copy_buffer(rhs, sloc, CH_STRINGIZE(name))) {} \
  explicit struct_name(const ch_scalar<traits::bitwidth>& rhs, const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(ch::internal::scalar_accessor::data(rhs), sloc, CH_STRINGIZE(name))) {} \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), \
                       const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(traits::bitwidth, sloc, CH_STRINGIZE(name))) { \
    this->init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_BODY, CH_SEP_COMMA, __VA_ARGS__)); \
  } \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), typename __T__, \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), \
                       const __T__& arg, \
                       const ch::internal::source_location& sloc = CH_SRC_LOCATION) \
    : struct_name(std::make_shared<ch::internal::type_buffer_t<traits>>(traits::bitwidth, sloc, CH_STRINGIZE(name))) { \
    this->init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_BODY, CH_SEP_COMMA, __VA_ARGS__), arg); \
  } \
protected: \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), typename... __Ts__> \
  void init_fields(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), __Ts__&&... args) { \
    base::init_fields(args...); \
    CH_REVERSE_FOR_EACH(CH_STRUCT_INIT_FIELD, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  const std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() const { \
    return base::buffer(); \
  } \
  std::shared_ptr<ch::internal::type_buffer_t<traits>>& buffer() { \
    return base::buffer(); \
  }

#define CH_STRUCT_ASSIGN_IMPL(struct_name) \
public: \
  struct_name& operator=(const struct_name& rhs) { \
    ch::internal::type_accessor_t<traits>::copy(*this, rhs); \
    return *this; \
  } \
  struct_name& operator=(struct_name&& rhs) { \
    ch::internal::type_accessor_t<traits>::move(*this, std::move(rhs)); \
    return *this; \
  }

#define CH_STRUCT_SCALAR_FRIENDS_IMPL(enum_name) \
protected: \
  friend class ch::internal::scalar_accessor;

#define CH_STRUCT_LOGIC_FRIENDS_IMPL(enum_name) \
protected: \
  friend class ch::internal::logic_accessor;

#define CH_STRUCT_IMPL2(struct_name, ...) \
  class struct_name { \
  private: \
    class __scalar_type__ { \
    public: \
      using traits = ch::internal::scalar_traits<CH_STRUCT_SIZE(__VA_ARGS__), false, __scalar_type__, struct_name>; \
      CH_STRUCT_SCALAR_IMPL2(__scalar_type__, CH_STRUCT_SCALAR_FIELD, __VA_ARGS__) \
      CH_SCALAR_INTERFACE(__scalar_type__) \
      CH_STRUCT_ASSIGN_IMPL(__scalar_type__) \
      CH_STRUCT_SCALAR_FRIENDS_IMPL(__scalar_type__) \
    }; \
  public: \
    using traits = ch::internal::logic_traits<CH_STRUCT_SIZE(__VA_ARGS__), false, struct_name, __scalar_type__>; \
    CH_STRUCT_LOGIC_IMPL2(struct_name, struct_name, CH_STRUCT_LOGIC_FIELD, __VA_ARGS__) \
    CH_LOGIC_INTERFACE(struct_name) \
    CH_STRUCT_ASSIGN_IMPL(struct_name) \
    CH_STRUCT_LOGIC_FRIENDS_IMPL(struct_name) \
  }

#define CH_STRUCT_IMPL3(struct_name, parent, ...) \
  class struct_name : public parent { \
  private: \
    class __scalar_type__ : public ch_scalar_t<parent> { \
    public: \
      using base = ch_scalar_t<parent>; \
      using traits = ch::internal::scalar_traits<ch_width_v<base> + CH_STRUCT_SIZE(__VA_ARGS__), false, __scalar_type__, struct_name>; \
      CH_STRUCT_SCALAR_IMPL3(__scalar_type__, CH_STRUCT_SCALAR_FIELD, __VA_ARGS__) \
      CH_STRUCT_ASSIGN_IMPL(__scalar_type__) \
      CH_SCALAR_INTERFACE(__scalar_type__) \
      CH_STRUCT_SCALAR_FRIENDS_IMPL(__scalar_type__) \
    }; \
  public: \
    using base = parent; \
    using traits = ch::internal::logic_traits<ch_width_v<base> + CH_STRUCT_SIZE(__VA_ARGS__), false, struct_name, __scalar_type__>; \
    CH_STRUCT_LOGIC_IMPL3(struct_name, struct_name, CH_STRUCT_LOGIC_FIELD, __VA_ARGS__) \
    CH_STRUCT_ASSIGN_IMPL(struct_name) \
    CH_LOGIC_INTERFACE(struct_name) \
    CH_STRUCT_LOGIC_FRIENDS_IMPL(struct_name) \
  }

#define CH_STRUCT2(name, body) \
  CH_STRUCT_IMPL2(name, CH_REM body)

#define CH_STRUCT3(name, parent, body) \
  CH_STRUCT_IMPL3(name, parent, CH_REM body)

#define GET_STRUCT(_1, _2, _3, NAME, ...) NAME
#define CH_STRUCT(...) GET_STRUCT(__VA_ARGS__, CH_STRUCT3, CH_STRUCT2, ignore)(__VA_ARGS__)