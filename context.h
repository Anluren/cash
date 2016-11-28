#pragma once

namespace chdl_internal {

class lnode;
class snode;
class lnodeimpl;
class snodeimpl;
class undefimpl;
class litimpl;
class ioimpl;
class inputimpl;
class outputimpl;
class tapimpl;
class assertimpl;
class clock_event;
class cdomain;

class context : public refcounted {
public:
  context();
  ~context();

  //--

  std::list<lnodeimpl*>::iterator erase_node(const std::list<lnodeimpl*>::iterator& iter);

  //--

  void push_clk(lnodeimpl* clk);
  void pop_clk();
  lnodeimpl* get_clk();

  void push_reset(lnodeimpl* reset);
  void pop_reset();
  lnodeimpl* get_reset();

  //--
  
  uint32_t add_node(lnodeimpl* node);  
  void remove_node(undefimpl* node);
  
  void begin_branch();
  void end_branch();
  
  void begin_cond(lnodeimpl* cond);
  void end_cond();
  bool has_conditionals() const {
    return m_active_branches != 0;
  }
  lnodeimpl* resolve_conditionals(lnodeimpl* dst, lnodeimpl* src);
  
  litimpl* create_literal(const bitvector& value);
  
  cdomain* create_cdomain(const std::vector<clock_event>& sensitivity_list);
  void remove_cdomain(cdomain* cd);
  
  void register_gtap(ioimpl* node);
  
  //-- 

  lnodeimpl* bind_input(snodeimpl* bus);  
  snodeimpl* bind_output(lnodeimpl* output);
  
  void register_tap(const std::string& name, lnodeimpl* lnode);
  snodeimpl* get_tap(const std::string& name, uint32_t size);
  
  //--
  
  void syntax_check();
    
  //--
  
  void get_live_nodes(std::set<lnodeimpl*>& live_nodes);
  
  //--
  
  void tick(ch_cycle t);  
  void tick_next(ch_cycle t);
  void eval(ch_cycle t);
  
  //--
  
  void toVerilog(const std::string& module_name, std::ostream& out);
  
  void dumpAST(std::ostream& out, uint32_t level);
  
protected:
  
  struct cond_val_t {
    lnodeimpl* dst;
    lnodeimpl* sel;
    bool defined;
  };
  
  struct cond_block_t {
    lnodeimpl* cond;
    std::set<lnodeimpl*> locals;
    std::vector<uint32_t> defs; 
    cond_block_t(lnodeimpl* cond_) : cond(cond_) {}
  };

  std::list<lnodeimpl*>   m_undefs;
  std::list<lnodeimpl*>   m_nodes;
  std::list<cdomain*>     m_cdomains;
  std::vector<ioimpl*>    m_inputs;
  std::vector<ioimpl*>    m_outputs;
  std::vector<tapimpl*>   m_taps;
  std::list<ioimpl*>      m_gtaps;
  std::list<litimpl*>     m_literals;
  std::list<cond_block_t> m_conds;   
  std::vector<cond_val_t> m_cond_vals;
  std::stack<lnode>       m_clk_stack;
  std::stack<lnode>       m_reset_stack;

  uint32_t   m_nodeids;
  inputimpl* m_clk;
  inputimpl* m_reset;
  
  int m_active_branches;
  
  std::map<std::string, unsigned> m_dup_taps;
  
  friend class optimizer;
  friend class ch_simulator;
  friend class ch_tracer;
};

context* ctx_begin();
context* ctx_curr();
void ctx_end();

}
