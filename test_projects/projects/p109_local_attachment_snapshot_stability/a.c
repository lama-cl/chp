/*
 * p109_local_attachment_snapshot_stability/a.c
 *
 * C6g4x1b regression fixture.  Several local pointer-arithmetic paths meet
 * at one call leaf.  The diagnostic may retain several variants, potentially
 * reallocating its bounded vector; it must render progress from copies made
 * before that retention rather than from invalidated vector references.
 */

static void consume(char *p) {
  if (p)
    *p = '\0';
}

void local_attachment_snapshot_stability(char *input, int mode) {
  char *cursor;
  if (mode == 0)
    cursor = input + 1;
  else if (mode == 1)
    cursor = (input + 1) + 1;
  else if (mode == 2)
    cursor = ((input + 1) + 1) + 1;
  else
    cursor = (((input + 1) + 1) + 1) + 1;
  consume(cursor);
}
