// fptr-17-fsm-next-state.c
//
// Fact F17: a finite state machine where each state function returns the
// next state (a struct wrapping a function pointer, since C cannot spell a
// recursive function-pointer type directly).  The call is loop-carried and the
// target set is the fixpoint of returned targets: {st_start, st_done}.
//
// Real world: "state function returns next state" tokenizers and protocol
// parsers (a common C idiom; no bench instance is pinned for it).
//
// @fptr-fact F17 fsm-next-state
// @subject   run::ctx
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=set@L1
struct state;
typedef struct state (*state_fn)(void *);
struct state { state_fn next; };

static struct state st_done(void *p) {
  int *q = p;
  struct state s = { 0 };
  *q = 0;
  return s;
}

static struct state st_start(void *p) {
  int *q = p;
  struct state s = { st_done };
  *q = 1;
  return s;
}

void run(void *ctx) {
  struct state s = { st_start };
  while (s.next)
    s = s.next(ctx);
}
