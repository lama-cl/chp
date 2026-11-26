Expected fptr-extension behavior for p145_fptr_xtu_switch_return_in_loop.

Fact: X14 xtu-switch-return-in-loop

Fact X14 (cross-TU, CFG + RET summary; round 4): lookup() in a.c selects
its return target with a switch; b.c calls lookup() inside a loop and
dispatches the same object x through the result on every iteration.
Needs a CFG-derived return-target summary for lookup ({use_int, use_dbl})
exported cross-TU (D27), and the loop makes the dispatch unrelated (D30).
Today (measured): Opaque, unresolved (no return-target summary).

Subject drive::x:
  today (pinned baseline):  category=OpaqueTransportOrViewOnly callback=UnresolvedCallback behavior=CallbackDependentPointer types=- resolution=unresolved@- sigmismatch=false
  required after extension: category=UnguardedMultiTargetActiveReinterpretation types=double*,int* resolution=set@L0 dispatch=unrelated

Machine check (the @-markers in the .c files are the source of truth;
regenerate this file with --write-project-readmes):
  python3 scripts/check_fptr_evidence.py --only p145_fptr_xtu_switch_return_in_loop
Design: design/fptr-extension/01-fact-catalog.md, 04-implementation-plan.md
