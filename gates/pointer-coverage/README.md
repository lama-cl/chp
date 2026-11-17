# Pointer-coverage gates

Executable acceptance criteria for `docs/pointer-coverage-plan.md`.

| File | Role |
|---|---|
| `fixtures/` | One minimal C program per coverage gap. The header comment of each file states the probe subjects and what must become true. |
| `gates.json` | Gate manifest: fixture, phase, and control/target checks over paper-facts subjects and report feature diagnostics. |
| `run_gates.py` | Runs C1, index build and one import round per fixture, evaluates checks, ratchets against `baseline.json`. |
| `baseline.json` | Pass/fail of every check on the last sealed state. Only moves forward. |
| `coverage_ledger.py` | Buckets every pointer subject of any paper-facts tree into resolved / blocked:reason / opaque. Used for the corpus gate. |

## Working on a gate

```bash
gates/pointer-coverage/run_gates.py --list
```

```bash
gates/pointer-coverage/run_gates.py --tool build/cast-history-tool --gate P1-fp-fsm -v --keep /tmp/fsm-out
```

`--keep` leaves the tool's text log, summaries, reports and paper facts under
`<dir>/<gate>/{c1,import}/` for inspection.

Exit status: `0` ok, `1` regression or a `--require`d gate is red, `2` the
pipeline failed (tool crash, no output).

## Finishing a phase

1. `run_gates.py --tool ... --require-phase P1` exits 0.
2. `run_gates.py --tool ... --update-baseline` and commit `baseline.json`.
3. Append the phase to `POINTER_COVERAGE_SEALED_PHASES` in `CMakeLists.txt`.
4. Refresh the corpus ledgers (plan section 4.3) and commit them.

## Rules

- Never weaken a target check to make it pass. Renaming an output field is
  fine if the schema change lands in the same commit.
- Control checks guard precision and existing behaviour. A red control is a
  regression even when every target is green.
- New coverage gap found while implementing? Add a fixture and a gate first,
  confirm it is red, then fix it.
