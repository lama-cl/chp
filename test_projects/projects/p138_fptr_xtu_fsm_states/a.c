/*
 * Fact X07 (cross-TU F17): the state functions live in b.c, the driver loop in
 * a.c.  The driver's target set is the fixpoint over returned next-state
 * targets, which are only visible in b.c's return summaries.
 *
 * @fptr-fact X07 xtu-fsm-states
 * @subject   run::ctx
 * @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
 * @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1
 */
#include "state.h"

void run(void *ctx) {
  struct state s = { st_start };
  while (s.next)
    s = s.next(ctx);
}
