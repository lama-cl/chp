/*
 * p110_named_call_observation_delta_diagnostics/a.c
 *
 * C6g4w2 regression fixture.  A field-held callback is selected under a
 * branch and invoked through a named helper.  With --analysis-progress, a
 * completed named-call round that changes a parameter summary must report a
 * bounded observation delta explaining the target parameter, update path,
 * before/after fingerprints, and structural shape.
 */

typedef void (*sink_fn)(void *);

struct Dispatcher {
  sink_fn sink;
};

static void read_int(void *p) {
  int *ip = (int *)p;
  (void)*ip;
}

static void read_char(void *p) {
  char *cp = (char *)p;
  (void)*cp;
}

static void invoke(const struct Dispatcher *dispatch, void *value) {
  dispatch->sink(value);
}

void observation_delta_driver(void *value, int choose_char) {
  struct Dispatcher dispatch;
  if (choose_char)
    dispatch.sink = read_char;
  else
    dispatch.sink = read_int;
  invoke(&dispatch, value);
}
