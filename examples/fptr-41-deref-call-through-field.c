// fptr-41-deref-call-through-field.c
//
// Fact F41 (control, supported today): the explicit dereference call form
// (*s.fn)(ctx) on a field of a local record.  Two sibling record types with
// the same first member keep their own targets.  Guards the resolver
// normalisation step (strip parens / unary *) when resolveIndirectCallee
// replaces the L0 helpers.
//
// Origin: owner probe examples/probes/fptr-struct-subtype-siblings.c
// (corrected: valid C, data actually passed).
//
// @fptr-fact F41 deref-call-through-field
// @subject   demo::a
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedFieldCallback behavior=ConcreteSpecializedRead types=int* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
// @subject   demo::b
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedFieldCallback behavior=ConcreteSpecializedRead types=double* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L0
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
struct X { handler_t fn; int i; };
struct Y { handler_t fn; double d; };
int demo(void *a, void *b) {
  struct X x1 = {use_int, 1};
  struct Y y1 = {use_dbl, 2.0};
  return (*x1.fn)(a) + (*y1.fn)(b);
}
