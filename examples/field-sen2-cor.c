// Corresponding to field-sen2.c but used a pointer for msg.payload
struct Msg {
  int tag;
  void *payload;
};

int main(void) {
  int x = 0;
  int* pv = &x;
  void* msgpayload;
  struct Msg msg;
  msg.tag = 1;
  msgpayload= pv;
  void *q = msgpayload;
  char *pc = q;

  return x;
}


