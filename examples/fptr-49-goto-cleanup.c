// fptr-49-goto-cleanup.c
//
// Fact F49 (CFG, goto; round 4): the C cleanup idiom.  On error, cb is set
// and control jumps forward to `out`.  Today goto is Unsupported (07 §2).
// A CFG treats it as an edge: x gets {use_int, use_dbl}.
//
// Real world: 732 bench files use goto (9,204 statements), mostly this idiom.
//
// Today (measured): set double*,int* (right set by accident: the goto is traversed in source order).
//
// @fptr-fact F49 goto-cleanup
// @subject   run::x
// @baseline  category=UnguardedMultiTargetActiveReinterpretation callback=ResolvedCallback behavior=MultiTypeRead types=double*,int* resolution=set@L0 sigmismatch=false
// @expected  types=double*,int* resolution=set@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
int run(int err, void *x) {
  handler_t cb = use_int;
  if (err) {
    cb = use_dbl;
    goto out;
  }
  err = 1;
out:
  return cb(x);
}
