/*
 * p106_local_pointer_use_attachment_diagnostics/a.c
 *
 * C6g4x regression fixture.  The three pointer-arithmetic statements are
 * intentionally distinct source operations.  C6g4x diagnostics must retain
 * their distinct operation sites and must not suppress or merge any facts.
 */

static void consume(char *p) {
  if (p)
    *p = '\0';
}

void local_pointer_use_attachment_diagnostics(char *buffer, unsigned offset,
                                              int choice) {
  char *cursor = buffer + offset;
  cursor += 1;
  if (choice)
    cursor += 2;
  else
    cursor += 3;
  consume(cursor);
}
