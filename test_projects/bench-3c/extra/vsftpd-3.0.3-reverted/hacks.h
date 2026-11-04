#define unchecked_widen_bounds(TYPE, WIDE_PTR, BOUNDS, PTR)\
  TYPE WIDE_PTR  = 0; \
   { WIDE_PTR = (TYPE)PTR; }

#define checked_widen_bounds(TYPE, WIDE_PTR, BOUNDS, PTR)\
  TYPE WIDE_PTR  = 0; \
  WIDE_PTR = (TYPE)PTR;
