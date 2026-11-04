// C6g4y2: a repeated same-type local ptrarith run is recognized before
// a PointerUseRecord is materialized. The representative retains one ptrarith
// atom and later replays hit the projected index without cloning the run.

extern void append_string_origin(char *text);

void replay_pre_materialized_ptrarith(char *buf, unsigned long written, int pretty) {
  if (buf && pretty) {
    buf += written;
    buf += written;
    buf += written;
    append_string_origin(buf);
  }
}
