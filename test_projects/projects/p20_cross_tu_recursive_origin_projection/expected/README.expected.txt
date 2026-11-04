C4D-full-internal expected behavior for p20_cross_tu_recursive_origin_projection
===============================================================================

After C4c SCC-aware import and recursive parameter-summary solving, function-name
call wrappers that arise only as provenance decorations should be stripped when
computing solved parameter-summary cores and when projecting solved summaries
back to parameter entities.

In particular, summaries/direct histories should be more closely aligned for:
- relay.$0 and relay::x
- f.$0 and f::x
- g.$0 and g::x

A representative improvement is that f::x should no longer stop at only the
prefix void* => char* => void* when the solved parameter summary core contains
additional recursive semantic history imported through relay/g.
