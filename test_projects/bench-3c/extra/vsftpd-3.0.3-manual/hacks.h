#define unchecked_widen_bounds(TYPE, WIDE_PTR, BOUNDS, PTR)\
  TYPE WIDE_PTR : BOUNDS = 0; \
  _Unchecked { WIDE_PTR = _Assume_bounds_cast<TYPE>(PTR, BOUNDS); }

#define checked_widen_bounds(TYPE, WIDE_PTR, BOUNDS, PTR)\
  TYPE WIDE_PTR : BOUNDS = 0; \
  WIDE_PTR = _Dynamic_bounds_cast<TYPE>(PTR, BOUNDS);
