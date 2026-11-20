typedef int (*handler_t)(void *);

struct reg { handler_t fn; void *arg; };
static struct reg regs[8];
static int nregs;

void add(handler_t fn, void *arg) {
  regs[nregs].fn = fn;
  regs[nregs].arg = arg;
  nregs++;
}

void fire(void) {
  for (int i = 0; i < nregs; i++)
    regs[i].fn(regs[i].arg);
}
