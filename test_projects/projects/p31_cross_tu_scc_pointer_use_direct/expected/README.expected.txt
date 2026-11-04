C4Q2 regression: direct cross-TU recursive SCC pointer-use solving.

Project shape:
  a.c: f(void *x) calls imported g(x)
  b.c: g(void *y) casts y to int *, writes through it, then calls imported f(y)

Expected after running the SCC driver for at least two import rounds:
  - Cross-TU SCC parameter-summary fixpoint reports domains:
      parameter-cast-histories, parameter-pointer-uses
  - f::x inherits g.$0's concrete write evidence:
      as=int *, access=int : deref-readwrite
      detail includes "project-scc via g.$0"
  - Pointer behavior metrics for f::x should classify the parameter as
      ConcreteSpecializedWrite
    with derefTypes/writeDerefTypes containing int *.
  - g::y keeps its local int * deref-readwrite evidence and should not grow
    unbounded recursive detail chains.

Suggested command pattern:
  python3 scripts/run_scc_param_fixpoint.py \
    test_projects/projects/p31_cross_tu_scc_pointer_use_direct/a.c \
    test_projects/projects/p31_cross_tu_scc_pointer_use_direct/b.c \
    --tool /path/to/cast-history-tool \
    -p test_projects/projects/p31_cross_tu_scc_pointer_use_direct \
    --work-dir /tmp/p31-c4q2 \
    --max-iters 4 \
    --keep-going-on-no-recursive-scc
