#pragma once

#include "bitvector.h"

namespace chdl_internal {

class lnodeimpl;
class context;

typedef refcounted_ptr<lnodeimpl> lnodeimpl_ptr;
typedef refcounted_ptr<context>   context_ptr;

typedef uint64_t ch_cycle;

class lnode {
public:

  lnode() {}
  lnode(const lnode& rhs);
  lnode(lnode&& rhs);
  lnode(const lnode& rhs, uint32_t size);
  explicit lnode(lnodeimpl_ptr impl);
  lnode(const std::string& value);
  explicit lnode(const std::initializer_list<uint32_t>& value, uint32_t size);

  virtual ~lnode();
  
  lnode& operator=(const lnode& rhs);
  
  lnode& operator=(lnode&& rhs);

  uint32_t get_id() const;
  
  uint32_t get_size() const;
  
  context_ptr get_ctx() const;
  
  bool ready() const;
  
  bool valid() const;
 
  const bitvector& eval(ch_cycle t);
  
  void assign(const std::initializer_list<uint32_t>& value, uint32_t size);
  
  void assign(uint32_t dst_offset, const lnode& src, uint32_t src_offset, uint32_t src_length, uint32_t size);
  
  explicit operator lnodeimpl_ptr() const { 
    return m_impl; 
  }
  
  void ensureInitialized(uint32_t size) const;

protected:

  void assign(lnodeimpl_ptr impl) const;
  
  void move(lnode& rhs);
  
  void clear();
  
  lnodeimpl_ptr m_impl;
  
  friend class lnodeimpl;
  friend class context;
  template <typename T> friend T* get_impl(const lnode& n);
};

inline std::ostream& operator<<(std::ostream& out, const lnode& rhs) {
  out << rhs.get_id();
  return out;
}

lnode createNullNode(uint32_t size);

}