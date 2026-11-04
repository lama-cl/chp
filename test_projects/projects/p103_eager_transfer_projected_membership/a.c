/*
 * p103_eager_transfer_projected_membership/a.c
 *
 * C6g4v regression: eager same-TU direct/data-field/higher-order transfers
 * must merge route-only alternatives while retaining explicit specialization
 * structure: target entity/field path, immutable source leaf, lexical leaf
 * guard, pointer-use shape, attachment, and selected callback target.
 */

struct Box {
  int *payload;
};

static void leaf(void *p) {
  int *ip = (int *)p;
  *ip += 1;               /* one unconditional immutable leaf operation */
}

static void direct_wrapper(void *p) {
  leaf(p);
}

static void consume_box(struct Box *box) {
  int *ip = box->payload;
  *ip += 1;               /* one relative data-field leaf operation */
}

static void callback(void *p) {
  leaf(p);
}

static void callback_other(void *p) {
  int *ip = (int *)p;
  *ip += 2;               /* distinct callback leaf site */
}

static void invoke(void (*fn)(void *), void *p) {
  fn(p);
}

void exercise_eager_transfer_projected_membership(int *value, int choice) {
  struct Box first;
  struct Box second;
  first.payload = value;
  second.payload = value;

  /* Same direct leaf / same actual entity, reached under complementary routes. */
  if (choice)
    direct_wrapper(value);
  else
    direct_wrapper(value);

  /* Same relative field leaf / same canonical field path, distinct routes. */
  if (choice)
    consume_box(&first);
  else
    consume_box(&first);

  /* A different field root/path must remain distinct. */
  consume_box(&second);

  /* Same selected callback target reached under complementary routes. */
  if (choice)
    invoke(callback, value);
  else
    invoke(callback, value);

  /* A different resolved callback target remains distinct. */
  invoke(callback_other, value);
}
