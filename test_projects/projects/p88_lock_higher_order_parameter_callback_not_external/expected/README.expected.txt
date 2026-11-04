C6f1 classification-lock target: higher-order callback mediation, not external.

This locks the C6e1d/C6e1e cleanup. The call pf(pcho) is through a function
parameter. It is higher-order/callback-dependent, but it is not an external
boundary.

Expected core markers:
  ho::pvho should be CallbackDependent / higher-order-dependent.
  demo::pi or demo::pv may aggregate direct-call, higher-order-call,
  higher-order-callback, and return-alias evidence.

  semanticMediation for ho::pvho:
    kind=Callback
    semanticFamily=CallbackDependent
    edges should include higher-order-call
    flags: callback=yes, externalBoundary=no

  semanticMediation for demo::pi / demo::pv, if reported:
    kind=Callback
    semanticFamily=CallbackDependent
    edges may include direct-call, higher-order-call, higher-order-callback
    flags: callback=yes, externalBoundary=no, returnAlias=yes

Negative lock markers:
  ho::pvho, ho::pcho, demo::pi, and demo::pv should not report
  edges=external-direct-call when externalBoundary=no.
