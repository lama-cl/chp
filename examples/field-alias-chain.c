struct Msg {
  void *payload;
};

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  msg.payload = pi;

  void *q1 = msg.payload;
  void *q2 = q1;
  char *pc = q2;
  *pc += 1;

  return i;
}

/*

Expected ideal histories:

```text
msg.payload : int * => void * => char *
q1          : void * => char *
q2          : void * => char *
```

Since we currently only do one-step alias-parent propagation, this test is useful because it may reveal whether we need limited alias-chain propagation later.   
*/