// fptr-1.c
//
// Purpose:
//
// Program:
//   int arr[4];
//   void *v = &arr[i];
//   char *pc = v;
//
// Expected key local history:
//   v : void * => char *
//
// Expected behavior metric for v:
//   view: LocalView
//   behavior: ByteViewNoDeref
//
// Rationale:
//   char* is used only as a byte-oriented view/conversion. There is no
//   dereference/read/write through pc in this example.

typedef void (*FPTR_t) (void*);

struct X {
    FPTR_t * fn;
    int i,
};

struct Y {
    FPTR_t *fn;
    char c;
};

void f(void *fd) {
    int *pi = fd;
}

void g(void *gd) {
    char *pc = gd;
}


void demo() {
    X x1 = {f, {1}};
    Y y1 = {g, {'a'}};

    x1.fn();
    y1.fn();
}
