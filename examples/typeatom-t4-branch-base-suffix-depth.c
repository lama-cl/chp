// typeatom-t4-branch-base-suffix-depth.c
//
// Purpose:
//   seqDepth should match the longest visible printed type path when a rendered
//   branch-sensitive base has a sequential suffix after the branch close.
//
// Representative printed history:
//   q : ⟨[tag] int * => void *, [!tag] char * => void *⟩ => void * => char *
//
// Expected Option-A metric:
//   seqDepth: 4
//
// because each visible branch path is:
//   int *  => void * => void * => char *
//   char * => void * => void * => char *

struct Msg {
  int tag;
  void *payload;
};

void branch_base_suffix_depth(struct Msg msg, int *pi, char *pc0) {
  if (msg.tag == 1)
    msg.payload = pi;
  else
    msg.payload = pc0;

  void *q0 = msg.payload;
  void *q = q0;
  char *pc = q;
  (void)pc;
}
