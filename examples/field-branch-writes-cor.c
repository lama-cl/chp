struct Msg {
  int tag;
  void *payload;
};

int main(void) {
  int i = 0;
  char c = 0;

  int *pi = &i;
  char *pc0 = &c;
  struct Msg msg;
  
  void* msgpayload;

  if (msg.tag == 1) {
    msgpayload = pi;
  } else {
    msgpayload = pc0;
  }

  void *q = msgpayload;
  char *pc = q;
  *pc += 1;

  return i;
}

/*
Expected shape:

```text
msg.payload : ⟨[msg.tag == 1] int * => void *,
                [!(msg.tag == 1)] char * => void *⟩ => char *
```

The exact formatting may differ, but the key is that the field origin should be branch-sensitive.

Expected risk:

```text
shape: BranchPolymorphic
risk: BranchPolymorphicWrite
```

or possibly `ByteViewWrite` depending on how the branch write is represented.
    
*/