#!/usr/bin/env python3
"""Smoke-test C6g5t exact-input project-index reuse."""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
builder = root / "scripts" / "build_summary_index.py"
with tempfile.TemporaryDirectory(prefix="c6g5t-index-reuse-") as tmp:
    td = Path(tmp)
    summaries = td / "summaries"
    summaries.mkdir()
    # Minimal schema-valid summary: no functions is permitted as a warning.
    (summaries / "a.cast-summary.json").write_text(json.dumps({
        "schemaName": "cast-history-tu-link-summary",
        "schemaVersion": 1,
        "tu": {"sourceFile": "a.c"},
        "functionSummaryDetails": [],
        "summaryDependencyEdges": [],
        "summaryParamTransferEdges": [],
    }), encoding="utf-8")
    out = td / "index.json"
    first = subprocess.run(
        [sys.executable, str(builder), str(summaries), "-o", str(out),
         "--project-kind", "example-suite", "--build-reason", "smoke"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
    )
    assert first.returncode == 0, first.stderr + first.stdout
    data = json.loads(out.read_text(encoding="utf-8"))
    meta = data.get("buildMetadata", {})
    assert meta.get("buildReason") == "smoke", meta
    assert meta.get("inputSummaryFingerprint"), meta
    second = subprocess.run(
        [sys.executable, str(builder), str(summaries), "-o", str(out),
         "--project-kind", "example-suite", "--reuse-output-if-input-unchanged", str(out)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
    )
    assert second.returncode == 0, second.stderr + second.stdout
    assert "reused project summary index" in second.stdout, second.stdout

print("C6g5t index reuse smoke: OK")
