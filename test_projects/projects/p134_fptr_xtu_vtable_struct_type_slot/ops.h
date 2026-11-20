typedef int (*handler_t)(void *);
struct ops { handler_t read; };
struct obj { const struct ops *vt; void *data; };
void obj_init(struct obj *o, void *data);
int obj_read(struct obj *o);
