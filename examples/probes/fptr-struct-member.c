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
    union {
        int i,
        char c
    };
    FPTR_t * fn;
};

void f(void *fd) {
    int *pi = fd;
}

void g(void *gd) {
    char *pc = gd;
}


void demo() {
    struct X x1 = {{1},f};
    struct X x2 = {{'a'},g};

    x1.fn();
    x2.fn();
}
