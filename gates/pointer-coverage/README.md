# Pointer-coverage gates

Executable acceptance criteria for `docs/pointer-coverage-plan.md`.

| File | Role |
|---|---|
| `fixtures/` | One minimal C program per coverage gap. The header comment of each file states the probe subjects and what must become true. |
| `gates.json` | Gate manifest: fixture, phase, and control/target checks over paper-facts subjects and report feature diagnostics. |
| `run_gates.py` | Runs C1, index build and one import round per fixture, evaluates checks, ratchets against `baseline.json`. |
| `baseline.json` | Pass/fail of every check on the last sealed state. Only moves forward. |
| `coverage_ledger.py` | Buckets every pointer subject of any paper-facts tree into resolved / blocked:reason / opaque; ranks escape callees; counts indirect call sites by kind and tier. |
| `corpus.json`, `run_corpus.py`, `ledgers/` | Source-only real-project corpus, its runner, and one committed ledger per project. |
| `review_packet.py`, `review/` | Generates the human review packet for a phase. |
| `signoffs.json` | Human verdict per phase. Reviewers only. |

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

Agent:

1. `run_gates.py --tool ... --require-phase P1` exits 0.
2. `run_corpus.py --tool ...` exits 0, then `--update` to refresh `ledgers/`.
3. `run_gates.py --tool ... --update-baseline`.
4. `review_packet.py --phase P1 --tool ... --before-tool <previous build>`.
5. Commit the code, `baseline.json`, `ledgers/` and `review/P1.md` together.

Human reviewer:

6. Read `review/P1.md`, tick the sample and the questions, and set the phase
   in `signoffs.json` to `approved` or `changes-requested`.
7. On `approved`, append the phase to `POINTER_COVERAGE_SEALED_PHASES` in
   `CMakeLists.txt`. The sealed-phase test fails without an approved sign-off.

## Rules

- Never weaken a target check to make it pass. Renaming an output field is
  fine if the schema change lands in the same commit.
- Control checks guard precision and existing behaviour. A red control is a
  regression even when every target is green.
- Agents never edit `signoffs.json` and never seal a phase.
- New coverage gap found while implementing? Add a fixture and a gate first,
  confirm it is red, then fix it.
