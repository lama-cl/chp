// GATE P0-escape-accounting (formal callback parameter)
// A data pointer is handed to a function-pointer *parameter*.  Inside
// `pump` the callee is whatever the caller passes, so the interpretation of
// `ctx` is lost at the definition; it is recovered per call site only.
// Found on zlib-1.2.11 inflateBack(in, in_desc, out, out_desc).
//
// Probe subjects:
//   pump.$1 (ctx)  -- escape.reason must be callback-formal and the escape
//                     site must point at the call `in(ctx)`
typedef int (*in_fn)(void *ctx);

int pump(in_fn in, void *ctx) {
  return in(ctx);
}
