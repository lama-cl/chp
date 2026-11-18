// GATE fp-global-reassigned
// A global function pointer is initialised to one target and reassigned to
// another in a different function.  The pre-P1 tool resolves calls through
// it from the initializer only (tier global-initializer), which is unsound.
// Found on parson (parson_free = free, reassigned by
// json_set_allocation_functions).
//
// Probe subjects:
//   call.$0 (p)  -- must carry both int * (initial target) and double *
//                   (reassigned target)
typedef void (*hook_fn)(void *);

static void h_int(void *p) {
  int *pi = (int *)p;
  *pi = 1;
}

static void h_dbl(void *p) {
  double *pd = (double *)p;
  *pd = 1.0;
}

static hook_fn hook = h_int;

void set_hook(hook_fn f) {
  hook = f;
}

void call(void *p) {
  hook(p);
}

void demo(void) {
  set_hook(h_dbl);
}
