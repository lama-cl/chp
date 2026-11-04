/*
 * C6g4z regression: direct calls embedded in if conditions must be traversed
 * exactly once and become same-TU named-call edges.  The byte access in
 * check_bytes must propagate through wrapper and api even though both calls
 * occur in conditions rather than expression statements.
 */

typedef unsigned char u8;

static int check_bytes(const void *raw) {
  const u8 *bytes = (const u8 *)raw;
  return bytes[0] != 0;
}

static int wrapper(const void *raw) {
  if (check_bytes(raw) == 0)
    return 0;
  return 1;
}

int api(const void *raw) {
  if (wrapper(raw) == 0)
    return 0;
  return 1;
}
