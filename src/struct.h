#pragma once

#include "bit.h"

#define CH_STRUCT_SIZE_EACH(i, x) \
  ch::internal::identity_t<CH_PAIR_L(x)>::bitcount

#define CH_STRUCT_SIZE(...) \
  (CH_FOR_EACH(CH_STRUCT_SIZE_EACH, CH_SEP_PLUS, __VA_ARGS__))

#define CH_STRUCT_SIM_FIELD(i, x) \
  typename CH_PAIR_L(x)::sim_type CH_PAIR_R(x)

#define CH_STRUCT_FIELD(i, x) \
  typename ch::internal::identity_t<CH_PAIR_L(x)>::value_type CH_PAIR_R(x)

#define CH_STRUCT_CONST_FIELD(i, x) \
  typename ch::internal::identity_t<CH_PAIR_L(x)>::const_type CH_PAIR_R(x)

#define CH_STRUCT_COPY_CTOR_APPLY(i, x) \
  CH_PAIR_R(x)(__rhs__.CH_PAIR_R(x))

#define CH_STRUCT_MOVE_CTOR_APPLY(i, x) \
  CH_PAIR_R(x)(std::move(__rhs__.CH_PAIR_R(x)))

#define CH_STRUCT_FIELD_CTOR_TMPL(i, x) \
  typename __T##i

#define CH_STRUCT_FIELD_CTOR_REQUIRES(i, x) \
  CH_REQUIRES(ch::internal::is_cast_convertible< \
    __T##i, typename ch::internal::identity_t<CH_PAIR_L(x)>::value_type>::value)

#define CH_STRUCT_FIELD_CTOR_ARGS(i, x) \
  const __T##i& CH_CONCAT(_,CH_PAIR_R(x))

#define CH_STRUCT_FIELD_CTOR_APPLY(i, x) \
  CH_PAIR_R(x)(CH_CONCAT(_,CH_PAIR_R(x)))

#define CH_STRUCT_MOVE_ASSIGN(i, x) \
  this->CH_PAIR_R(x) = std::move(__rhs__.CH_PAIR_R(x))

#define CH_STRUCT_CLONE(i, x) \
  CH_PAIR_R(x).clone()


#define CH_STRUCT_MEMBER2(field, dst_offset, length, size, func) \
  if (dst_offset < size) { \
    size_t __len__ = std::min<size_t>(length, size - dst_offset); \
    func(field, dst_offset, __len__); \
    length -= __len__; \
    if (0 == length) \
      return; \
    dst_offset = size; \
  } \
  dst_offset -= size

#define CH_STRUCT_MEMBER3(field, dst_offset, src_offset, length, size, func) \
  if (dst_offset < size) { \
    size_t __len__ = std::min<size_t>(length, size - dst_offset); \
    func(field, dst_offset, src_offset, __len__); \
    length -= __len__; \
    if (0 == length) \
      return; \
    src_offset += __len__; \
    dst_offset = size; \
  } \
  dst_offset -= size

#define CH_STRUCT_MEMBER_READ_DATA(field, offset, length) \
  ch::internal::read_lnode(field, __inout__, offset, length)

#define CH_STRUCT_MEMBER_WRITE_DATA(field, dst_offset, src_offset, length) \
  ch::internal::write_lnode(field, dst_offset, __in__, src_offset, length)

#define CH_STRUCT_MEMBER_READ_BYTES(field, dst_offset, src_offset, length) \
  ch::internal::read_bytes(field, dst_offset, __out__, __out_cbsize__, src_offset, length)

#define CH_STRUCT_MEMBER_WRITE_BYTES(field, dst_offset, src_offset, length) \
  ch::internal::write_bytes(field, dst_offset, __in__, __in_cbsize__, src_offset, length)

#define CH_STRUCT_READ_DATA(i, x) \
  CH_STRUCT_MEMBER2(CH_PAIR_R(x), __offset__, __length__, \
    ch::internal::identity_t<CH_PAIR_L(x)>::bitcount, CH_STRUCT_MEMBER_READ_DATA)
          
#define CH_STRUCT_WRITE_DATA(i, x) \
  CH_STRUCT_MEMBER3(CH_PAIR_R(x), __dst_offset__, __src_offset__, __length__, \
    ch::internal::identity_t<CH_PAIR_L(x)>::bitcount, CH_STRUCT_MEMBER_WRITE_DATA)

#define CH_STRUCT_READ_BYTES(i, x) \
  CH_STRUCT_MEMBER3(CH_PAIR_R(x), __dst_offset__, __src_offset__, __length__, \
    ch::internal::identity_t<CH_PAIR_L(x)>::bitcount, CH_STRUCT_MEMBER_READ_BYTES)

#define CH_STRUCT_WRITE_BYTES(i, x) \
  CH_STRUCT_MEMBER3(CH_PAIR_R(x), __dst_offset__, __src_offset__, __length__, \
    ch::internal::identity_t<CH_PAIR_L(x)>::bitcount, CH_STRUCT_MEMBER_WRITE_BYTES)

#define CH_STRUCT_SIM_IMPL2(struct_name, value_name, ...) \
  class struct_name { \
  public: \
    static constexpr unsigned bitcount = CH_STRUCT_SIZE(__VA_ARGS__); \
    using value_type = value_name; \
    struct_name() {} \
    struct_name(const struct_name& __rhs__) : CH_FOR_EACH(CH_STRUCT_COPY_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
    struct_name(struct_name&& __rhs__) : CH_FOR_EACH(CH_STRUCT_MOVE_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
    template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), \
              CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
    explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__)) \
      : CH_FOR_EACH(CH_STRUCT_FIELD_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
    template <typename U, CH_REQUIRES(is_sim_type<U>::value)> \
    explicit struct_name(const U& __rhs__) : \
    template <typename __T__, CH_REQUIRES(ch::internal::is_scalar<__T__>::value)> \
    explicit struct_name(__T__ __rhs__) { this->assign(__rhs__); } \
    struct_name& operator=(const struct_name& __rhs__) { \
      CH_FOR_EACH(CH_STRUCT_COPY_ASSIGN, CH_SEP_SEMICOLON, __VA_ARGS__); \
      return *this; \
    } \
    struct_name& operator=(struct_name&& __rhs__) { \
      CH_FOR_EACH(CH_STRUCT_MOVE_ASSIGN, CH_SEP_SEMICOLON, __VA_ARGS__); \
      return *this; \
    } \
    CH_FOR_EACH(CH_STRUCT_SIM_FIELD, CH_SEP_SEMICOLON, __VA_ARGS__); \
  protected: \
    void read_bytes(uint32_t __dst_offset__, void* __out__, uint32_t __out_cbsize__, uint32_t __src_offset__, uint32_t __length__) const { \
      CH_FOR_EACH(CH_STRUCT_READ_BYTES, CH_SEP_SEMICOLON, __VA_ARGS__); \
    } \
    void write_bytes(uint32_t __dst_offset__, const void* __in__, uint32_t __in_cbsize__, uint32_t __src_offset__, uint32_t __length__) { \
      CH_FOR_EACH(CH_STRUCT_WRITE_BYTES, CH_SEP_SEMICOLON, __VA_ARGS__); \
    } \
  }

#define CH_STRUCT_BODY_IMPL2(struct_name, assignment_body, field_body, ...) \
  struct_name() {} \
  struct_name(const struct_name& __rhs__) : \
    CH_FOR_EACH(CH_STRUCT_COPY_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(struct_name&& __rhs__) : \
    CH_FOR_EACH(CH_STRUCT_MOVE_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(const ch_bitbase<struct_name::bitcount>& __rhs__) { \
    base::assign(__rhs__); \
  } \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__)) \
    : CH_FOR_EACH(CH_STRUCT_FIELD_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
  template <typename __T__, CH_REQUIRES(ch::internal::is_scalar<__T__>::value)> \
  explicit struct_name(__T__ __rhs__) { base::assign(__rhs__); } \
  assignment_body(struct_name, __VA_ARGS__) \
  const auto clone() const { \
    return value_type(CH_REVERSE_FOR_EACH(CH_STRUCT_CLONE, CH_SEP_COMMA, __VA_ARGS__)); \
  } \
  CH_FOR_EACH(field_body, CH_SEP_SEMICOLON, __VA_ARGS__); \
protected: \
  void read_lnode(ch::internal::nodelist& __inout__, size_t __offset__, size_t __length__) const { \
    CH_FOR_EACH(CH_STRUCT_READ_DATA, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  void write_lnode(size_t __dst_offset__, const ch::internal::nodelist& __in__, size_t __src_offset__, size_t __length__) { \
    CH_FOR_EACH(CH_STRUCT_WRITE_DATA, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  void read_bytes(uint32_t __dst_offset__, void* __out__, uint32_t __out_cbsize__, uint32_t __src_offset__, uint32_t __length__) const override { \
    CH_FOR_EACH(CH_STRUCT_READ_BYTES, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  void write_bytes(uint32_t __dst_offset__, const void* __in__, uint32_t __in_cbsize__, uint32_t __src_offset__, uint32_t __length__) override { \
    CH_FOR_EACH(CH_STRUCT_WRITE_BYTES, CH_SEP_SEMICOLON, __VA_ARGS__); \
  }

#define CH_STRUCT_BODY_IMPL3(struct_name, assignment_body, field_body, ...) \
  struct_name() {} \
  struct_name(const struct_name& __rhs__) \
    : __parent_type__(__rhs__), CH_FOR_EACH(CH_STRUCT_COPY_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(struct_name&& __rhs__) \
    : __parent_type__(std::move(__rhs__))\
    , CH_FOR_EACH(CH_STRUCT_MOVE_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
  struct_name(const ch_bitbase<struct_name::bitcount>& __rhs__) { base::assign(__rhs__); } \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), typename... __Ts__, \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), __Ts__&&... __args__) \
    : __parent_type__(__args__...)\
    , CH_FOR_EACH(CH_STRUCT_FIELD_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
  template <typename __T__, CH_REQUIRES(ch::internal::is_scalar<__T__>::value)> \
  explicit struct_name(__T__ __rhs__) { base::assign(__rhs__); } \
  assignment_body(struct_name, __VA_ARGS__) \
  const auto clone() const { \
    return value_type(CH_REVERSE_FOR_EACH(CH_STRUCT_CLONE, CH_SEP_COMMA, __VA_ARGS__), __parent_type__::clone()); \
  } \
  CH_FOR_EACH(field_body, CH_SEP_SEMICOLON, __VA_ARGS__); \
protected: \
  template <CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_TMPL, CH_SEP_COMMA, __VA_ARGS__), \
            CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_REQUIRES, CH_SEP_COMMA, __VA_ARGS__)> \
  explicit struct_name(CH_REVERSE_FOR_EACH(CH_STRUCT_FIELD_CTOR_ARGS, CH_SEP_COMMA, __VA_ARGS__), const __parent_type__& __parent__) \
    : __parent_type__(__parent__)\
    , CH_FOR_EACH(CH_STRUCT_FIELD_CTOR_APPLY, CH_SEP_COMMA, __VA_ARGS__) {} \
  void read_lnode(ch::internal::nodelist& __inout__, size_t __offset__, size_t __length__) const { \
    CH_STRUCT_MEMBER2(reinterpret_cast<const __parent_type__&>(*this), __offset__, __length__, __parent_type__::bitcount, CH_STRUCT_MEMBER_READ_DATA); \
    CH_FOR_EACH(CH_STRUCT_READ_DATA, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  void write_lnode(size_t __dst_offset__, const ch::internal::nodelist& __in__, size_t __src_offset__, size_t __length__) { \
    CH_STRUCT_MEMBER3(reinterpret_cast<__parent_type__&>(*this), __dst_offset__, __src_offset__, __length__, __parent_type__::bitcount, CH_STRUCT_MEMBER_WRITE_DATA); \
    CH_FOR_EACH(CH_STRUCT_WRITE_DATA, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  void read_bytes(uint32_t __dst_offset__, void* __out__, uint32_t __out_cbsize__, uint32_t __src_offset__, uint32_t __length__) const override { \
    CH_STRUCT_MEMBER3(reinterpret_cast<const __parent_type__&>(*this), __dst_offset__, __src_offset__, __length__, __parent_type__::bitcount, CH_STRUCT_MEMBER_READ_BYTES); \
    CH_FOR_EACH(CH_STRUCT_READ_BYTES, CH_SEP_SEMICOLON, __VA_ARGS__); \
  } \
  void write_bytes(uint32_t __dst_offset__, const void* __in__, uint32_t __in_cbsize__, uint32_t __src_offset__, uint32_t __length__) override { \
    CH_STRUCT_MEMBER3(reinterpret_cast<__parent_type__&>(*this), __dst_offset__, __src_offset__, __length__, __parent_type__::bitcount, CH_STRUCT_MEMBER_WRITE_BYTES); \
    CH_FOR_EACH(CH_STRUCT_WRITE_BYTES, CH_SEP_SEMICOLON, __VA_ARGS__); \
  }

#define CH_STRUCT_READONLY_IMPL(struct_name, ...) \
  CH_BIT_READONLY_INTERFACE(struct_name) \

#define CH_STRUCT_WRITABLE_IMPL(struct_name, ...) \
  struct_name(const const_type& __rhs__) : struct_name(reinterpret_cast<const base&>(__rhs__)) {} \
  struct_name& operator=(const struct_name& __rhs__) { \
    base::assign(__rhs__); \
    return *this; \
  } \
  struct_name& operator=(struct_name&& __rhs__) { \
    CH_FOR_EACH(CH_STRUCT_MOVE_ASSIGN, CH_SEP_SEMICOLON, __VA_ARGS__); \
    return *this; \
  } \
  CH_BIT_WRITABLE_INTERFACE(struct_name)

#define CH_STRUCT_IMPL2(struct_name, ...) \
  class struct_name : public virtual ch_bitbase<CH_STRUCT_SIZE(__VA_ARGS__)> { \
  private: \
    CH_STRUCT_SIM_IMPL2(__sim_type__, struct_name, __VA_ARGS__); \
    class __const_type__ : public virtual ch_bitbase<CH_STRUCT_SIZE(__VA_ARGS__)> { \
    public: \
      using base = ch_bitbase<__const_type__::bitcount>; \
      using value_type = struct_name; \
      using const_type = __const_type__; \
      using sim_type   = __sim_type__; \
      CH_STRUCT_BODY_IMPL2(__const_type__, CH_STRUCT_READONLY_IMPL, CH_STRUCT_CONST_FIELD, __VA_ARGS__) \
    }; \
  public: \
    using base = ch_bitbase<struct_name::bitcount>; \
    using value_type = struct_name; \
    using const_type = __const_type__; \
    using sim_type   = __sim_type__; \
    CH_STRUCT_BODY_IMPL2(struct_name, CH_STRUCT_WRITABLE_IMPL, CH_STRUCT_FIELD, __VA_ARGS__) \
  }

#define CH_STRUCT_IMPL3(struct_name, parent, ...) \
  class struct_name : public virtual ch_bitbase<parent::bitcount + CH_STRUCT_SIZE(__VA_ARGS__)>, public parent { \
  private: \
    class __const_type__ : public virtual ch_bitbase<parent::bitcount + CH_STRUCT_SIZE(__VA_ARGS__)>, public parent::const_type { \
    private: \
      using __parent_type__ = typename parent::const_type; \
    public: \
      using base = ch_bitbase<parent::bitcount + CH_STRUCT_SIZE(__VA_ARGS__)>; \
      using base::bitcount; \
      using value_type = struct_name; \
      using const_type = __const_type__; \
      CH_STRUCT_BODY_IMPL3(__const_type__, CH_STRUCT_READONLY_IMPL, CH_STRUCT_CONST_FIELD, __VA_ARGS__) \
    }; \
    using __parent_type__ = parent; \
  public: \
    using base = ch_bitbase<parent::bitcount + CH_STRUCT_SIZE(__VA_ARGS__)>; \
    using base::bitcount; \
    using value_type = struct_name; \
    using const_type = __const_type__; \
    CH_STRUCT_BODY_IMPL3(struct_name, CH_STRUCT_WRITABLE_IMPL, CH_STRUCT_FIELD, __VA_ARGS__) \
  }

#define CH_STRUCT2(name, body) \
  CH_STRUCT_IMPL2(name, CH_REM body)

#define CH_STRUCT3(name, parent, body) \
  CH_STRUCT_IMPL3(name, parent, CH_REM body)

#define GET_STRUCT(_1, _2, _3, NAME, ...) NAME
#define CH_STRUCT(...) GET_STRUCT(__VA_ARGS__, CH_STRUCT3, CH_STRUCT2)(__VA_ARGS__)
