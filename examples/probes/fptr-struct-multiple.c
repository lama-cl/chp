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

struct FN {
    FPTR_t *r;
    FPTR_t *w;
};

struct X {
    FN f;
    int i,
};

struct Y {
    FN f;
    char c;
};

void fr(void *fd) {
    int *pi = fd;
}

void fw(void *fd) {
    int *pi = fd;
    *pi += 1;
}

void gr(void *gd) {
    char *pc = gd;
}

void gw(void *gd) {
    char *pc = gd;
    *pc += 1;
}

void demo() {
    X x1 = {{fr, fw}, {1}};
    Y y1 = {{gr, gw}, {'a'}};

    x1.f.r();
    x1.f.w();
    y1.f.r();
    y1.f.w();
}
