C6g5f paper-facts and directional array-index regression target.

Run the analyzer with both --paper-facts-dir and --summary-report-dir. The
paper-facts sidecar must contain a subject with these operation facts:

  - arrayIndexRead=true for p[0];
  - arrayIndexWrite=true for p[1] = first;
  - arrayIndexReadWrite=true for p[2] += 1;
  - pointerArithmetic=true for p = p + 1;
  - no legacy direction-ambiguous array-index fact for the simple cases.

The callback-use relation must retain the resolved callback target and the
active dereference in consume_int. Exact history rendering remains deliberately
out of scope for this compact paper-facts regression.
