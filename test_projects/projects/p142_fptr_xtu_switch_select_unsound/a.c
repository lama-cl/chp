/*
 * Fact X11 (cross-TU F09, SOUNDNESS BUG today): switch arms select between two
 * targets defined in b.c.  The syntactic switch traversal keeps only the last
 * assignment, so the imported summary of use_int is never composed.
 *
 * @fptr-fact X11 xtu-switch-select-unsound
 * @subject   run::ctx
 * @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=double*
 * @expected  category=GuardedActiveReinterpretation types=double*,int* resolution=set@L0
 */
typedef int (*handler_t)(void *);

int use_int(void *p);
int use_dbl(void *p);

int run(int kind, void *ctx) {
  handler_t cb = 0;
  switch (kind) {
  case 0:
    cb = use_int;
    break;
  default:
    cb = use_dbl;
    break;
  }
  return cb(ctx);
}
