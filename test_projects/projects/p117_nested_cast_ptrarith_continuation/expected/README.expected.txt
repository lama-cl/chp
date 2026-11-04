C6g4z4 shared higher-order identity specialization regression

Both functions assign a comparator argument through a char-pointer arithmetic
expression and then pass the stored local to a function-pointer comparator.
The only difference is the minimap2-style inner void* cast:

  simple:      (const void *)((const char *)base + width)
  nested-void: (const void *)((void *)((const char *)base + width))

Required generic callee summaries remain symbolic:

  bsearch_simple.$1:
    const void * => const char * => ptrarith(const char *) => const void *
    => $3.$1

  bsearch_nested_void.$1:
    const void * => const char * => ptrarith(const char *) => void *
    => const void * => $3.$1

At run_nested_cast_ptrarith, however, compare_bytes is a unique concrete
callback with no parameter cast summaries.  Its $3.$0/$3.$1 effects are
cast-history identity.  The caller summary must therefore contain exactly the
two meaningful non-identity arms:

  run_nested_cast_ptrarith.$0:
    const void * => <|
      const char * => ptrarith(const char *) => const void *,
      const char * => ptrarith(const char *) => void * => const void *
    |>

It must not contain $3.$0, $3.$1, or
<unknown:compare_bytes.$i-unsummarized>.  The two cast-free key arguments may
remain in per-call provenance but contribute no semantic cast-history arm.


C6g4z4b edge-snapshot invariant

The two direct calls are independent uses of the original `base` formal.
The second arm must not be prefixed by the first call's wrapper, for example:

  bsearch_simple⌜...⌝bsearch_simple => const char * => ...

Such a chain signals that the second NamedCallEdge reused mutable caller entity
observations instead of its own immutable `originsAtCall` snapshot.

The final import pass may consume a project index, but it must not overwrite
this fully specialized caller result with an older solvedParamSummaries record
that still contains a generic higher-order formal placeholder.  The generic
`$3.$i` forms remain valid only in the bsearch_* callee summaries.
