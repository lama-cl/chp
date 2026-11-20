typedef int (*handler_t)(void *);

static int use_int(void *p) { int *q = p; return *q; }

handler_t pick(void) { return use_int; }
