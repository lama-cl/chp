// GATE fp-dlsym-plugin (TU 2 of 2: plugin)
struct job {
  int id;
  int done;
};

int plugin_entry(void *data) {
  struct job *j = (struct job *)data;
  j->done = 1;
  return j->id;
}
