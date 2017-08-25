#pragma once

#include "lnodeimpl.h"
#include "arithm.h"

namespace cash {
namespace detail {

class aluimpl : public lnodeimpl {
public:
  aluimpl(ch_alu_op op, const lnode& a, const lnode& b);
  aluimpl(ch_alu_op op, const lnode& a);
  
  ch_alu_op get_alu_op() const {
    return alu_op_;
  }  

  const bitvector& eval(ch_cycle t) override;  
  void print_vl(std::ostream& out) const override;
  
protected:
  ch_alu_op alu_op_;
  ch_cycle ctime_;
};

}
}