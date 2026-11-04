struct Msg {
  void *payload;
};

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  struct Msg *pmsg = &msg;

  pmsg->payload = pi;

  char *pc = pmsg->payload;
  *pc += 1;

  return i;
}

/*
Expected:

```text
pmsg.payload : int * => void * => char *
```

or possibly:

```text
msg.payload : int * => void * => char *
```

depending on your current syntactic representation. If it prints `pmsg.payload`, that is acceptable for now because we are not doing full object-sensitive alias analysis.

*/