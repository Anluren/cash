#pragma once

#include "common.h"

namespace ch {
namespace internal {

class context;
class lnodeimpl;

class moduleimpl : public refcounted {
public:
  moduleimpl(size_t signature, const char* name);
  ~moduleimpl();

  void begin_context();

  void end_context();

  void compile();

  context* get_ctx() const {
    return ctx_;
  }

protected:
  context* ctx_;
  context* old_ctx_;
};

}
}