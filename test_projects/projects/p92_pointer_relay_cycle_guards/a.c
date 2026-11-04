// C6g4l regression: source-level pointer-arithmetic replay guard.
//
// Two distinct arithmetic statements must remain visible as two ptrarith
// steps.  C6g4l suppresses only reapplication of one exact source operation
// to a lineage that already contains that operation.

void distinct_pointer_arithmetic_sites(int *p) {
  p += 1;
  p += 2;
  char *bytes = (char *)p;
  (void)bytes;
}

struct Box {
  int *payload;
};

void distinct_data_field_uses(struct Box *box) {
  int *first = box->payload;
  int *second = box->payload;
  *first += 1;
  *second += 2;
}
