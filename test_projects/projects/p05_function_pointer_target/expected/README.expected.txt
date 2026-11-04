Purpose:
  Cross-TU function pointer target.

Important functions:
  a.c: run passes idv to ho
  b.c: idv specializes void* to int* and writes
  b.c: ho calls pf(pc)

Expected C2 behavior:
  idv and ho should be indexed as external functions.

Expected C3a behavior:
  Direct named calls can be imported.
  Function-pointer call pf(pc) may remain a higher-order placeholder unless the
  target set is known.

Expected C3b behavior:
  If target propagation sees pf = idv or ho(idv, x), then ho/run can import
  idv's summary through the function-pointer target.
