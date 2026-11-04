# Expected behavior: C4P project-level return-summary solving

This project is the cross-TU version of `examples/single-return-summary-compose.c`.

`b.c` defines:

    void *idv(void *x) {
      int *p = x;
      *p = *p + 1;
      return p;
    }

`a.c` calls `idv` and later casts the returned alias:

    void *z = idv(y);
    char *c = z;

The project summary index should materialize a `solvedReturnSummary` for `idv`:

    idv return: $0 => int * => void *    [ret aliases $0]

During the final import run, the caller-side history for `use.$0` / `use::y`
should include the return-alias prefix from `idv` followed by the later cast
through `z`:

    use($0 : void * => int * => void * => char *)
    use::y : void * => int * => void * => char *

The returned local may keep display-only provenance, for example:

    use::z : idv⌜$0 => int * => void *⌝idv => void * => char *

The required invariant is source-to-use ordering:

    void * => int * => void * => char *

not:

    void * => char * => void * => int *
