#pragma once

#include "lnodeimpl.h"

namespace ch {
namespace internal {

class selectimpl : public lnodeimpl {
public:

  bool has_key() const {
    return key_idx_ != -1;
  }

  auto& key() const {
    return this->src(key_idx_);
  }

  void remove_key() {
    this->srcs().erase(this->srcs().begin());
    key_idx_ = -1;
  }

  bool is_ternary() const {
    return this->srcs().size() == (has_key() ? 4 : 3);
  }

  lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) const override;

  bool equals(const lnodeimpl& other) const override;

  void print(std::ostream& out) const override;

protected:

  selectimpl(context* ctx,
             uint32_t size,
             lnodeimpl* key,
             const source_location& sloc);

  int key_idx_;

  friend class context;
};

}
}
