struct Msg {
  void *payload1;
  void *payload2;
};

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  msg.payload1 = pi;
  msg.payload2 = msg.payload1;

  char *pc = msg.payload2;
  *pc += 1;

  return i;
}

/*
Desired result:

```text
msg.payload1 : int * => void *
msg.payload2 : int * => void * => char *
```

This is important because real structs often move payloads between fields.
    
*/