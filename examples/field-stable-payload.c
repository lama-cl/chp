struct Msg {
  void *payload;
};

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  msg.payload = pi;

  int *pi2 = msg.payload;
  *pi2 += 1;

  return i;
}

/*
Expected:

```text
msg.payload : int * => void * => int *
```

Risk should be relatively low because the pointer returns to the same non-void type.
    
    */