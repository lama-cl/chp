struct Msg {
  int tag;
  void *payload;
};

int main(void) {
  int x = 0;
  struct Msg msg;
  msg.tag = 1;
  msg.payload = &x;

  if (msg.tag == 1) {
    int *pi = msg.payload;
    *pi += 1;
  } else {
    char *pc = msg.payload;
    *pc += 1;
  }

  return x;
}
