/*
 * p104_named_call_round_convergence/a.c
 *
 * C6g4w regression: the named-call solver must expose separate per-round
 * progress for parameter cast-history summaries and parameter pointer-use
 * summaries. It must not use pointer-use quiescence alone to terminate a
 * mutually recursive named-call SCC.
 */

struct Buffer {
  int *data;
  unsigned count;
};

static void use_buffer(struct Buffer *buffer) {
  if (buffer && buffer->data)
    buffer->data[buffer->count] += 1;
}

static void parse_b(struct Buffer *buffer);

static void parse_a(struct Buffer *buffer) {
  if (buffer && buffer->count)
    parse_b(buffer);
  use_buffer(buffer);
}

static void parse_b(struct Buffer *buffer) {
  if (buffer && buffer->count > 1)
    parse_a(buffer);
  use_buffer(buffer);
}

void exercise_named_call_round_convergence(struct Buffer *buffer) {
  parse_a(buffer);
}
