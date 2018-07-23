#pragma once

#define CH_OP_ARY(x)   (x & (0x1 << 5))
#define CH_OP_CLASS(x) (x & (0x7 << 6))

#define CH_OP_TYPE(n, v) op_##n = v,
#define CH_OP_NAME(n, v) #n,
#define CH_OP_INDEX(op)  (op & 0x1f)
#define CH_OP_ENUM(m) \
  m(eq,     0 | op_binary | op_equality | op_symmetric) \
  m(ne,     1 | op_binary | op_equality | op_symmetric) \
  m(lt,     2 | op_binary | op_relational) \
  m(gt,     3 | op_binary | op_relational) \
  m(le,     4 | op_binary | op_relational) \
  m(ge,     5 | op_binary | op_relational) \
  m(inv,    6 | op_unary  | op_bitwise) \
  m(and,    7 | op_binary | op_bitwise | op_symmetric) \
  m(or,     8 | op_binary | op_bitwise | op_symmetric) \
  m(xor,    9 | op_binary | op_bitwise | op_symmetric) \
  m(andr,  10 | op_unary  | op_reduce) \
  m(orr,   11 | op_unary  | op_reduce) \
  m(xorr,  12 | op_unary  | op_reduce) \
  m(shl,   13 | op_binary | op_shift) \
  m(shr,   14 | op_binary | op_shift) \
  m(neg,   15 | op_unary  | op_arithmetic) \
  m(add,   16 | op_binary | op_arithmetic | op_symmetric) \
  m(sub,   17 | op_binary | op_arithmetic) \
  m(mul ,  18 | op_binary | op_arithmetic | op_symmetric) \
  m(div,   19 | op_binary | op_arithmetic) \
  m(mod,   20 | op_binary | op_arithmetic) \
  m(pad,   21 | op_unary  | op_misc)

namespace ch {
namespace internal {

enum op_flags {
  op_unary      = 0 << 5,
  op_binary     = 1 << 5,

  op_equality   = 0 << 6,
  op_relational = 1 << 6,
  op_bitwise    = 2 << 6,
  op_shift      = 3 << 6,
  op_arithmetic = 4 << 6,
  op_reduce     = 5 << 6,
  op_misc       = 6 << 6,

  op_symmetric  = 1 << 9,
};

enum ch_op {
  CH_OP_ENUM(CH_OP_TYPE)
};

const char* to_string(ch_op op);

///////////////////////////////////////////////////////////////////////////////

enum traits_type {
  traits_none      = 0x00,
  traits_logic     = 0x01,
  traits_scalar    = 0x02,
  traits_logic_io  = 0x04,
  traits_scalar_io = 0x08,
  traits_udf       = 0x10,
};

inline constexpr auto operator|(traits_type lsh, traits_type rhs) {
  return traits_type((int)lsh | (int)rhs);
}

enum class ch_direction {
  in    = 0x1,
  out   = 0x2,
  inout = in | out,
};

inline constexpr auto operator|(ch_direction lsh, ch_direction rhs) {
  return ch_direction((int)lsh | (int)rhs);
}

template <typename T>
inline constexpr ch_direction ch_direction_v = std::decay_t<T>::traits::direction;

CH_DEF_SFINAE_CHECK(is_object_type, (T::traits::type & (traits_logic | traits_scalar)));

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename Enable = void>
struct width_value_impl {
  static constexpr uint32_t value = 0;
};

template <typename T>
struct width_value_impl<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
  static constexpr uint32_t value = std::numeric_limits<T>::digits +
                                    std::numeric_limits<T>::is_signed;
};

template <typename T>
struct width_value_impl<T, std::enable_if_t<is_object_type_v<T>>> {
  static constexpr uint32_t value = T::traits::bitwidth;
};

template <typename... Ts>
struct width_impl;

template <typename T>
struct width_impl<T> {
  static constexpr uint32_t value = width_value_impl<T>::value;
};

template <typename T0, typename... Ts>
struct width_impl<T0, Ts...> {
  static constexpr uint32_t value = T0::traits::bitwidth + width_impl<Ts...>::value;
};

template <typename... Ts>
inline constexpr uint32_t ch_width_v = width_impl<std::decay_t<Ts>...>::value;

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename Enable = void>
struct signed_impl {
  static constexpr bool value = false;
};

template <typename T>
struct signed_impl<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
  static constexpr bool value = std::numeric_limits<T>::is_signed;
};

template <typename T>
struct signed_impl<T, std::enable_if_t<is_object_type_v<T>>> {
  static constexpr bool value = T::traits::is_signed;
};

template <typename T>
inline constexpr bool ch_signed_v = signed_impl<std::decay_t<T>>::value;

///////////////////////////////////////////////////////////////////////////////

template <unsigned Bitwidth, bool Signed, typename ScalarType, typename LogicType>
struct scalar_traits {
  static constexpr traits_type type  = traits_scalar;
  static constexpr unsigned bitwidth = Bitwidth;
  static constexpr unsigned is_signed = Signed;
  using scalar_type = ScalarType;
  using logic_type  = LogicType;
};

template <typename T>
using ch_scalar_t = typename std::decay_t<T>::traits::scalar_type;

template <typename T>
inline constexpr bool is_scalar_traits_v = bool_constant_v<(T::type & traits_scalar)>;

CH_DEF_SFINAE_CHECK(is_scalar_only, bool_constant_v<(std::decay_t<T>::traits::type == traits_scalar)>);

CH_DEF_SFINAE_CHECK(is_scalar_type, is_scalar_traits_v<typename std::decay_t<T>::traits>);

///////////////////////////////////////////////////////////////////////////////

template <unsigned Bitwidth, bool Signed, typename LogicType, typename ScalarType>
struct logic_traits {
  static constexpr traits_type type = traits_logic;
  static constexpr unsigned bitwidth = Bitwidth;
  static constexpr unsigned is_signed = Signed;
  using logic_type  = LogicType;
  using scalar_type = ScalarType;
};

template <typename T>
using ch_logic_t = typename std::decay_t<T>::traits::logic_type;

template <typename T>
inline constexpr bool is_logic_traits_v = bool_constant_v<(T::type & traits_logic)>;

CH_DEF_SFINAE_CHECK(is_logic_only, bool_constant_v<(std::decay_t<T>::traits::type == traits_logic)>);

CH_DEF_SFINAE_CHECK(is_logic_type, is_logic_traits_v<typename std::decay_t<T>::traits>);

///////////////////////////////////////////////////////////////////////////////

template <unsigned Bitwidth,
          ch_direction Direction,
          typename ScalarIO,
          typename FlipIO,
          typename LogicIO>
struct scalar_io_traits {
  static constexpr traits_type type  = traits_scalar_io;
  static constexpr unsigned bitwidth = Bitwidth;
  static constexpr ch_direction direction = Direction;
  using scalar_io = ScalarIO;
  using flip_io   = FlipIO;
  using logic_io  = LogicIO;
};

template <unsigned Bitwidth,
          ch_direction Direction,
          typename LogicIO,
          typename FlipIO,
          typename ScalarIO>
struct logic_io_traits {
  static constexpr traits_type type  = traits_logic_io;
  static constexpr unsigned bitwidth = Bitwidth;
  static constexpr ch_direction direction = Direction;
  using logic_io  = LogicIO;
  using flip_io   = FlipIO;
  using scalar_io = ScalarIO;
};

template <ch_direction Direction,
          typename ScalarIO,
          typename FlipIO,
          typename LogicIO,
          typename ScalarType>
struct mixed_scalar_io_traits {
  static constexpr traits_type type = traits_scalar_io | traits_scalar;
  static constexpr unsigned bitwidth = ch_width_v<ScalarType>;
  static constexpr unsigned is_signed = ch_signed_v<ScalarType>;
  static constexpr ch_direction direction = Direction;
  using scalar_io   = ScalarIO;
  using flip_io     = FlipIO;
  using logic_io    = LogicIO;
  using scalar_type = ScalarType;
  using logic_type  = ch_logic_t<ScalarType>;
};

template <ch_direction Direction,
          typename LogicIO,
          typename FlipIO,
          typename ScalarIO,
          typename LogicType>
struct mixed_logic_io_traits {
  static constexpr traits_type type = traits_logic_io | traits_logic;
  static constexpr unsigned bitwidth = ch_width_v<LogicType>;
  static constexpr unsigned is_signed = ch_signed_v<LogicType>;
  static constexpr ch_direction direction = Direction;
  using logic_io    = LogicIO;
  using flip_io     = FlipIO;
  using scalar_io   = ScalarIO;
  using logic_type  = LogicType;
  using scalar_type = ch_scalar_t<LogicType>;
};

template <typename T>
using ch_logic_io = typename std::decay_t<T>::traits::logic_io;

template <typename T>
using ch_scalar_io = typename std::decay_t<T>::traits::scalar_io;

template <typename T>
using ch_flip_io = typename std::decay_t<T>::traits::flip_io;

template <typename T>
inline constexpr bool is_scalar_io_traits_v = bool_constant_v<(T::type & traits_scalar_io)>;

template <typename T>
inline constexpr bool is_logic_io_traits_v = bool_constant_v<(T::type & traits_logic_io)>;

template <typename T>
inline constexpr bool is_io_traits_v = bool_constant_v<(T::type & (traits_scalar_io | traits_logic_io))>;

CH_DEF_SFINAE_CHECK(is_scalar_io, is_scalar_io_traits_v<typename std::decay_t<T>::traits>);

CH_DEF_SFINAE_CHECK(is_logic_io, is_logic_io_traits_v<typename std::decay_t<T>::traits>);

CH_DEF_SFINAE_CHECK(is_io, is_io_traits_v<typename std::decay_t<T>::traits>);

///////////////////////////////////////////////////////////////////////////////

struct non_ch_type {
  struct traits {
    static constexpr traits_type type = traits_none;
    static constexpr unsigned bitwidth = 0;
    static constexpr bool is_signed = false;
  };
};

template <typename T>
using ch_type_t = std::conditional_t<is_object_type_v<T>, T, non_ch_type>;

template <bool resize, typename T0, typename T1>
struct deduce_type_impl {
  using D0 = std::decay_t<T0>;
  using D1 = std::decay_t<T1>;
  using U0 = std::conditional_t<is_object_type_v<D0>, D0, non_ch_type>;
  using U1 = std::conditional_t<is_object_type_v<D1>, D1, non_ch_type>;
  using type = std::conditional_t<(ch_width_v<U0> != 0) && (ch_width_v<U1> != 0),
    std::conditional_t<(ch_width_v<U0> == ch_width_v<U1>) || ((ch_width_v<U0> > ch_width_v<U1>) && resize), U0,
        std::conditional_t<(ch_width_v<U0> < ch_width_v<U1>) && resize, U1, non_ch_type>>,
          std::conditional_t<(ch_width_v<U0> != 0), U0, U1>>;
};

template <bool resize, typename... Ts>
struct deduce_type;

template <bool resize, typename T0, typename T1>
struct deduce_type<resize, T0, T1> {
  using type = typename deduce_type_impl<resize, T0, T1>::type;
};

template <bool resize, typename T0, typename T1, typename... Ts>
struct deduce_type<resize, T0, T1, Ts...> {
  using type = typename deduce_type<resize, typename deduce_type_impl<resize, T0, T1>::type, Ts...>::type;
};

template <bool resize, typename... Ts>
using deduce_type_t = typename deduce_type<resize, Ts...>::type;

template <typename T0, typename T1>
struct deduce_first_type_impl {
  using D0 = std::decay_t<T0>;
  using D1 = std::decay_t<T1>;
  using U0 = std::conditional_t<is_object_type_v<D0>, D0, non_ch_type>;
  using U1 = std::conditional_t<is_object_type_v<D1>, D1, non_ch_type>;
  using type = std::conditional_t<(ch_width_v<U0> != 0), U0, U1>;
};

template <typename T0, typename T1>
using deduce_first_type_t = typename deduce_first_type_impl<T0, T1>::type;

}
}