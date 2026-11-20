int use_int(void *p) { int *q = p; return *q; }
int use_dbl(void *p) { double *q = p; return (int)*q; }
