// GATE un-tagged
// Tagged union: the active member is selected by a discriminant.  Storage is
// shared, so a write through one member is the value later read through the
// member the tag selects.
//
// Probe subjects:
//   value_get_int.$0 / field v->u.ip  -- guarded by v->tag == T_INT
//   value_set.$1 (payload : void *)   -- stored through u.raw, later read
//                                        through u.ip / u.dp under tag guards:
//                                        history must be a guarded parallel
//                                        {int *, double *}, classified as a
//                                        tagged-union carrier
enum tag { T_INT, T_DBL };

struct value {
  enum tag tag;
  union {
    int *ip;
    double *dp;
    void *raw;
  } u;
};

void value_set(struct value *v, void *payload, enum tag t) {
  v->tag = t;
  v->u.raw = payload;
}

double value_read(const struct value *v) {
  switch (v->tag) {
  case T_INT:
    return (double)*v->u.ip;
  case T_DBL:
    return *v->u.dp;
  }
  return 0.0;
}

double demo(void) {
  int i = 3;
  double d = 4.0;
  struct value a, b;
  value_set(&a, &i, T_INT);
  value_set(&b, &d, T_DBL);
  return value_read(&a) + value_read(&b);
}
