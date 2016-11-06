#include "tickimpl.h"
#include "context.h"
#include "bit.h"

using namespace std;
using namespace chdl_internal;

tickimpl::tickimpl(context* ctx) 
  : lnodeimpl("tick", ctx, CHAR_BIT * sizeof(ch_cycle))
  , m_ctime(~0ull) 
{}

const bitvector& tickimpl::eval(ch_cycle t) {
  if (m_ctime != t) {
    m_ctime = t;
    m_value = t;
  }
  return m_value;
}

// LCOV_EXCL_START
void tickimpl::print_vl(std::ostream& out) const {
  TODO("Not yet implemented!");
}
// LCOV_EXCL_END

///////////////////////////////////////////////////////////////////////////////

ch_bit<64> chdl_internal::ch_tick() {
  return ch_bit<64>(new tickimpl(ctx_curr()));
}