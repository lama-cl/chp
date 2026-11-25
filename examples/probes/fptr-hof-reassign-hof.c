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

void hof(FPTR_t *pf, void *data) {
    pf(data);
}

void f(void *fd) {
    int *pi = fd;
}

void g(void *gd) {
    char *pc = gd;
}

void demo() {
    //int nums[] = {1, 2, 3};
    //char* words[] = {"bbc", "xcd", "ede", "def"};
    FPTR_t fp = 0;
    int i = 1;
    char c = 'a';

    fp = f;
    hof(fp, &i);

    fp = g;
    hof(fp, &c);
}
