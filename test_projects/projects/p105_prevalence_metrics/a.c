struct MetricsBox {
  void *opaque;
  char *bytes;
  void (*callback)(int);
  int count;
};

void *global_opaque;
int *global_numbers;
void (*global_callback)(int);

static void sink(int value) {
  (void)value;
}

void *metrics_example(void *input, char *text, void (*callback)(int)) {
  void *to_void = (void *)text;
  char *from_void = (char *)input;
  unsigned char *byte_view = (unsigned char *)from_void;
  int *typed_view = (int *)byte_view;
  char *implicit_from_void = input;
  callback(1);
  sink(*typed_view);
  return to_void;
}
