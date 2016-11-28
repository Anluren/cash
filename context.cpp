#include <thread>
#include "lnode.h"
#include "litimpl.h"
#include "regimpl.h"
#include "memimpl.h"
#include "ioimpl.h"
#include "snodeimpl.h"
#include "selectimpl.h"
#include "assertimpl.h"
#include "cdomain.h"
#include "device.h"
#include "context.h"
#include "arithm.h"
#include "select.h"

using namespace std;
using namespace chdl_internal;

thread_local context* tls_ctx = nullptr;

context::context()
  : m_nodeids(0)
  , m_clk(nullptr)
  , m_reset(nullptr) 
  , m_active_branches(0)
{}

context::~context() { 
  //
  // cleanup allocated resources
  //
  
  for (lnodeimpl* node : m_nodes) {
    node->release();
  }

  assert(m_cdomains.empty());
  assert(m_active_branches == 0);
}

std::list<lnodeimpl*>::iterator
context::erase_node(const std::list<lnodeimpl*>::iterator& iter) {
  lnodeimpl* const node = *iter;
  if (node == m_clk) {
    m_clk = nullptr;
  } else
  if (node == m_reset) {
    m_reset = nullptr;
  }
  node->release();
  return m_nodes.erase(iter);
}

void context::push_clk(lnodeimpl* clk) {
  m_clk_stack.emplace(clk);
}

void context::pop_clk() {
  m_clk_stack.pop();
}

void context::push_reset(lnodeimpl* reset) {
  m_reset_stack.emplace(reset);
}

void context::pop_reset() {
  m_reset_stack.pop();
}

lnodeimpl* context::get_clk() {
  if (!m_clk_stack.empty())
    return m_clk_stack.top().get_impl();
  if (m_clk == nullptr)
    m_clk = new inputimpl("clk", this, 1);
  return m_clk;
}

lnodeimpl* context::get_reset() {
  if (!m_reset_stack.empty())
    return m_reset_stack.top().get_impl();
  if (m_reset == nullptr)
     m_reset = new inputimpl("reset", this, 1);
  return m_reset;
}

uint32_t context::add_node(lnodeimpl* node) {
  uint32_t nodeid = ++m_nodeids;  
#ifndef NDEBUG
  uint32_t dbg_node = platform::self().get_dbg_node();
  if (dbg_node) {
    CHDL_CHECK(nodeid != dbg_node, "debugbreak on nodeid %d hit!", nodeid);
  }
#endif
  if (node->m_name == "undef") {
    m_undefs.emplace_back(node);
  } else {
    m_nodes.emplace_back(node);
  }  
  node->acquire();
  if (m_conds.size() > 0) {
    // memory ports have global scope
    if (node->get_name() != "memport") {
      m_conds.front().locals.emplace(node);
    }
  }
  return nodeid;  
}

void context::remove_node(undefimpl* node) {
  assert(m_undefs.size() > 0);
  m_undefs.remove(node);
}

void context::begin_branch() {
  ++m_active_branches;
}

void context::end_branch() {
  assert(m_active_branches > 0);
  if (0 == --m_active_branches) {
    m_cond_vals.clear();
  }
}

void context::begin_cond(lnodeimpl* cond) {
  m_conds.emplace_front(cond);
}

void context::end_cond() {
  const cond_block_t& cb = m_conds.front();
  for (uint32_t v : cb.defs) {
    m_cond_vals[v].defined = false; 
  }
  m_conds.pop_front();
}

lnodeimpl* context::resolve_conditionals(lnodeimpl* dst, lnodeimpl* src) {
  if (m_conds.size() > 0 
   && (0 == m_conds.front().locals.count(dst))) {
    lnodeimpl* cond;
    {
      // aggregate nested condition value
      auto iter = m_conds.begin();
      cond = iter->cond;    
      ++iter;
      for (auto iterEnd = m_conds.end(); iter != iterEnd && 0 == iter->locals.count(dst); ++iter) {
        if (iter->cond) {
          if (cond) {
            cond = createAluNode(op_and, 1, cond, iter->cond);
          } else {
            cond = iter->cond;
          }
        }
      }
    }
 
    // lookup dst value if already defined
    auto iter = std::find_if(m_cond_vals.begin(), m_cond_vals.end(),
      [dst](const cond_val_t& v)->bool { return v.dst == dst; });
    if (iter != m_cond_vals.end()) {
      CHDL_CHECK(!iter->defined, "redundant assignment to node %s%d(#%d)!\n", dst->get_name().c_str(), dst->get_size(), dst->get_id());   
      if (cond) {
        src = createSelectNode(cond, src, iter->sel);
      }
      dynamic_cast<selectimpl*>(iter->sel)->m_srcs[2].assign(src, true);
      iter->sel = src;
      src = dst; // return original value
    } else {
      if (cond) {
        if (dst == nullptr) {
          dst = new undefimpl(this, src->get_size());
        }
        src = createSelectNode(cond, src, dst);
        cond_block_t& cb = m_conds.front();
        cb.defs.push_back(m_cond_vals.size());
        m_cond_vals.push_back({src, src, true});      
      }
    }
  }
  return src;
}

litimpl* context::create_literal(const bitvector& value) {
  for (litimpl* lit : m_literals) {
    if (lit->get_value() == value) {
      return lit;
    }
  }
  litimpl* const lit = new litimpl(this, value);
  m_literals.emplace_back(lit);
  return lit;
}

void context::register_gtap(ioimpl* node) {
  m_gtaps.emplace_back(node);  
}

cdomain* context::create_cdomain(const std::vector<clock_event>& sensitivity_list) {
  // return existing cdomain 
  for (cdomain* cd : m_cdomains) {
    if (*cd == sensitivity_list)
      return cd;
  }  
  // allocate new cdomain
  cdomain* const cd = new cdomain(this, sensitivity_list);
  m_cdomains.emplace_back(cd);
  return cd;
}

void context::remove_cdomain(cdomain* cd) {
  m_cdomains.remove(cd);
}

lnodeimpl* context::bind_input(snodeimpl* bus) {
  inputimpl* const impl = new inputimpl("input", this, bus->get_size());
  impl->bind(bus);
  m_inputs.emplace_back(impl);
  return impl;
}

snodeimpl* context::bind_output(lnodeimpl* output) {
  outputimpl* const impl = new outputimpl("output", output);
  m_outputs.emplace_back(impl);
  return impl->get_bus();
}

void context::register_tap(const std::string& name, lnodeimpl* node) {
  // resolve duplicate names
  string full_name(name);
  unsigned instances = m_dup_taps[name]++;
  if (instances > 0) {
    if (instances == 1) {
      // rename first instance
      auto iter = std::find_if(m_taps.begin(), m_taps.end(),
        [name](tapimpl* t)->bool { return t->get_tapName() == name; });
      assert(iter != m_taps.end());
      (*iter)->m_name = fstring("%s_%d", name.c_str(), 0);
    }
    full_name = fstring("%s_%d", name.c_str(), instances);
  }
  // add to list
  m_taps.emplace_back(new tapimpl(full_name, node));
}

snodeimpl* context::get_tap(const std::string& name, uint32_t size) {
  for (tapimpl* tap : m_taps) {
    if (tap->get_tapName() == name) {
      CHDL_CHECK(tap->get_size() == size, "tap bus size mismatch: received %u, expected %u", size, tap->get_size());
      return tap->get_bus();
    }
  } 
  CHDL_ABORT("couldn't find tab '%s'", name.c_str());
}

void context::syntax_check() {
  // check for un-initialized nodes
  if (m_undefs.size()) {
    this->dumpAST(std::cerr, 1);    
    for (auto node : m_undefs) {
      fprintf(stderr, "error: un-initialized node %s%d(#%d)!\n", node->get_name().c_str(), node->get_size(), node->get_id());
    }
    if (m_undefs.size() == 1)
      CHDL_ABORT("1 node has not been initialized.");
    else
      CHDL_ABORT("%zd nodes have not been initialized.", m_undefs.size());
  }
}

void context::get_live_nodes(std::set<lnodeimpl*>& live_nodes) {
  // get inputs
  for (auto node : m_inputs) {
    live_nodes.emplace(node);
  }

  // get outputs
  for (auto node : m_outputs) {
    live_nodes.emplace(node);
  }  
  
  // get debug taps
  for (auto node : m_taps) {
    live_nodes.emplace(node);
  }

  // get assert taps
  for (auto node : m_gtaps) {
    live_nodes.emplace(node);
  }
}

void context::tick(ch_cycle t) {
  for (auto cd : m_cdomains) {
    cd->tick(t);
  }  
}

void context::tick_next(ch_cycle t) {
  for (auto cd : m_cdomains) {
    cd->tick_next(t);
  }  
}

void context::eval(ch_cycle t) {
  // evaluate outputs
  for (auto node : m_outputs) {
    node->eval(t);
  }

  // evaluate taps
  for (auto node : m_taps) {
    node->eval(t);
  }  
  
  // evaluate asserts
  for (auto node : m_gtaps) {
    node->eval(t);
  }
}

void context::toVerilog(const std::string& module_name, std::ostream& out) {
  TODO("Not yet implemented!");
}

void context::dumpAST(std::ostream& out, uint32_t level) {
  for (lnodeimpl* node : m_nodes) {
    node->print(out);
  }
}

///////////////////////////////////////////////////////////////////////////////

context* chdl_internal::ctx_begin() {
  context* const ctx = new context();
  tls_ctx = ctx;
  ctx->acquire();
  return ctx;
}

void chdl_internal::ctx_end() {
  tls_ctx = nullptr;
}

context* chdl_internal::ctx_curr() {
  CHDL_CHECK(tls_ctx, "invalid CHDL context!");
  return tls_ctx;
}
