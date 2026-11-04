struct Msg {
  int tag;
  void *payload;
};

int main(void) {
  int x = 0;
  int* pv = &x;
  struct Msg msg;
  msg.tag = 1;
  msg.payload = pv;
  void *q = msg.payload;
  char *pc = q;

  return x;
}

