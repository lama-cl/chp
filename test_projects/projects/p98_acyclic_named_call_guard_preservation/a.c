// C6g4p regression: acyclic named-call transfers retain full caller guards.
static void leaf(void *x) {
  char *p = x;
  *p += 1;
}

void caller(void *x, int mode) {
  if (mode)
    leaf(x);
  else
    leaf(x);
}
