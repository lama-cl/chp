typedef int (*handler_t)(void *);
struct job { handler_t fn; void *arg; };

int exec(struct job *j) { return j->fn(j->arg); }
