// branch-position-basic.c
//
// Purpose:
//   Branch-cast metrics should report branch position and per-alternative
//   sequence lengths.
//
// Expected branch metrics for pv/msg.payload-like value:
//   branchClass: TagLikeTypeSplit or GuardedTypeSplit
//   branchPosition: earliest around 1 or 2 depending on base/tail split
//   branchAltSeqLen: min=1, max=1
//   branchNonVoidTypes: {char *, int *}
//   branchFlags: differentTypes=yes
struct Msg {
  int tag;
  void *payload;
};

void demo(struct Msg msg) {
  int i = 0;
  char c = 0;

  if (msg.tag == 1) {
    int *pi = msg.payload;
    *pi += 1;
  } else {
    char *pc = msg.payload;
    *pc += 1;
  }
}
