/*
 * Fact X12 (cross-TU F10, SOUNDNESS BUG today): the callback is reassigned on
 * the loop back edge; both targets are defined in b.c.  Only use_int's
 * imported summary is composed today.
 *
 * @fptr-fact X12 xtu-loop-backedge-unsound
 * @subject   run::ctx
 * @baseline  category=MonomorphicActiveSpecialization callback=ResolvedCallback behavior=ConcreteSpecializedRead types=int*
 * @expected  category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L0
 */
typedef int (*handler_t)(void *);

int use_int(void *p);
int use_dbl(void *p);

int run(int n, void *ctx) {
  handler_t cb = use_int;
  int acc = 0;
  for (int i = 0; i < n; i++) {
    acc += cb(ctx);
    cb = use_dbl;
  }
  return acc;
}
