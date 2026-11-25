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

void f(void *fd) {
    int *pi = fd;
}

void g(void *gd) {
    char *pc = gd;
}

typedef void (*FPTR_t) (void*);

void demo() {
    //int nums[] = {1, 2, 3};
    //char* words[] = {"bbc", "xcd", "ede", "def"};
    FPTR_t fp = 0;
    int i = 1;
    char c = 'a';

    fp = f;
    fp(&i);

    fp = g;
    fp(&c);
}
