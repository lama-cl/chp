typedef int (*handler_t)(void *);

int use_int(void *p) { int *q = p; return *q; }
int use_int_neg(void *p) { int *q = p; return -*q; }

const handler_t table[2] = { use_int, use_int_neg };
