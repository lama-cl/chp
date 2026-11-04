Purpose:
  Acyclic direct cross-TU call.

Important functions:
  a.c: f(void *x) calls g(x)
  b.c: g(void *y) converts y to int* and writes through it

Expected C1/local behavior:
  f should contain an unresolved or importable direct call reference to g.
  g should summarize y as void * => int * with write-through-int behavior.

Expected C2 index behavior:
  g should be indexed as an external function with a signature key like:
    fn:(ptr:void)->void

Expected C3 direct-import behavior:
  f::x should receive imported evidence from g:
    void * => g⌜void * => int *⌝g
  f should be classified as affected by an imported concrete-specialized write.
