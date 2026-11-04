C6f1 classification-lock target: recursive direct call should not become ExternalBoundary.

This locks the C6e1f cleanup. The target f may appear in low-level evidence as
a decorated synthetic target such as f-alias-parent, but the real callee is the
local function f.

Expected core markers:
  Direct cast histories:
    f::x should include the local byte/concrete reinterpretation evidence.

  Pointer behavior metrics for f::x:
    behavior: ConcreteSpecializedWrite or equivalent concrete-write behavior
    paperWildness: semanticFamily=ConcreteSpecialization
    semanticMediation should not be kind=ExternalBoundary
    flags should include externalBoundary=no

Negative lock markers:
  f::x should not report:
    semanticMediation: kind=ExternalBoundary
    semanticFamily=ExternalUnknownBoundary
    targets=f-alias-parent
    edges=external-direct-call
