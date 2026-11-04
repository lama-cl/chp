Purpose:
  Cross-TU return flow.

Important functions:
  a.c: f(void *x) returns g(x)
  b.c: g(void *y) converts y to char* and returns p

Expected C1/local behavior:
  f records unresolved/importable direct call to g.
  g records y : void * => char * and return aliases/projected from y/p.

Expected C2 index behavior:
  g should be indexed as:
    fn:(ptr:void)->ptr:void

Expected C3 direct-import behavior:
  f::x should get imported char* specialization evidence.
  f::ret should reflect g's return summary.
