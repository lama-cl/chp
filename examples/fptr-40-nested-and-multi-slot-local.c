// fptr-40-nested-and-multi-slot-local.c
//
// Fact F40 (control, supported today): two instances of one record type, each
// with a nested ops record holding two fptr slots (read, write), initialised
// locally and called through nested member paths.  Per-instance, per-field L0
// state keeps them apart: demo::x -> int*, demo::y -> double*.  Guards M3c:
// introducing object-insensitive L1 record slots must not replace this
// per-object L0 precision (L0 overlay wins when it has information).
//
// Origin: owner probes examples/probes/fptr-struct-first-member.c,
// fptr-struct-multiple.c, fptr-struct-member.c and fptr-1.c (corrected).
//
// @fptr-fact F40 nested-and-multi-slot-local
// @subject   demo::x
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedFieldCallback behavior=ConcreteSpecializedWrite types=int* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L0
// @subject   demo::y
// @baseline  category=MonomorphicActiveSpecialization callback=ResolvedFieldCallback behavior=ConcreteSpecializedWrite types=double* resolution=exact@L0 sigmismatch=false
// @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L0
typedef int (*handler_t)(void *);
static int rd_int(void *p) { int *q = p; return *q; }
static int wr_int(void *p) { int *q = p; *q += 1; return 0; }
static int rd_dbl(void *p) { double *q = p; return (int)*q; }
static int wr_dbl(void *p) { double *q = p; *q += 1.0; return 0; }
struct ops { handler_t r; handler_t w; };
struct X { struct ops o; int tag; };
int demo(void *x, void *y) {
  struct X a = {{rd_int, wr_int}, 0};
  struct X b = {{rd_dbl, wr_dbl}, 1};
  return a.o.r(x) + a.o.w(x) + b.o.r(y) + b.o.w(y);
}
