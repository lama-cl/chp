// GATE P0-escape-accounting (TU 1 of 2: forwarding to another project TU)
// `fwd` hands its pointer to a project function defined in another TU that
// produces no interpretation.  The tool's pre-existing externalBoundary flag
// treats this as external; P0 must report it as forwarded-to-project with the
// callee named, so the external bucket is not inflated.
// Found on thttpd-2.29 (shut_down -> httpd_terminate in libhttpd.c).
//
// Probe subjects:
//   fwd.$0 (p)  -- escape.reason forwarded-to-project, projectCallees [sink]
void sink(void *p);

void fwd(void *p) {
  sink(p);
}
