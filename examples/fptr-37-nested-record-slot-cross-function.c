// fptr-37-nested-record-slot-cross-function.c
//
// Fact F37: a function pointer lives in a nested record (struct ops embedded
// as a member of two *different* outer records), is installed in one function
// and called through the outer record in another.  Each outer record must keep
// its own target: call_x::a -> int*, call_y::b -> double*.
//
// Design consequence (D7 revised as D7'): an object-insensitive slot keyed by
// the innermost record type (record:struct ops.fn) merges both installs and
// yields a FALSE multi-type answer for both subjects.  The slot key must be
// the outermost record visible in the access path (record:struct X.o.fn); the
// inner type key is used only when the access itself goes through struct ops *.
//
// Origin: owner probes examples/probes/fptr-struct-first-member.c and
// fptr-struct-multiple.c (corrected: valid C, data actually passed).
//
// @fptr-fact F37 nested-record-slot-cross-function
// @subject   call_x::a
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=int* resolution=exact@L1
// @subject   call_y::b
// @baseline  category=OpaqueTransportOrViewOnly callback=FieldCallback behavior=CallbackDependentPointer types=-
// @expected  category=MonomorphicActiveSpecialization types=double* resolution=exact@L1
typedef int (*handler_t)(void *);
static int use_int(void *p) { int *q = p; return *q; }
static int use_dbl(void *p) { double *q = p; return (int)*q; }
struct ops { handler_t fn; };
struct X { struct ops o; int i; };
struct Y { struct ops o; double d; };
void init_x(struct X *x) { x->o.fn = use_int; }
void init_y(struct Y *y) { y->o.fn = use_dbl; }
int call_x(struct X *x, void *a) { return x->o.fn(a); }
int call_y(struct Y *y, void *b) { return y->o.fn(b); }
