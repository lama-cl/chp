C4Q expected behavior: pointer-use continuation through imported returned alias.

Relevant code:

  void *z = idv(y);   // idv returns an alias of y through int * => void *
  int *p = z;
  *p += 1;

C4P/C4P3 should already propagate the cast-history continuation back to y:

  use::y : void * => int * => void * => int *

C4Q additionally propagates the pointer-use payload through the same returned-alias edge.
The pointer-use summaries for use::y should include deref-readwrite evidence whose
semantic attachment contains the wrapper-free returned-alias core, for example:

  attach=int * => void * => int *, as=int *, access=int : deref-readwrite
    {return-alias-continuation; ...}

The exact raw report may also retain a display/provenance-rich attachment such as
idv⌜$0 => int * => void *⌝idv => int *.  The important invariant is that behavior
classification for use::y can see the write through the returned alias, without
encoding the pointer-use as a fake cast and without feeding call wrappers back into
project-level solver summaries.
