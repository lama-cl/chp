/*
 * Today (measured): Opaque, unresolved (no return-target summary).
 *
 * @fptr-fact X14 xtu-switch-return-in-loop
 * @subject   drive::x
 * @baseline  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=- resolution=unresolved@- sigmismatch=false
 * @expected  category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L0 dispatch=unrelated
 */
typedef int (*handler_t)(void *);
handler_t lookup(int k);

int drive(int n, void *x) {
  int rc = 0;
  for (int i = 0; i < n; ++i)
    rc += lookup(i)(x);
  return rc;
}
