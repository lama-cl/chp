// feature-logging-control-flow.c
//
// Purpose:
//   Exercise feature-coverage diagnostics for constructs that are currently
//   unsupported or approximated by the AST-order transfer engine.
//
// Expected diagnostics include:
//   [approximated] switch-statement
//   [approximated] for-loop
//   [approximated] while-loop
//   [approximated] do-loop
//   [approximated] break-statement
//   [approximated] continue-statement
//   [approximated] conditional-operator
//   [unsupported] goto-statement
//   [approximated] label-statement
//   [unsupported] function-pointer-array-call

typedef void *(*handler_t)(void *);

void *h1(void *x) { return x; }
void *h2(void *x) { return x; }

void feature_logging_demo(void *x, int tag, handler_t *handlers, int n) {
  switch (tag) {
  case 1: {
    int *pi = x;
    (void)pi;
    break;
  }
  default: {
    char *pc = x;
    (void)pc;
    break;
  }
  }

  for (int i = 0; i < n; ++i) {
    handlers[i](x);
  }

  while (tag > 0) {
    if (tag == 3) {
      --tag;
      continue;
    }
    break;
  }

  do {
    --tag;
  } while (tag > 0);

  double *pd = 0;
  if (n < 0)
    goto done;
  pd = x;

done:
  (void)pd;

  (tag ? handlers[0] : handlers[1])(x);
}
