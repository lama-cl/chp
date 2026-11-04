C6g5t guarded local-transformer materialization regression target.

The recursive calls are syntactically outside the if statements, but each
function has one local guarded byte-pointer arithmetic transformer:
  <[tag] ptrarith(unsigned char *), [!(tag)] ptrarith(unsigned char *)>

Required properties:
  - C1 cast history reaches quiescence using finite semantic facts;
  - the canonical C2 report retains the guarded transformer once rather than
    recursively nesting it in Join/Parallel/Seq report trees;
  - the project is local-only, so C2 import/index refinement rounds are skipped
    after a final reporting overlay;
  - pointer-use changes never reopen cast solving; and
  - no rec{...} marker is introduced.

C6g5w1b metadata-only prerequisite:
  - The ordinary f.$0 -> g.$0 transfer edge must remain one edge, but expose
    one guarded-local-ptrarith branch family with two diagnostic arm snapshots:
      [tag]  ptrarith(unsigned char *)
      [!(tag)] ptrarith(unsigned char *)
  - The analogous g.$0 -> f.$0 transfer must expose its own same-shaped
    guarded-local-ptrarith family.
  - This metadata is not yet consumed by SCC solving or Function summaries.

C6g5w1c metadata-only prerequisite:
  - f.$0 and g.$0 keep distinct source family IDs but expose the same
    `recursiveClosureKey`, derived from formal $0, unsigned-char-pointer type,
    and the sorted [tag]/[!(tag)] ptrarith arm signatures.

C6g5w1d authoritative C2 behavior:
  - f.$0 and g.$0 each retain one guarded local transformer:
      [tag] ptrarith(unsigned char *)
      [!(tag)] ptrarith(unsigned char *)
  - no unguarded duplicate ptrarith arm or recursive nested Join is allowed
    in the C2 imported Function summaries.
