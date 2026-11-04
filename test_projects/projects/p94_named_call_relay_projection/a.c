// C6g4n regression: named-call relay projection.
//
// The two routes from entry.$0 to the leaf operation in consume() differ only
// in intermediate formal relays (relay_a.$0 / relay_b.$0).  The named-call
// pointer-use solver should retain one consume-side dereference fact keyed by
// its leaf source location and semantic use shape, while keeping the distinct
// relay routes only as bounded witnesses.

static void consume(char *p) {
  *p = 0;
}

static void relay_a(char *p) {
  consume(p);
}

static void relay_b(char *p) {
  relay_a(p);
}

void entry(char *p) {
  relay_a(p);
  relay_b(p);
}

// Distinct leaf locations must remain distinct, even though both operations
// have the same use kind, type, and attachment shape.
static void consume_twice(char *p) {
  *p = 1;
  p[1] = 2;
}

void distinct_leaf_locations(char *p) {
  consume_twice(p);
}
