#pragma once

#include "ioimpl.h"

namespace cash {
namespace detail {

class assertimpl : public ioimpl {
public:
  assertimpl(const lnode& src, const std::string& msg);
  
  const bitvector& eval(ch_cycle t) override;
  
  void print_vl(std::ostream& out) const override;

private:
  std::string msg_;
};

}
}
