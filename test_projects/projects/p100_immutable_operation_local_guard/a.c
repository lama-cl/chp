// C6g4r regression: two guarded reaching origins feed one lexically
// unconditional leaf use.  The source-level operation-local guard must remain
// <always>; only the full record guard retains `choose` / `!choose`.

static void relay_b(void *x, int depth, int choose);

static void relay_a(void *x, int depth, int choose) {
  void *selected;
  if (choose)
    selected = x;
  else
    selected = x;

  char *bytes = selected;
  *bytes += 1;  // lexical leaf guard: <always>

  if (depth > 0)
    relay_b(selected, depth - 1, choose);
}

static void relay_b(void *x, int depth, int choose) {
  if (depth > 0)
    relay_a(x, depth - 1, choose);
}

void wrapper(void *x, int choose) {
  relay_a(x, 2, choose);
}
