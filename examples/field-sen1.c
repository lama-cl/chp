struct Msg {
  int tag;
  void *payload;
};

int main(void) {
  int x = 0;
  int* pi = &x;
  void *pv = pi;
  struct Msg msg;
  msg.tag = 1;
  msg.payload = pv;
  char *pc = msg.payload;

  if (msg.tag == 1) {
    int *pi = msg.payload;
    *pi += 1;
  } else {
    char *pc = msg.payload;
    *pc += 1;
  }

  return x;
}

