Expected C3a behavior:
- Assignment form should behave like local initialization:
    r = g(x);
    return r;
- The assignment RHS originInfoForExpr(CallExpr) should attach imported return
  alias information to r's origin.
