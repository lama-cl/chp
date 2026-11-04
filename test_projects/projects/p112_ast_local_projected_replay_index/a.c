// C6g4y1: repeated local pointer arithmetic at one source leaf should
// hit the projected-membership index and retain only bounded route evidence.

extern void append_string_origin(char *text);

void replay_local_leaf(char *buf, unsigned long count, int pretty) {
  if (buf && pretty) {
    for (unsigned long i = 0; i < count; ++i) {
      buf = buf + 1;
      append_string_origin(buf);
    }
  }
}
