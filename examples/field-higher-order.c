struct Msg {
  void *payload;
};

void *idv(void *x) {
  int *pi = x;
  *pi += 1;
  return pi;
}

void *fret(void *x) {
  char *pc = x;
  *pc += 1;
  return x;
}

void ho(void *(*pf)(void *), void *x) {
  char *pc = x;
  pf(pc);
}

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  msg.payload = pi;

  void *(*pfunc)(void *) = idv;
  ho(pfunc, msg.payload);

  pfunc = fret;
  ho(pfunc, msg.payload);

  return i;
}

/*
Expected:

```text
msg.payload : int * => void * => ⟨|
  ho⌜char * => void * => int *⌝ho,
  ho⌜char * => void * => char *⌝ho
|⟩
```

This is a very good stress test because it combines:

```text
field origin + function pointer strong update + higher-order transfer
```
    
    */