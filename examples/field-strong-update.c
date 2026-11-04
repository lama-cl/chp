struct Msg {
  void *payload;
};

int main(void) {
  int i = 0;
  char c = 0;

  int *pi = &i;
  char *pc0 = &c;

  struct Msg msg;
  msg.payload = pi;
  msg.payload = pc0;

  char *pc = msg.payload;
  *pc += 1;

  return i;
}