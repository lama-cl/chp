// GATE st-heap-carrier
// A void* payload is hidden in a heap-allocated struct that is only ever
// reached through pointers, and is stored and loaded in different functions.
// There is no named instance to key a field entity on.
//
// Probe subjects:
//   node_new.$0 (payload : void *)  -- stored into n->payload; must inherit
//                                      the struct item * interpretation from
//                                      node_item's load of the same field
struct item {
  int key;
};

struct node {
  struct node *next;
  void *payload;
};

void *xalloc(unsigned long n);

struct node *node_new(void *payload) {
  struct node *n = (struct node *)xalloc(sizeof *n);
  n->next = 0;
  n->payload = payload;
  return n;
}

int node_item(const struct node *n) {
  struct item *it = (struct item *)n->payload;
  return it->key;
}

int demo(struct item *it) {
  struct node *n = node_new(it);
  return node_item(n);
}
