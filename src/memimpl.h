#pragma once

#include "ioimpl.h"
#include "cdimpl.h"

namespace ch {
namespace internal {

class memportimpl;
class marportimpl;
class msrportimpl;
class mwportimpl;

class memimpl : public ioimpl {
public:

  bool is_logic_rom() const;

  uint32_t data_width() const {
    return data_width_;
  }

  uint32_t num_items() {
    return num_items_;
  }

  bool has_init_data() const {
    return !init_data_.empty();
  }

  const sdata_type& init_data() const {
    return init_data_;
  }

  auto& rdports() const {
    return rdports_;
  }

  auto& rdport(uint32_t index) const {
    return rdports_[index];
  }

  auto& wrports() const {
    return wrports_;
  }

  auto& wrport(uint32_t index) const {
    return wrports_[index];
  }

  int port_index(memportimpl* port) const;

  bool is_readwrite(memportimpl* port) const;

  virtual lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) override;

  memportimpl* create_arport(lnodeimpl* addr,
                             const source_location& sloc);

  memportimpl* create_srport(lnodeimpl* cd,
                             lnodeimpl* addr,
                             lnodeimpl* enable,
                             const source_location& sloc);

  mwportimpl* create_wport(lnodeimpl* cd,
                           lnodeimpl* addr,
                           lnodeimpl* wdata,
                           lnodeimpl* enable,
                           const source_location& sloc);

  void remove_port(memportimpl* port);

  void print(std::ostream& out) const override;

protected:

  memimpl(context* ctx,
          uint32_t data_width,
          uint32_t num_items,
          const sdata_type& init_data,
          const source_location& sloc);
  
  std::vector<memportimpl*> rdports_;
  std::vector<mwportimpl*>  wrports_;
  sdata_type init_data_;
  uint32_t data_width_;
  uint32_t num_items_;

  friend class context;
};

///////////////////////////////////////////////////////////////////////////////

class memportimpl : public ioimpl {
public:

  memimpl* mem() const {
    return mem_;
  }

  const lnode& addr() const {
    return srcs_[addr_idx_];
  }

  bool has_enable() const {
    return (enable_idx_ != -1);
  }

  const lnode& enable() const {
    return srcs_[enable_idx_];
  }

  bool has_cd() const {
    return (cd_idx_ != -1);
  }

  auto& cd() const {
    return srcs_[cd_idx_];
  }

protected:

  memportimpl(context* ctx,
              lnodetype type,
              uint32_t size,
              memimpl* mem,
              lnodeimpl* cd,
              lnodeimpl* addr,
              lnodeimpl* enable,
              const source_location& sloc);

  ~memportimpl();

  memimpl* mem_;
  int cd_idx_;
  int addr_idx_;  
  int enable_idx_;
};

///////////////////////////////////////////////////////////////////////////////

class marportimpl : public memportimpl {
public:

  virtual lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) override;

protected:

  marportimpl(context* ctx,
              memimpl* mem,
              lnodeimpl* addr,
              const source_location& sloc);

  ~marportimpl();

  friend class context;
};

///////////////////////////////////////////////////////////////////////////////

class msrportimpl : public memportimpl {
public:

  virtual lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) override;

protected:

  msrportimpl(context* ctx,
              memimpl* mem,
              lnodeimpl* cd,
              lnodeimpl* addr,
              lnodeimpl* enable,
              const source_location& sloc);

  ~msrportimpl();

  friend class context;
};

///////////////////////////////////////////////////////////////////////////////

class mwportimpl : public memportimpl {
public:

  const lnode& wdata() const {
    return srcs_[wdata_idx_];
  }

  virtual lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) override;

  friend class context;

protected:

  mwportimpl(context* ctx,
             memimpl* mem,
             lnodeimpl* cd,
             lnodeimpl* addr,
             lnodeimpl* wdata,
             lnodeimpl* enable,
             const source_location& sloc);

  mwportimpl(context* ctx, uint32_t size, const source_location& sloc);

  ~mwportimpl();

  int wdata_idx_;
};

}
}
