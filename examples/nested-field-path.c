struct Inner {
  void *payload;
};

struct Msg {
  int tag;
  struct Inner inner;
};

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  msg.inner.payload = pi;

  char *pc = msg.inner.payload;
  *pc += 1;

  return i;
}

/*

Expected:

```text
msg.inner.payload : int * => void * => char *
```

This tests whether `MemberExpr` path construction works for nested fields.
    
*/