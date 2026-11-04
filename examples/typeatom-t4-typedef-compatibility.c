// typeatom-t4-typedef-compatibility.c
//
// Purpose:
//   T4 uses TypeAtom semantic/compatibility keys for metric counts, not only
//   rendered type strings. int* and I* should not split concrete target counts
//   merely because one arm uses a typedef spelling.

typedef int I;

void typedef_compat(void *x) {
  int *pi = x;
  I *pt = x;
  (void)pi;
  (void)pt;
}
