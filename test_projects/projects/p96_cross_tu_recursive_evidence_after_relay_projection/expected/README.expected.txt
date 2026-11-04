C6g4o1 regression target: cross-TU project-SCC recursive evidence after C6g4n relay projection.

Cycle:
  relay (a.c) -> g (b.c) -> f (a.c) -> relay (a.c)

Expected after the source-only/index/import rounds used by p18:
  - relay::$0 and f::$0 evidenceSources include same-tu-recursive-pointer-use
  - evidenceFlags report sameTURecursive=yes
  - project-scc-pointer-use/imported-direct-call evidence remains present
  - detail may omit the older "recursive via relay.$0" clause; recursion is
    carried by provenance.recursiveNamedCallEvidence instead.
