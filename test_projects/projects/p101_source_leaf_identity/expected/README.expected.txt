C6g4s/C6g4t regression target: immutable source-leaf identity,
operation-local-guard provenance, and transfer-span-independent named-call
solver membership.

Required source-leaf behavior:
  1. The `*ip += 1` operation in shared_leaf has one immutable
     operationSiteSpan and one operationLocalGuard (unconditional), even though
     shared_leaf is reached once with p and once through alias.
  2. The `*ip += 1` operation in the depth > 0 arm and the `*cp += 1` operation
     in the else arm remain distinct operationSiteSpan values after both flow
     through recursive_head and export_recursive_summary.
  3. The named-call diagnostic for the acyclic caller -> recursive callee export
     prints both the mutable transfer/report span and source-op-loc.  Candidate
     grouping must use source-op-loc plus leaf detail, use kind, pointer type,
     and access type, rather than the mutable transfer span alone.
  4. C6g4t named-call solver membership must replace the mutable transfer span
     with operationSiteSpanForPointerUse().  Thus, records sharing the same
     immutable operation site, normalized attachment, type/access/kind,
     projected detail, membership guard, and projected provenance must occupy
     one solver record even when they arrived through different transfer spans.
     Existing legacy records without a usable operationSiteSpan retain their
     former transfer-span behavior through the explicit fallback.
  5. The representative keeps a reporting span, while namedCallRelay witnesses
     retain a bounded `transfer-span=<file>:<line>:<column>; ...` record for
     distinct merged routes.  Transfer span must not appear as a reported
     solver-membership-difference component after C6g4t.
  6. Direct AST records use operationLocalGuardOrigin=ast-leaf.  Named-call,
     function-pointer, data-field, and project-SCC transfers preserve the same
     operationSiteSpan and report operationLocalGuardOrigin=preserved-transfer.
  7. If an old/imported summary lacks operationSiteSpan or operationLocalGuard,
     C6g4s/C6g4t retain prior behavior but expose legacy-fallback counters.

Scope boundary:
  - C6g4t changes only same-TU named-call relay membership and the
    diagnostic-only semantic key.  It replaces a mutable transfer-span key
    component with the existing immutable operationSiteSpan; it does not add a
    new key dimension.
  - Project-SCC Python solver membership, non-named-call pointer-use insertion,
    guard projection, higher-order specialization, and recursive-evidence
    behavior are unchanged.
