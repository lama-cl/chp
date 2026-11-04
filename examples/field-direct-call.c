struct Msg {
  void *payload;
};

void f(void *x) {
  char *pc = x;
  *pc += 1;
}

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  msg.payload = pi;

  f(msg.payload);

  return i;
}

/*
Expected:

```text
msg.payload : int * => void * => f⌜char *⌝f
```

Pointer-use summary should include transferred `char *` deref-write behavior.    
*/