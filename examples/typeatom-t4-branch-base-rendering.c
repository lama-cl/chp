// typeatom-t4-branch-base-rendering.c
//
// Purpose:
//   Rendered branch-sensitive storage bases must not be parsed as raw types.
//   The metric output should not contain unpaired branch delimiters inside
//   distinctTypes/pointerTypes/nonVoidPointerTypes.
//
// Expected intent:
//   distinctTypes: 3 {char *, int *, void *}
//   pointerTypes: 3 {char *, int *, void *}
//   nonVoidPointerTypes: 2 {char *, int *}

struct Msg {
  int tag;
  void *payload;
};

void branch_base_rendering(struct Msg msg, int *pi, char *pc0) {
  if (msg.tag == 1)
    msg.payload = pi;
  else
    msg.payload = pc0;

  void *q = msg.payload;
  char *pc = q;
  (void)pc;
}
