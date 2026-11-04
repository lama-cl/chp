C4Q2 regression: cross-TU SCC pointer-use solving through a same-TU relay.

Project shape:
  a.c: f(void *x) calls local relay(x); relay(void *r) calls imported g(r)
  b.c: g(void *y) casts y to int *, writes through it, then calls imported f(y)

This checks the local-edge half of C4Q2: after relay receives imported pointer-use
facts from g.$0, f.$0 must inherit those facts through the local f -> relay edge
inside the project-level SCC fixpoint.

Expected after running the SCC driver:
  - Cross-TU SCC parameter-summary fixpoint reports domains:
      parameter-cast-histories, parameter-pointer-uses
  - relay::r has project-scc/imported int * deref-readwrite evidence through g.$0.
  - f::x also has int * deref-readwrite evidence, with detail including
      project-scc via relay.$0
    or a composed local/imported project-scc path.
  - Pointer behavior metrics for f::x should classify the parameter as
      ConcreteSpecializedWrite.

Suggested command pattern:
  python3 scripts/run_scc_param_fixpoint.py \
    test_projects/projects/p32_cross_tu_scc_pointer_use_relay/a.c \
    test_projects/projects/p32_cross_tu_scc_pointer_use_relay/b.c \
    --tool /path/to/cast-history-tool \
    -p test_projects/projects/p32_cross_tu_scc_pointer_use_relay \
    --work-dir /tmp/p32-c4q2 \
    --max-iters 4 \
    --keep-going-on-no-recursive-scc
