// fptr-28-variadic-fptr-argument.c
//
// Fact F28: a function pointer passed through `...` and recovered with
// va_arg.  The higher-order binding is not a named formal, so today's
// $k.$i substitution cannot see it.
//
// Real world: variadic option setters such as curl_easy_setopt(h,
// CURLOPT_WRITEFUNCTION, cb) followed by CURLOPT_WRITEDATA ctx.
//
// @fptr-fact F28 variadic-fptr-argument
// @subject   caller::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=ResolvedCallback behavior=OpaqueForwarding types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
#include <stdarg.h>

typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

static int call_with(void *ctx, ...) {
  va_list ap;
  va_start(ap, ctx);
  handler_t cb = va_arg(ap, handler_t);
  va_end(ap);
  return cb(ctx);
}

int caller(void *ctx) { return call_with(ctx, use_int); }
