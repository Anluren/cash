#pragma once

#include "bit.h"
#include "bus.h"

#define CH_UNION_SIZE(i, x) \
  CH_PAIR_L(x)::bit_count

#define CH_UNION_FIELD(i, x) \
  cash::detail::slice_ref<base, CH_PAIR_L(x)::bit_count> CH_PAIR_R(x)

#define CH_UNION_CTOR_INIT(i, x) \
  CH_PAIR_R(x)(_)

#define CH_UNION_IMPL(name, ...) \
  class name : public cash::detail::ch_bitbase<std::max({CH_FOR_EACH(CH_UNION_SIZE, CH_SEP_COMMA, __VA_ARGS__)})> { \
  public:\
    using base = cash::detail::ch_bitbase<std::max({CH_FOR_EACH(CH_UNION_SIZE, CH_SEP_COMMA, __VA_ARGS__)})>; \
    using base::operator=; \
    using data_type = typename base::data_type; \
    class bus_type : public cash::detail::ch_busbase<std::max({CH_FOR_EACH(CH_UNION_SIZE, CH_SEP_COMMA, __VA_ARGS__)})> { \
    public: \
      using base = cash::detail::ch_busbase<std::max({CH_FOR_EACH(CH_UNION_SIZE, CH_SEP_COMMA, __VA_ARGS__)})>; \
      using base::operator=; \
      using data_type = typename base::data_type; \
      bus_type() : CH_FOR_EACH(CH_UNION_CTOR_INIT, CH_SEP_COMMA, __VA_ARGS__) {} \
      bus_type(const bus_type& rhs) : CH_FOR_EACH(CH_UNION_CTOR_INIT, CH_SEP_COMMA, __VA_ARGS__), _(rhs._) {} \
      bus_type(const base& rhs) : CH_FOR_EACH(CH_UNION_CTOR_INIT, CH_SEP_COMMA, __VA_ARGS__) { base::operator=(rhs); } \
      bus_type& operator=(const bus_type& rhs) { \
        _ = rhs._; \
        return *this; \
      } \
      CH_FOR_EACH(CH_UNION_FIELD, CH_SEP_SEMICOLON, __VA_ARGS__); \
    protected: \
      ch_bus<base::bit_count> _; \
      void read(data_type& inout, size_t offset, size_t length) const override { \
        read_data(_, inout, offset, length); \
      } \
      void write(size_t dst_offset, const data_type& in, size_t src_offset, size_t src_length) override { \
        write_data(_, dst_offset, in, src_offset, src_length); \
      } \
    };\
    name() : CH_FOR_EACH(CH_UNION_CTOR_INIT, CH_SEP_COMMA, __VA_ARGS__) {} \
    name(const name& rhs) : CH_FOR_EACH(CH_UNION_CTOR_INIT, CH_SEP_COMMA, __VA_ARGS__), _(rhs._) {} \
    name(const base& rhs) : CH_FOR_EACH(CH_UNION_CTOR_INIT, CH_SEP_COMMA, __VA_ARGS__) { base::operator=(rhs); } \
    name& operator=(const name& rhs) { \
      _ = rhs._; \
      return *this; \
    } \
    CH_FOR_EACH(CH_UNION_FIELD, CH_SEP_SEMICOLON, __VA_ARGS__); \
  protected: \
    ch_bit<base::bit_count> _; \
    void read(data_type& inout, size_t offset, size_t length) const override { \
      read_data(_, inout, offset, length); \
    } \
    void write(size_t dst_offset, const data_type& in, size_t src_offset, size_t src_length) override { \
      write_data(_, dst_offset, in, src_offset, src_length); \
    } \
  public:

#define CH_UNION(name, body, ...) CH_UNION_IMPL(name, CH_REM body) __VA_ARGS__ }
