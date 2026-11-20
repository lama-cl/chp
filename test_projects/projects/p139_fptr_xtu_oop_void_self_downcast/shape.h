struct shape_vt { double (*area)(void *self); };
struct shape { const struct shape_vt *vt; };
struct circle { struct shape base; double r; };
void circle_init(struct circle *c, double r);
double shape_area(struct shape *s);
