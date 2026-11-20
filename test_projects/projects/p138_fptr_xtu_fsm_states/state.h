struct state;
typedef struct state (*state_fn)(void *);
struct state { state_fn next; };
struct state st_start(void *p);
