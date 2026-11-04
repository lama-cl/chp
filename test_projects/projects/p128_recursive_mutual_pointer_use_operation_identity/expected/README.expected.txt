C6g5r regression target: an ordinary two-function cast-history SCC has a
pointer dereference operation in g.  The cast phase should reach its standard
int* / char* closure first.  The subsequent pointer-use phase must transfer
that dereference through f <-> g as one immutable operation seed per target
formal, rather than repeatedly creating longer attach= histories.

Required properties:
  - cast-history SCC reaches quiescence before pointer-use propagation;
  - pointer-use SCC reaches quiescence before its edge-execution limit;
  - pointer-use operation membership ignores attachment/provenance route growth;
  - a frozen cast-history attachment is used for relayed operation reporting;
  - operation evidence retains char*, int*, and a dereference-write use; and
  - pointer-use changes never reopen cast-history solving.
