// GATE fp-fsm
// Table-driven state machine.  The next state is data, the transition
// functions share one void* context that every state downcasts.
//
// Probe subjects:
//   run.$0 (ctx)  -- must inherit struct parser * from every state function
//   run: call table[st].step(ctx) resolves to {st_start, st_body, st_end}
struct parser {
  const char *cur;
  int depth;
};

enum state { ST_START, ST_BODY, ST_END, ST_DONE };

struct transition {
  enum state (*step)(void *ctx);
  const char *name;
};

static enum state st_start(void *ctx) {
  struct parser *p = (struct parser *)ctx;
  p->depth = 0;
  return ST_BODY;
}
static enum state st_body(void *ctx) {
  struct parser *p = (struct parser *)ctx;
  p->cur++;
  return *p->cur ? ST_BODY : ST_END;
}
static enum state st_end(void *ctx) {
  struct parser *p = (struct parser *)ctx;
  p->depth = -1;
  return ST_DONE;
}

static const struct transition table[] = {
    [ST_START] = {st_start, "start"},
    [ST_BODY] = {st_body, "body"},
    [ST_END] = {st_end, "end"},
};

void run(void *ctx) {
  enum state st = ST_START;
  while (st != ST_DONE)
    st = table[st].step(ctx);
}

void demo(const char *text) {
  struct parser p = {text, 0};
  run(&p);
}
