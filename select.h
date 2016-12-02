#pragma once

#include "bit.h"

namespace chdl_internal {

lnodeimpl* createSelectNode(lnodeimpl* cond, lnodeimpl* a, lnodeimpl* b);

class select_impl {  
protected:
  
  struct stmt_t {
    lnodeimpl* cond;
    lnodeimpl* value;
  };
  
  using stmts_t = std::stack<stmt_t>;
  
  lnodeimpl* m_key;
  stmts_t* m_stmts;
    
public:
  
  ~select_impl() {
    if (m_stmts)
      delete m_stmts;
  }
  
  select_impl(lnodeimpl* key = nullptr) : m_key(key), m_stmts(new stmts_t()) {}
  
  void push(lnodeimpl* cond, lnodeimpl* value) {
    m_stmts->push({cond, value});
  }
  
  lnodeimpl* eval(lnodeimpl* value);
};

template <unsigned N> 
class select_t {
public:
    
  select_t<N>& operator()(const ch_logicbase& cond, const ch_bitbase<N>& value) {
    m_impl.push(cond.get_node().get_impl(), value.get_node().get_impl());
    return *this;
  }
  
  select_t<N>& operator()(const ch_logicbase& cond, const ch_bit<N>& value) {
    m_impl.push(cond.get_node().get_impl(), value.get_node().get_impl());
    return *this;
  }
  
  ch_bit<N> operator()(const ch_bitbase<N>& value) {
    return ch_bit<N>(m_impl.eval(value.get_node().get_impl()));
  }
  
  ch_bit<N> operator()(const ch_bit<N>& value) {
    return ch_bit<N>(m_impl.eval(value.get_node().get_impl()));
  }
  
protected:
  
  select_t(const ch_logicbase& cond, const ch_bitbase<N>& value) {
    m_impl.push(cond.get_node().get_impl(), value.get_node().get_impl());
  }

  select_impl m_impl;
  
  template <unsigned N_> friend select_t<N_> ch_select(const ch_logicbase& cond, const ch_bitbase<N_>& value);
};

template <unsigned N> 
class select2_t {
public:

  template <unsigned M>  
  class values_t {
  public:
    
    values_t(lnodeimpl* key, lnodeimpl* cond, lnodeimpl* value) : m_impl(key) {
      m_impl.push(cond, value);
    }
    
    values_t& operator()(const ch_bitbase<N>& cond, const ch_bitbase<M>& value) {
      m_impl.push(cond.get_node().get_impl(), value.get_node().get_impl());
      return *this;
    }
    
    values_t& operator()(const ch_bitbase<N>& cond, const ch_bit<M>& value) {
      m_impl.push(cond.get_node().get_impl(), value.get_node().get_impl());
      return *this;
    }
    
    values_t& operator()(const ch_bit<N>& cond, const ch_bitbase<M>& value) {
      m_impl.push(cond.get_node().get_impl(), value.get_node().get_impl());
      return *this;
    }
    
    values_t& operator()(const ch_bit<N>& cond, const ch_bit<M>& value) {
      m_impl.push(cond.get_node().get_impl(), value.get_node().get_impl());
      return *this;
    }
    
    ch_bit<M> operator()(const ch_bitbase<M>& value) {
      return ch_bit<M>(m_impl.eval(value.get_node().get_impl()));
    }
    
    ch_bit<M> operator()(const ch_bit<M>& value) {
      return ch_bit<M>(m_impl.eval(value.get_node().get_impl()));
    }  
    
  protected:
    
    select_impl m_impl;
  };

  template <unsigned M>
  values_t<M> operator()(const ch_bitbase<N>& cond, const ch_bitbase<M>& value) {
    return values_t<M>(m_key, cond.get_node().get_impl(), value.get_node().get_impl());
  }
  
  template <unsigned M>
  values_t<M> operator()(const ch_bitbase<N>& cond, const ch_bit<M>& value) {
    return values_t<M>(m_key, cond.get_node().get_impl(), value.get_node().get_impl());
  }
  
  template <unsigned M>
  values_t<M> operator()(const ch_bit<N>& cond, const ch_bitbase<M>& value) {
    return values_t<M>(m_key, cond.get_node().get_impl(), value.get_node().get_impl());
  }
  
  template <unsigned M>
  values_t<M> operator()(const ch_bit<N>& cond, const ch_bit<M>& value) {
    return values_t<M>(m_key, cond.get_node().get_impl(), value.get_node().get_impl());
  }
  
protected:
  
  lnodeimpl* m_key;
  
  select2_t(const ch_bitbase<N>& key) : m_key(key.get_node().get_impl()) {}

  template <unsigned N_> friend select2_t<N_> ch_select(const ch_bitbase<N_>& key);
};

template <unsigned N> 
select2_t<N> ch_select(const ch_bitbase<N>& key) {
  return select2_t<N>(key);
}

template <unsigned N> 
select_t<N> ch_select(const ch_logicbase& cond, const ch_bitbase<N>& value) {
  return select_t<N>(cond, value);
}

template <unsigned N> 
select_t<N> ch_select(const ch_logicbase& cond, const ch_bit<N>& value) {
  return ch_select(cond, reinterpret_cast<const ch_bitbase<N>&>(value));
}

template <unsigned N>
ch_bit<N> ch_select(const ch_logicbase& cond, const ch_bitbase<N>& True, const ch_bitbase<N>& False) {
  return ch_bit<N>(createSelectNode(cond.get_node().get_impl(), True.get_node().get_impl(), False.get_node().get_impl()));
}

template <unsigned N>
ch_bit<N> ch_select(const ch_logicbase& cond, const ch_bitbase<N>& True, const ch_bit<N>& False) {
  return ch_bit<N>(createSelectNode(cond.get_node().get_impl(), True.get_node().get_impl(), False.get_node().get_impl()));
}

template <unsigned N>
ch_bit<N> ch_select(const ch_logicbase& cond, const ch_bit<N>& True, const ch_bitbase<N>& False) {
  return ch_bit<N>(createSelectNode(cond.get_node().get_impl(), True.get_node().get_impl(), False.get_node().get_impl()));
}

template <unsigned N>
ch_bit<N> ch_select(const ch_logicbase& cond, const ch_bit<N>& True, const ch_bit<N>& False) {
  return ch_bit<N>(createSelectNode(cond.get_node().get_impl(), True.get_node().get_impl(), False.get_node().get_impl()));
}

template <unsigned N> 
ch_bit<N> ch_min(const ch_bitbase<N>& lhs, const ch_bitbase<N>& rhs) {
  return ch_select<N>(lhs < rhs, lhs, rhs);
}

template <unsigned N> 
ch_bit<N> ch_min(const ch_bitbase<N>& lhs, const ch_bit<N>& rhs) {
  return ch_select<N>(lhs < rhs, lhs, rhs);
}

template <unsigned N> 
ch_bit<N> ch_min(const ch_bit<N>& lhs, const ch_bitbase<N>& rhs) {
  return ch_select<N>(lhs < rhs, lhs, rhs);
}

template <unsigned N> 
ch_bit<N> ch_min(const ch_bit<N>& lhs, const ch_bit<N>& rhs) {
  return ch_select<N>(lhs < rhs, lhs, rhs);
}

template <unsigned N> 
ch_bit<N> ch_max(const ch_bitbase<N>& lhs, const ch_bitbase<N>& rhs) {
  return ch_select<N>(lhs > rhs, lhs, rhs);
}

template <unsigned N> 
ch_bit<N> ch_max(const ch_bitbase<N>& lhs, const ch_bit<N>& rhs) {
  return ch_select<N>(lhs > rhs, lhs, rhs);
}

template <unsigned N> 
ch_bit<N> ch_max(const ch_bit<N>& lhs, const ch_bitbase<N>& rhs) {
  return ch_select<N>(lhs > rhs, lhs, rhs);
}

template <unsigned N> 
ch_bit<N> ch_max(const ch_bit<N>& lhs, const ch_bit<N>& rhs) {
  return ch_select<N>(lhs > rhs, lhs, rhs);
}

}
