C6g5d1 regression target: ordinary mutual recursion remains on the pre-C6g5d
summary path.

Required properties:

  - f -> g, g -> h, and h -> f are in one recursive same-TU SCC;
  - every one of those edges is recorded-in-if-condition=no;
  - output contains no rec{same-tu-if-scc|...} marker; and
  - the SCC worklist converges without the conditional-recursion widening.
