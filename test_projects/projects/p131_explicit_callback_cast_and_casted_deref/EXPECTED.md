# C6g5f3 expected semantic contract

The generic `dispatch` summary is intentionally callback-parametric and retains
its callback references:

```text
dispatch($0 : void * => $2.$0,
         $1 : void * => $2.$1,
         $2 : int (*)(void *, void *))
```

Concrete caller histories must specialize those references:

```text
exercise_plain::values       ... dispatch<... int * ...>       # no $2.$0/$2.$1
exercise_casted_int::values  ... dispatch<... int * ...>       # no $2.$0/$2.$1
exercise_casted_strings::values ... dispatch<... char ** ...>  # no $2.$0/$2.$1
```

The explicit cast on the callback actual is a type-adaptation wrapper only. It
must not remain unresolved at the caller. In contrast, a cast from an arbitrary
non-function value to `compare_fn` must not be used to infer a callback target.

`cmpstr` must summarize both parameters as:

```text
cmpstr($0 : void * => char **, $1 : void * => char **)
```

The `char *` type loaded from `*(char **)v1` or `*(char **)v2` is an access
result and must not appear as a second cast-history segment on `v1` or `v2`.
