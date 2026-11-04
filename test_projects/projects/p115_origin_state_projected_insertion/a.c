// C6g4y4: duplicate source-local origins are projected when branch state is
// merged, not only when a later pointer-use consumes originState.  The nested
// diamond shape creates many route guards for the same buf/ptrarith source
// shape; the retained origin-state vector must stay bounded by projected keys.

extern void append_string_origin_state(char *text);

void replay_origin_state_insert(char *buf, unsigned long written,
                                int a, int b, int c, int d) {
  if (a) buf += written; else buf += written;
  if (b) buf += written; else buf += written;
  if (c) buf += written; else buf += written;
  if (d) buf += written; else buf += written;
  append_string_origin_state(buf);
}
