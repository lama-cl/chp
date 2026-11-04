/*
Expected behavior for C4P0: single-TU return-alias composition.

The callee idv returns an alias of its parameter after interpreting it as int *:

  idv($0 : void * => int *) -> void * [$0 => int * => void *] [ret aliases $0]

At the call site:

  void *z = idv(y);
  char *c = z;

z aliases the returned value, which aliases y. Therefore the later char *
interpretation through z should propagate back to y. The root/source history
for y should use the wrapper-free semantic continuation:

  use::y : void * => int * => void * => char *

The returned local z may keep a provenance-rich rendering that records the
callee return-alias path:

  use::z : idv⌜$0 => int * => void *⌝idv => void * => char *

A future display-normalization mode may also render z's semantic core as:

  use::z : void * => int * => void * => char *

but the current intended convention is:

  - source/root entity histories, such as use::y, should receive the
    wrapper-free semantic continuation in source-to-use order;
  - returned-local histories, such as use::z, may preserve display-only
    call-return provenance wrappers.

The important invariant is that the cast through z is not isolated to z only;
it must also extend the reaching source/root y because the call return aliases
idv.$0. The required semantic ordering is:

  y enters idv as void *
  idv interprets it as int *
  idv returns it through void *
  z is later interpreted as char *

Therefore the root/source path must be:

  void * => int * => void * => char *

not:

  void * => char * => void * => int *
*/
void *idv(void *x) {
  int *p = x;
  *p = *p + 1;
  return p;
}

void use(void *y) {
  void *z = idv(y);
  char *c = z;
  (void)c;
}
