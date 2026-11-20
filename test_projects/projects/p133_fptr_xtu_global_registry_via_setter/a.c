/*
 * Fact X02 (cross-TU F04): b.c owns a global hook and a setter API; a.c
 * registers its own handler through the setter.  The store into g_handler is
 * relative to the setter's parameter ($0), so the slot's target set is only
 * known after the caller binding set_handler(use_int) is applied project-wide.
 * The subject that benefits is in b.c (fire::ctx).
 *
 * Real world: parson json_set_allocation_functions() (parson.c:2052).
 */
typedef int (*handler_t)(void *);

void set_handler(handler_t fn);

static int use_int(void *p) { int *q = p; return *q; }

void install(void) { set_handler(use_int); }
