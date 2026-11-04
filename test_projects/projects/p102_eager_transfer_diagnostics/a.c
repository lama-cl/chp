/*
 * p102_eager_transfer_diagnostics/a.c
 *
 * C6g4u regression target: exercise the eager AST-traversal materializers
 * separately from the deferred named-call SCC solver.  With
 * --analysis-progress, direct-param, data-field-param, and
 * higher-order-direct scopes must expose source-record and candidate-vector
 * growth without changing pointer-use membership.
 */

struct Box {
  int *payload;
};

static void leaf(void *p) {
  int *ip = (int *)p;
  *ip += 1;
}

static void direct_wrapper(void *p) {
  leaf(p);
}

static void consume_box(struct Box *box) {
  int *ip = box->payload;
  *ip += 1;
}

static void callback(void *p) {
  leaf(p);
}

static void invoke(void (*fn)(void *), void *p) {
  fn(p);
}

void exercise_eager_transfer_diagnostics(int *value) {
  struct Box box;
  box.payload = value;

  /* Two eager direct-call materializations of the same leaf. */
  direct_wrapper(value);
  direct_wrapper(value);

  /* Eager relative data-field materialization: box.payload. */
  consume_box(&box);

  /* Eager resolved higher-order callback materialization. */
  invoke(callback, value);
}
