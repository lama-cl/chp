Purpose:
  Intentional duplicate external definition diagnostic.

This is not a valid linked C project. It exists only to test the C2 index
builder's duplicate-external-definition classification.

Expected C2 behavior:
  dup should be reported as:
    duplicate-external-definition
  resolution:
    unsafe
