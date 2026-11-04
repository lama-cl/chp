struct Msg {
  void *payload;
};

int main(void) {
  int i = 0;
  int *pi = &i;

  struct Msg msg;
  struct Msg *pmsg = &msg;

  pmsg->payload = pi;

  void *q = pmsg->payload;
  char *pc = q;
  *pc += 1;

  return i;
}

/*
Expected under the current design:

Storage-location cast histories:
  pmsg.payload : int * => void * => char *
*/