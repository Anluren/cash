#pragma once

#include "bit.h"

namespace ch {
namespace internal {

class memimpl;

class memory {
public:
  memory(uint32_t data_width, uint32_t addr_width, bool writeEnable);

  template <typename T>
  void load(const std::vector<T>& data, uint32_t data_width, uint32_t lines) {
    assert(data.size() <= (lines * CH_CEILDIV(data_width, sizeof(T) * 8)));
    std::vector<uint8_t> packed;
    packed.reserve(CH_CEILDIV(data_width * lines, 8));
    uint8_t curr_value = 0;
    uint8_t curr_size = 0;
    for (const T& x : data) {
      for (int i = 0, n = std::min<uint32_t>(sizeof(T) * 8, data_width); i < n; ++i) {
        curr_value |= ((x >> i) & 0x1) << curr_size++;
        if (curr_size == 8) {
          packed.emplace_back(curr_value);
          curr_value = 0;
          curr_size = 0;
        }
      }
    }
    if (curr_size) {
      packed.emplace_back(curr_value);
    }
    this->load(packed);
  }

  void load(const char* file);

  lnode& read(const lnode& addr) const;

  void write(const lnode& addr,
             size_t dst_offset,
             const nodelist& in,
             size_t src_offset,
             size_t length);

protected:

  void load(const std::vector<uint8_t>& data);

  memimpl* impl_;
};

template <unsigned N>
class memport_ref : public ch_bitbase<N> {
public:
  using base = ch_bitbase<N>;
  using value_type = ch_bit<N>;

  memport_ref& operator=(const ch_bitbase<N>& rhs) {
    base::assign(rhs);
    return *this;
  }

protected:
  memport_ref(memory& mem, const lnode& addr)
    : mem_(mem)
    , addr_(addr)
  {}

  void read_lnode(nodelist& inout, size_t offset, size_t length) const override {
    CH_CHECK(offset + length <= N, "invalid read range");
    inout.push(mem_.read(addr_), offset, length);
  }

  void write_lnode(size_t dst_offset, const nodelist& in, size_t src_offset, size_t length) override {
    CH_CHECK(0 == dst_offset || N == length, "partial update not supported!");
    mem_.write(addr_, dst_offset, in, src_offset, length);
  }

  void read_bytes(uint32_t dst_offset, void* out, uint32_t out_cbsize, uint32_t src_offset, uint32_t length) const override {
    CH_UNUSED(dst_offset, out, out_cbsize, src_offset, length);
    CH_ABORT("invalid call");
  }

  void write_bytes(uint32_t dst_offset, const void* in, uint32_t in_cbsize, uint32_t src_offset, uint32_t length) override {
    CH_UNUSED(dst_offset, in, in_cbsize, src_offset, length);
    CH_ABORT("invalid call");
  }

  memory& mem_;
  lnode addr_;

  template <unsigned W, unsigned A> friend class ch_ram;
};

template <unsigned W, unsigned A>
class ch_rom {
public:
    ch_rom() : mem_(W, A, false) {}
  
    ch_rom(const char* init_file) : mem_(W, A, false) {
      mem_.load(init_file);
    }
    
    template <typename T>
    ch_rom(const std::vector<T>& init_data) : mem_(W, A, false) {
      mem_.load<T>(init_data, W, 1 << A);
    }
    
    template <typename T>
    ch_rom(const std::initializer_list<T>& init_data) : mem_(W, A, false) {
      mem_.load<T>(init_data, W, 1 << A);
    }
    
    template <typename T,
              CH_REQUIRES(is_bit_convertible<T, A>::value)>
    const auto operator[](const T& addr) const {
      return make_type<ch_bit<W>>(mem_.read(get_lnode<T, A>(addr)));
    }
    
protected:
    memory mem_;
};

template <unsigned W, unsigned A>
class ch_ram {
public:
    ch_ram() : mem_(W, A, true) {}
  
    ch_ram(const char* init_file) : mem_(W, A, true) {
      mem_.load(init_file);
    }

    template <typename T>
    ch_ram(const std::vector<T>& init_data) : mem_(W, A, true) {
      mem_.load<T>(init_data, W, 1 << A);
    }

    template <typename T>
    ch_ram(const std::initializer_list<T>& init_data) : mem_(W, A, true) {
      mem_.load<T>(init_data, W, 1 << A);
    }
    
    template <typename T,
              CH_REQUIRES(is_bit_convertible<T, A>::value)>
    const auto operator[](const T& addr) const {
      return make_type<ch_bit<W>>(mem_.read(get_lnode<T, A>(addr)));
    }
    
    template <typename T,
              CH_REQUIRES(is_bit_convertible<T, A>::value)>
    memport_ref<W> operator[](const T& addr) {
      return memport_ref<W>(mem_, get_lnode<T, A>(addr));
    }
    
protected:
    memory mem_;
};

}
}
