struct Msg {
  void *payload;
};

int main(void) {
  unsigned long address = 0x40001000;
  int *ptr = (int *)address;

  struct Msg msg;
  msg.payload = ptr;

  char *pc = msg.payload;

  return pc != 0;
}

/*
Expected:

```text
msg.payload : unsigned long => int * => void * => char *
```

Metrics should include:

```text
intToPtr=yes
risk: DangerousProvenance
```
    
    */