C6g4y4 projected origin-state insertion/merge regression

Source-local reaching origins are projected at every originState and
fieldOriginState write.  The projected key retains source entity / field path
and normalized attachment shape, while excluding accumulated route guards and
transient transfer-token order.

When two alternatives share that key but have different guards, the retained
state is widened conservatively to an unguarded may-reach alternative.  Up to
four original guards are retained only as progress diagnostics.  Applied
transfer tokens are unioned on the representative to keep arithmetic replay
suppression effective after projection.

With --analysis-progress, C6g4y4 reports:
  ast-local-origin-state-raw-alternatives
  ast-local-origin-state-unique-projected-alternatives
  ast-local-origin-state-coalesced-alternatives
  ast-local-origin-state-guard-projected
  ast-local-origin-state-transfer-token-unions
  ast-local-origin-state-witnesses-retained
  ast-local-origin-state-witnesses-truncated

C6g4y3 origin-batch counters remain as a downstream verification backstop.
After C6g4y4, the batch raw count should remain close to the projected unique
count rather than growing into millions.
