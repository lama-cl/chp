// C6g4y: repeated local pointer-arithmetic views at one AST leaf must
// not create a new pointer-use fact solely because reaching guards recur.

extern void append_string_origin(char *text);

void json_serialize_to_buffer_r(char *buf, unsigned long count, int is_pretty) {
  if (buf != 0 && is_pretty) {
    buf = buf + count;
    if (count > 0)
      append_string_origin(buf);
  }
}
