// C6g4y3: branch-distinguished reaching origins at one source-local AST leaf
// are coalesced by projected local membership before recordPointerUse().
// The source operation, lexical leaf guard, root entity, and attachment shape
// remain distinct; only route/effective-guard alternatives collapse.

extern void append_string_origin_batch(char *text);

void replay_origin_batch(char *buf, unsigned long written,
                         int pretty, int reserve) {
  if (pretty) {
    buf += written;
  } else {
    buf += written;
  }

  if (reserve) {
    append_string_origin_batch(buf);
  } else {
    append_string_origin_batch(buf);
  }
}
