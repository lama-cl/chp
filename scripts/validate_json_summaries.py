#!/usr/bin/env python3
"""Validate C6g5e compact link and verbose report JSON artifacts.

This validator intentionally does not warn about JSON object key order,
because JSON object order is not semantically meaningful.

Typical use from a tool/version root:
  python3 scripts/validate_json_summaries.py summaries reports

Typical use from a build directory:
  python3 ../scripts/validate_json_summaries.py ../json-summaries

Optional weak cross-check against text output:
  python3 scripts/validate_json_summaries.py json-summaries --text-output-dir example-output
"""
from __future__ import annotations

import argparse
import json
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any

LINK_SCHEMA_NAME = "cast-history-tu-link-summary"
REPORT_SCHEMA_NAME = "cast-history-tu-report"
LEGACY_SCHEMA_NAME = "cast-history-tu-summary"

REQUIRED_TOP_LEVEL = [
    "schemaVersion",
    "schemaName",
    "tu",
    "directCastHistories",
    "pointerBehaviorMetrics",
    "functionSummaries",
    "featureDiagnostics",
]

LINK_REQUIRED_TOP_LEVEL = [
    "schemaVersion",
    "schemaName",
    "tu",
    "functionSummaryDetails",
    "summaryDependencyEdges",
    "summaryParamTransferEdges",
    "returnAliasPointerUseContinuations",
]

RECOMMENDED_TOP_LEVEL = [
    "toolVersion",
    "flowRecords",
    "storageCastHistories",
    "observedInterpretations",
    "derivedObservationSummaries",
    "pointerUseSummaries",
    "storageHistoryMetrics",
    "functionParameterBehaviorMetrics",
    "functionBehaviorClassifications",
    "functionSummaryDetails",
]

ARRAY_SECTIONS = [
    "flowRecords",
    "directCastHistories",
    "storageCastHistories",
    "observedInterpretations",
    "derivedObservationSummaries",
    "pointerUseSummaries",
    "pointerBehaviorMetrics",
    "storageHistoryMetrics",
    "functionParameterBehaviorMetrics",
    "functionBehaviorClassifications",
    "functionSummaries",
    "functionSummaryDetails",
    "featureDiagnostics",
]

@dataclass
class Issue:
    severity: str
    file: Path
    path: str
    message: str

class Validator:
    def __init__(self, *, strict: bool, check_pretty: bool, text_output_dir: Path | None, max_issues_per_file: int) -> None:
        self.strict = strict
        self.check_pretty = check_pretty
        self.text_output_dir = text_output_dir
        self.max_issues_per_file = max_issues_per_file
        self.issues: list[Issue] = []
        self.issue_counts: Counter[Path] = Counter()
        self.current_data_by_file: dict[Path, dict[str, Any]] = {}

    def error(self, file: Path, path: str, msg: str) -> None:
        self._add("error", file, path, msg)

    def warn(self, file: Path, path: str, msg: str) -> None:
        self._add("warning", file, path, msg)

    def _add(self, severity: str, file: Path, path: str, msg: str) -> None:
        if self.issue_counts[file] >= self.max_issues_per_file:
            if self.issue_counts[file] == self.max_issues_per_file:
                self.issues.append(Issue("warning", file, "$", f"too many issues; suppressing further issues after {self.max_issues_per_file}"))
                self.issue_counts[file] += 1
            return
        self.issues.append(Issue(severity, file, path, msg))
        self.issue_counts[file] += 1

    def validate_file(self, file: Path) -> None:
        text = file.read_text(encoding="utf-8", errors="replace")
        if not text.strip():
            self.error(file, "$", "file is empty")
            return
        if self.check_pretty and "\n" not in text.rstrip("\n"):
            self.warn(file, "$", "JSON appears to be one line; expected pretty-printed summary JSON")
        try:
            data = json.loads(text)
        except json.JSONDecodeError as exc:
            self.error(file, "$", f"invalid JSON: {exc}")
            return
        if not isinstance(data, dict):
            self.error(file, "$", f"top-level JSON must be object, got {type(data).__name__}")
            return
        self.current_data_by_file[file] = data

        schema_name = data.get("schemaName")
        if schema_name == LINK_SCHEMA_NAME:
            self.validate_link_summary(file, data)
            return

        self.validate_top_level(file, data)
        self.validate_tu(file, data.get("tu"))
        self.validate_direct_cast_histories(file, data.get("directCastHistories"))
        self.validate_storage_cast_histories(file, data.get("storageCastHistories"))
        self.validate_pointer_use_summaries(file, data.get("pointerUseSummaries"))
        self.validate_pointer_behavior_metrics(file, data.get("pointerBehaviorMetrics"))
        self.validate_storage_history_metrics(file, data.get("storageHistoryMetrics"))
        self.validate_function_summaries(file, data)
        self.validate_feature_diagnostics(file, data.get("featureDiagnostics"))
        if self.text_output_dir is not None:
            self.cross_check_text_output(file, data)

    def validate_top_level(self, file: Path, data: dict[str, Any]) -> None:
        schema_name = data.get("schemaName")
        if schema_name not in {REPORT_SCHEMA_NAME, LEGACY_SCHEMA_NAME}:
            self.error(file, "$.schemaName",
                       f"unsupported report schemaName: {schema_name!r}")
        for key in REQUIRED_TOP_LEVEL:
            if key not in data:
                self.error(file, "$", f"missing required top-level section: {key}")
        for key in RECOMMENDED_TOP_LEVEL:
            # C6g5e stores import payloads in the sibling link artifact rather
            # than repeating them in the verbose report.
            if schema_name == REPORT_SCHEMA_NAME and key == "functionSummaryDetails":
                continue
            if key not in data:
                self.warn(file, "$", f"missing recommended top-level section: {key}")
        for key in ARRAY_SECTIONS:
            if key in data and not isinstance(data[key], list):
                self.error(file, f"$.{key}", f"section must be array, got {type(data[key]).__name__}")
        if "schemaVersion" in data and not isinstance(data["schemaVersion"], int):
            self.error(file, "$.schemaVersion", "schemaVersion must be integer")
        if schema_name == REPORT_SCHEMA_NAME:
            link = data.get("linkSummary")
            if link is not None and not isinstance(link, dict):
                self.error(file, "$.linkSummary", "linkSummary must be object or null")
            if isinstance(link, dict):
                if not isinstance(link.get("schemaName"), str):
                    self.error(file, "$.linkSummary.schemaName", "schemaName must be string")
                if not isinstance(link.get("fileName"), str):
                    self.error(file, "$.linkSummary.fileName", "fileName must be string")
                if "publishedUnder" in link and not isinstance(link.get("publishedUnder"), str):
                    self.error(file, "$.linkSummary.publishedUnder", "publishedUnder must be string")

    def validate_link_history_summary(self, file: Path, path: str, obj: Any) -> None:
        if not isinstance(obj, dict):
            self.error(file, path, "link history summary must be object")
            return
        if "ir" not in obj:
            self.error(file, path + ".ir", "link history summary must contain structured IR")
        if "rendered" in obj:
            self.warn(file, path + ".rendered",
                      "compact link summary should not duplicate rendered history text")

    def validate_link_pointer_use(self, file: Path, path: str, use: Any) -> None:
        if not isinstance(use, dict):
            self.error(file, path, "pointer use must be object")
            return
        for key in ["kind", "pointerType", "detail"]:
            if key not in use:
                self.error(file, path, f"missing key: {key}")
            elif not isinstance(use[key], str):
                self.error(file, path + f".{key}", f"{key} must be string")
        loc_key = "location" if "location" in use else "span"
        if loc_key in use:
            self.validate_location(file, path + f".{loc_key}", use[loc_key])
        else:
            self.warn(file, path, "pointer use missing location/span")
        self.validate_link_history_summary(file, path + ".attachPath", use.get("attachPath"))

    def validate_link_summary(self, file: Path, data: dict[str, Any]) -> None:
        for key in LINK_REQUIRED_TOP_LEVEL:
            if key not in data:
                self.error(file, "$", f"missing required link-summary section: {key}")
        self.validate_tu(file, data.get("tu"))
        if not isinstance(data.get("schemaVersion"), int):
            self.error(file, "$.schemaVersion", "schemaVersion must be integer")

        for section in ["summaryDependencyEdges", "summaryParamTransferEdges",
                        "returnAliasPointerUseContinuations"]:
            value = data.get(section)
            if value is not None and not isinstance(value, list):
                self.error(file, f"$.{section}", "section must be array")

        details = data.get("functionSummaryDetails")
        if not isinstance(details, list):
            return
        for i, detail in enumerate(details):
            path = f"$.functionSummaryDetails[{i}]"
            if not isinstance(detail, dict):
                self.error(file, path, "detail must be object")
                continue
            self.require_string(file, path, detail, "function")
            identity = detail.get("identity")
            if not isinstance(identity, dict):
                self.error(file, path + ".identity", "identity must be object")
            else:
                for key in ["qualifiedName", "signatureKey", "linkage", "sourceFile"]:
                    self.require_string(file, path + ".identity", identity, key)
            params = detail.get("params")
            if not isinstance(params, list):
                self.error(file, path + ".params", "params must be array")
                continue
            for j, param in enumerate(params):
                ppath = f"{path}.params[{j}]"
                if not isinstance(param, dict):
                    self.error(file, ppath, "param must be object")
                    continue
                if not isinstance(param.get("index"), int):
                    self.error(file, ppath + ".index", "index must be integer")
                self.require_string(file, ppath, param, "declaredType")
                self.validate_link_history_summary(file, ppath + ".summary", param.get("summary"))
                for uses_key in ["pointerUses", "dataFieldPointerUses", "dataFieldObservations"]:
                    if uses_key in param and not isinstance(param[uses_key], list):
                        self.error(file, ppath + f".{uses_key}", "must be array")
                for k, use in enumerate(param.get("pointerUses", []) if isinstance(param.get("pointerUses"), list) else []):
                    self.validate_link_pointer_use(file, f"{ppath}.pointerUses[{k}]", use)
                for k, item in enumerate(param.get("dataFieldPointerUses", []) if isinstance(param.get("dataFieldPointerUses"), list) else []):
                    upath = f"{ppath}.dataFieldPointerUses[{k}]"
                    if not isinstance(item, dict):
                        self.error(file, upath, "entry must be object")
                    else:
                        self.validate_link_pointer_use(file, upath + ".use", item.get("use"))
                for k, item in enumerate(param.get("dataFieldObservations", []) if isinstance(param.get("dataFieldObservations"), list) else []):
                    opath = f"{ppath}.dataFieldObservations[{k}]"
                    if not isinstance(item, dict):
                        self.error(file, opath, "entry must be object")
                    else:
                        self.validate_link_history_summary(file, opath + ".summary", item.get("summary"))
                        self.validate_link_history_summary(file, opath + ".attachPath", item.get("attachPath"))
            ret = detail.get("return")
            if not isinstance(ret, dict):
                self.error(file, path + ".return", "return must be object")
            else:
                self.require_string(file, path + ".return", ret, "declaredType")
                self.validate_link_history_summary(file, path + ".return.summary", ret.get("summary"))

    def validate_tu(self, file: Path, tu: Any) -> None:
        if tu is None:
            return
        if not isinstance(tu, dict):
            self.error(file, "$.tu", "tu must be object")
            return
        source = tu.get("sourceFile")
        if not isinstance(source, str) or not source:
            self.warn(file, "$.tu.sourceFile", "missing or empty sourceFile")
        for key in ["configurationId", "compileCommandHash"]:
            if key in tu and not isinstance(tu[key], str):
                self.error(file, f"$.tu.{key}", f"{key} must be string")

    def validate_direct_cast_histories(self, file: Path, section: Any) -> None:
        if not isinstance(section, list):
            return
        seen = Counter()
        for i, item in enumerate(section):
            path = f"$.directCastHistories[{i}]"
            if not isinstance(item, dict):
                self.error(file, path, "entry must be object")
                continue
            entity = item.get("entity")
            history = item.get("history")
            seen[entity] += 1
            if not isinstance(entity, str) or not entity:
                self.error(file, path + ".entity", "entity must be nonempty string")
            if not isinstance(history, str) or not history:
                self.error(file, path + ".history", "history must be nonempty string")
            elif history == "<none>":
                self.warn(file, path + ".history", "direct cast history is <none>")
            self.validate_components(file, path + ".components", item.get("components"))
        for entity, n in seen.items():
            if entity and n > 1:
                self.warn(file, "$.directCastHistories", f"duplicate entity: {entity!r}")

    def validate_storage_cast_histories(self, file: Path, section: Any) -> None:
        if section is None or not isinstance(section, list):
            return
        for i, item in enumerate(section):
            path = f"$.storageCastHistories[{i}]"
            if not isinstance(item, dict):
                self.error(file, path, "entry must be object")
                continue
            self.require_string(file, path, item, "entity")
            self.require_string(file, path, item, "history")
            self.validate_components(file, path + ".components", item.get("components"))

    def validate_pointer_use_summaries(self, file: Path, section: Any) -> None:
        if section is None or not isinstance(section, list):
            return
        for i, item in enumerate(section):
            path = f"$.pointerUseSummaries[{i}]"
            if not isinstance(item, dict):
                self.error(file, path, "entry must be object")
                continue
            self.require_string(file, path, item, "entity")
            uses = item.get("uses")
            if not isinstance(uses, list):
                self.error(file, path + ".uses", "uses must be array")
                continue
            for j, use in enumerate(uses):
                self.validate_pointer_use(file, f"{path}.uses[{j}]", use, compact_allowed=False)

    def validate_pointer_behavior_metrics(self, file: Path, section: Any) -> None:
        if not isinstance(section, list):
            return
        direct_entities = self.entities_in_section(file, "directCastHistories")
        seen = set()
        for i, item in enumerate(section):
            path = f"$.pointerBehaviorMetrics[{i}]"
            if not isinstance(item, dict):
                self.error(file, path, "entry must be object")
                continue
            entity = item.get("entity")
            if isinstance(entity, str) and entity:
                seen.add(entity)
            else:
                self.error(file, path + ".entity", "entity must be nonempty string")
            for key in ["view", "historyShape", "historyRisk", "behavior"]:
                self.require_string(file, path, item, key)
            if "wildLike" in item and not isinstance(item["wildLike"], bool):
                self.error(file, path + ".wildLike", "wildLike must be boolean")
            self.validate_history_metrics(file, path + ".metrics", item.get("metrics"))
        missing = sorted(direct_entities - seen)
        if missing:
            self.warn(file, "$.pointerBehaviorMetrics", f"{len(missing)} direct cast entities have no behavior metrics entry")

    def validate_storage_history_metrics(self, file: Path, section: Any) -> None:
        if section is None or not isinstance(section, list):
            return
        for i, item in enumerate(section):
            path = f"$.storageHistoryMetrics[{i}]"
            if not isinstance(item, dict):
                self.error(file, path, "entry must be object")
                continue
            self.require_string(file, path, item, "entity")
            self.validate_history_metrics(file, path + ".metrics", item.get("metrics"))

    def validate_function_summaries(self, file: Path, data: dict[str, Any]) -> None:
        readable = data.get("functionSummaries")
        details = data.get("functionSummaryDetails")
        if not isinstance(readable, list):
            return
        detail_by_name: dict[str, list[dict[str, Any]]] = defaultdict(list)
        if details is None:
            if data.get("schemaName") != REPORT_SCHEMA_NAME:
                self.warn(file, "$.functionSummaryDetails", "missing detail section; compact summaries cannot be cross-checked")
        elif not isinstance(details, list):
            self.error(file, "$.functionSummaryDetails", "must be array")
        else:
            for d in details:
                if isinstance(d, dict) and isinstance(d.get("function"), str):
                    detail_by_name[d["function"]].append(d)
        seen = Counter()
        for i, fn in enumerate(readable):
            path = f"$.functionSummaries[{i}]"
            if not isinstance(fn, dict):
                self.error(file, path, "entry must be object")
                continue
            name = fn.get("function")
            seen[name] += 1
            for key in ["function", "signature", "functionBehavior", "story"]:
                self.require_string(file, path, fn, key)
            if "identity" in fn:
                self.warn(file, path + ".identity", "readable summary contains identity; prefer compact location/signature")
            if "declaredTypeAtom" in fn:
                self.warn(file, path + ".declaredTypeAtom", "readable summary should not expand TypeAtom")
            params = fn.get("params")
            if not isinstance(params, list):
                self.error(file, path + ".params", "params must be array")
                params = []
            for j, param in enumerate(params):
                self.validate_readable_param(file, f"{path}.params[{j}]", param)
            ret = fn.get("return")
            if not isinstance(ret, dict):
                self.error(file, path + ".return", "return must be object")
            else:
                self.validate_readable_return(file, path + ".return", ret)
            if isinstance(name, str) and name:
                matches = detail_by_name.get(name, [])
                if details is not None and not matches:
                    self.warn(file, path, f"no matching functionSummaryDetails for {name!r}")
                elif len(matches) == 1:
                    self.cross_check_function_detail(file, path, fn, matches[0])
                elif len(matches) > 1:
                    self.warn(file, path, f"multiple detail entries for {name!r}; skipping detail cross-check")
        for name, n in seen.items():
            if isinstance(name, str) and n > 1:
                self.warn(file, "$.functionSummaries", f"duplicate function name: {name!r}")

    def validate_readable_param(self, file: Path, path: str, param: Any) -> None:
        if not isinstance(param, dict):
            self.error(file, path, "param must be object")
            return
        for key in ["index", "name", "declaredType", "declaredTypeKey", "summary", "role", "view", "historyShape", "historyRisk", "behavior"]:
            if key not in param:
                self.error(file, path, f"missing key: {key}")
        if "index" in param and not isinstance(param["index"], int):
            self.error(file, path + ".index", "index must be integer")
        for key in ["declaredType", "declaredTypeKey", "summary", "role", "view", "historyShape", "historyRisk", "behavior"]:
            if key in param and not isinstance(param[key], str):
                self.error(file, path + f".{key}", f"{key} must be string")
        if "wildLike" in param and not isinstance(param["wildLike"], bool):
            self.error(file, path + ".wildLike", "wildLike must be boolean")
        if "declaredTypeAtom" in param:
            self.warn(file, path + ".declaredTypeAtom", "readable param expands TypeAtom; belongs in functionSummaryDetails")
        self.validate_compact_metrics(file, path + ".metrics", param.get("metrics"))
        uses = param.get("pointerUses")
        if uses is not None:
            if not isinstance(uses, list):
                self.error(file, path + ".pointerUses", "pointerUses must be array")
            else:
                for i, use in enumerate(uses):
                    self.validate_pointer_use(file, f"{path}.pointerUses[{i}]", use, compact_allowed=True)

    def validate_readable_return(self, file: Path, path: str, ret: dict[str, Any]) -> None:
        for key in ["declaredType", "declaredTypeKey", "summary", "mayAliasParam", "aliasedParamIndex"]:
            if key not in ret:
                self.error(file, path, f"missing key: {key}")
        for key in ["declaredType", "declaredTypeKey", "summary"]:
            if key in ret and not isinstance(ret[key], str):
                self.error(file, path + f".{key}", f"{key} must be string")
        if "mayAliasParam" in ret and not isinstance(ret["mayAliasParam"], bool):
            self.error(file, path + ".mayAliasParam", "mayAliasParam must be boolean")
        if ret.get("mayAliasParam") is True and ret.get("aliasedParamIndex") is None:
            self.warn(file, path + ".aliasedParamIndex", "mayAliasParam is true but aliasedParamIndex is null")
        if "declaredTypeAtom" in ret:
            self.warn(file, path + ".declaredTypeAtom", "readable return expands TypeAtom; belongs in functionSummaryDetails")

    def cross_check_function_detail(self, file: Path, path: str, compact: dict[str, Any], detail: dict[str, Any]) -> None:
        if compact.get("functionBehavior") != detail.get("functionBehavior"):
            self.error(file, path + ".functionBehavior", f"compact/detail mismatch: {compact.get('functionBehavior')!r} vs {detail.get('functionBehavior')!r}")
        cp = compact.get("params")
        dp = detail.get("params")
        if isinstance(cp, list) and isinstance(dp, list):
            details = {p.get("index"): p for p in dp if isinstance(p, dict) and isinstance(p.get("index"), int)}
            for p in cp:
                if not isinstance(p, dict) or not isinstance(p.get("index"), int):
                    continue
                idx = p["index"]
                d = details.get(idx)
                if d is None:
                    self.error(file, path + f".params[{idx}]", "missing matching detail param")
                    continue
                compact_summary = p.get("summary")
                detail_summary = rendered_summary(d.get("summary"))
                if isinstance(compact_summary, str) and detail_summary is not None and compact_summary != detail_summary:
                    self.error(file, path + f".params[{idx}].summary", f"compact/detail summary mismatch: {compact_summary!r} vs {detail_summary!r}")
                if p.get("behavior") != d.get("behavior"):
                    self.error(file, path + f".params[{idx}].behavior", f"compact/detail behavior mismatch: {p.get('behavior')!r} vs {d.get('behavior')!r}")
        cr = compact.get("return")
        dr = detail.get("return")
        if isinstance(cr, dict) and isinstance(dr, dict):
            compact_summary = cr.get("summary")
            detail_summary = rendered_summary(dr.get("summary"))
            if isinstance(compact_summary, str) and detail_summary is not None and compact_summary != detail_summary:
                self.error(file, path + ".return.summary", f"compact/detail return summary mismatch: {compact_summary!r} vs {detail_summary!r}")
            for key in ["mayAliasParam", "aliasedParamIndex"]:
                if cr.get(key) != dr.get(key):
                    self.error(file, path + f".return.{key}", f"compact/detail return {key} mismatch")

    def validate_feature_diagnostics(self, file: Path, section: Any) -> None:
        if not isinstance(section, list):
            return
        for i, item in enumerate(section):
            path = f"$.featureDiagnostics[{i}]"
            if not isinstance(item, dict):
                self.error(file, path, "entry must be object")
                continue
            for key in ["severity", "feature", "reason", "location"]:
                if key not in item:
                    self.error(file, path, f"missing key: {key}")
            if item.get("severity") not in {"approximated", "unsupported"}:
                self.warn(file, path + ".severity", f"unexpected severity: {item.get('severity')!r}")

    def validate_components(self, file: Path, path: str, components: Any) -> None:
        if components is None:
            self.warn(file, path, "missing components")
            return
        if not isinstance(components, dict):
            self.error(file, path, "components must be object")
            return
        if "tail" in components:
            self.validate_summary_object(file, path + ".tail", components["tail"])
        else:
            self.warn(file, path + ".tail", "components missing tail")

    def validate_summary_object(self, file: Path, path: str, obj: Any) -> None:
        if not isinstance(obj, dict):
            self.error(file, path, "summary must be object")
            return
        if not isinstance(obj.get("rendered"), str):
            self.error(file, path + ".rendered", "rendered must be string")
        if "ir" not in obj:
            self.warn(file, path + ".ir", "missing structured IR")

    def validate_pointer_use(self, file: Path, path: str, use: Any, *, compact_allowed: bool) -> None:
        if not isinstance(use, dict):
            self.error(file, path, "pointer use must be object")
            return
        for key in ["kind", "pointerType", "detail"]:
            if key not in use:
                self.error(file, path, f"missing key: {key}")
            elif not isinstance(use[key], str):
                self.error(file, path + f".{key}", f"{key} must be string")
        loc_key = "location" if "location" in use else "span"
        if loc_key in use:
            self.validate_location(file, path + f".{loc_key}", use[loc_key])
        else:
            self.warn(file, path, "pointer use missing location/span")
        # C6g4s fields remain optional so old exported summaries continue to
        # validate, but emitted source-leaf metadata must be well formed.
        if "operationSiteSpan" in use:
            self.validate_location(file, path + ".operationSiteSpan",
                                   use["operationSiteSpan"])
        if "operationLocalGuardOrigin" in use:
            origin = use["operationLocalGuardOrigin"]
            if not isinstance(origin, str):
                self.error(file, path + ".operationLocalGuardOrigin",
                           "operationLocalGuardOrigin must be string")
            elif origin not in {"ast-leaf", "preserved-transfer",
                                "imported-summary", "legacy-fallback"}:
                self.warn(file, path + ".operationLocalGuardOrigin",
                          f"unexpected origin: {origin!r}")
        if "attachPath" in use:
            attach = use["attachPath"]
            if compact_allowed and isinstance(attach, str):
                return
            self.validate_summary_object(file, path + ".attachPath", attach)

    def validate_history_metrics(self, file: Path, path: str, metrics: Any) -> None:
        if not isinstance(metrics, dict):
            self.error(file, path, "metrics must be object")
            return
        for n_key, arr_key in [("numDistinctTypes", "distinctTypes"), ("numPointerTypes", "pointerTypes"), ("numNonVoidPointerTypes", "nonVoidPointerTypes")]:
            if n_key in metrics and arr_key in metrics:
                self.check_count_matches_array(file, path, metrics, n_key, arr_key)
        for key in ["seqDepth", "maxParallelBreadth", "maxBranchBreadth", "numSeqNodes", "numParallelNodes", "numBranchNodes", "numCallWraps", "numPtrArithNodes"]:
            if key in metrics and not isinstance(metrics[key], int):
                self.error(file, path + f".{key}", f"{key} must be integer")

    def validate_compact_metrics(self, file: Path, path: str, metrics: Any) -> None:
        if not isinstance(metrics, dict):
            self.error(file, path, "metrics must be object")
            return
        for n_key, arr_key in [("numPointerTypes", "pointerTypes"), ("numNonVoidPointerTypes", "nonVoidPointerTypes")]:
            if n_key in metrics and arr_key in metrics:
                self.check_count_matches_array(file, path, metrics, n_key, arr_key)
        for key in ["seqDepth", "numDistinctTypes", "numPointerTypes", "numNonVoidPointerTypes"]:
            if key in metrics and not isinstance(metrics[key], int):
                self.error(file, path + f".{key}", f"{key} must be integer")

    def check_count_matches_array(self, file: Path, path: str, obj: dict[str, Any], n_key: str, arr_key: str) -> None:
        n = obj.get(n_key)
        arr = obj.get(arr_key)
        if not isinstance(n, int):
            self.error(file, path + f".{n_key}", f"{n_key} must be integer")
            return
        if not isinstance(arr, list):
            self.error(file, path + f".{arr_key}", f"{arr_key} must be array")
            return
        if n != len(arr):
            self.error(file, path, f"{n_key}={n} but len({arr_key})={len(arr)}")

    def validate_location(self, file: Path, path: str, loc: Any) -> None:
        if not isinstance(loc, dict):
            self.error(file, path, "location must be object")
            return
        if "file" in loc and not isinstance(loc["file"], str):
            self.error(file, path + ".file", "file must be string")
        for key in ["line", "column"]:
            if key in loc and not isinstance(loc[key], int):
                self.error(file, path + f".{key}", f"{key} must be integer")

    def require_string(self, file: Path, path: str, obj: dict[str, Any], key: str) -> None:
        if key not in obj:
            self.error(file, path, f"missing key: {key}")
        elif not isinstance(obj[key], str) or obj[key] == "":
            self.error(file, path + f".{key}", f"{key} must be nonempty string")

    def entities_in_section(self, file: Path, section_name: str) -> set[str]:
        data = self.current_data_by_file.get(file, {})
        section = data.get(section_name)
        if not isinstance(section, list):
            return set()
        return {item["entity"] for item in section if isinstance(item, dict) and isinstance(item.get("entity"), str)}

    def cross_check_text_output(self, file: Path, data: dict[str, Any]) -> None:
        if self.text_output_dir is None:
            return
        stem = file.name
        if stem.endswith(".cast-report.json"):
            base = stem[:-len(".cast-report.json")]
        elif stem.endswith(".cast-summary.json"):
            base = stem[:-len(".cast-summary.json")]
        elif stem.endswith(".json"):
            base = stem[:-len(".json")]
        else:
            base = file.stem
        candidates = [self.text_output_dir / f"{base}-output.txt", self.text_output_dir / f"{base}.txt"]
        out_file = next((p for p in candidates if p.exists()), None)
        if out_file is None:
            self.warn(file, "$", f"no matching text output found in {self.text_output_dir}")
            return
        text = out_file.read_text(encoding="utf-8", errors="replace")
        for i, fn in enumerate(data.get("functionSummaries", []) if isinstance(data.get("functionSummaries"), list) else []):
            if not isinstance(fn, dict):
                continue
            name = fn.get("function")
            behavior = fn.get("functionBehavior")
            if isinstance(name, str) and name and name not in text:
                self.warn(file, f"$.functionSummaries[{i}].function", f"function name {name!r} not found in text output")
            if isinstance(behavior, str) and behavior and behavior not in text:
                self.warn(file, f"$.functionSummaries[{i}].functionBehavior", f"behavior {behavior!r} not found in text output")
        for i, item in enumerate(data.get("directCastHistories", []) if isinstance(data.get("directCastHistories"), list) else []):
            if not isinstance(item, dict):
                continue
            entity = item.get("entity")
            history = item.get("history")
            if isinstance(entity, str) and isinstance(history, str) and history != "<none>":
                if entity not in text or history not in text:
                    self.warn(file, f"$.directCastHistories[{i}]", f"entity/history may not appear in text output: {entity!r} : {history!r}")

    def report(self) -> int:
        for issue in self.issues:
            print(f"{issue.severity.upper()}: {issue.file}: {issue.path}: {issue.message}")
        errors = sum(1 for i in self.issues if i.severity == "error")
        warnings = sum(1 for i in self.issues if i.severity == "warning")
        print()
        print("Validation summary:")
        print(f"  errors:   {errors}")
        print(f"  warnings: {warnings}")
        fail = errors > 0 or (self.strict and warnings > 0)
        print(f"  result:   {'FAIL' if fail else 'PASS'}")
        return 1 if fail else 0


def rendered_summary(obj: Any) -> str | None:
    if isinstance(obj, dict) and isinstance(obj.get("rendered"), str):
        return obj["rendered"]
    if isinstance(obj, str):
        return obj
    return None


def find_json_files(paths: list[Path]) -> list[Path]:
    files: list[Path] = []
    for path in paths:
        if path.is_file():
            files.append(path)
        elif path.is_dir():
            files.extend(sorted(path.rglob("*.json")))
        else:
            print(f"warning: path does not exist: {path}", file=sys.stderr)
    seen = set()
    out = []
    for file in files:
        resolved = file.resolve()
        if resolved not in seen:
            seen.add(resolved)
            out.append(file)
    return out


def default_paths() -> list[Path]:
    for p in [Path("json-summaries"), Path("../json-summaries"), Path("json-summeries"), Path("../json-summeries")]:
        if p.exists():
            return [p]
    return [Path("json-summaries")]


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Validate cast-history JSON summaries")
    ap.add_argument("paths", nargs="*", type=Path, help="JSON summary files or directories; default is json-summaries/ if present")
    ap.add_argument("--text-output-dir", type=Path, default=None, help="Optional example-output directory for weak text cross-checks")
    ap.add_argument("--strict", action="store_true", help="Treat warnings as failures")
    ap.add_argument("--no-pretty-check", action="store_true", help="Do not warn if JSON appears to be one line")
    ap.add_argument("--max-issues-per-file", type=int, default=200, help="Limit issue spam per file")
    args = ap.parse_args(argv)

    paths = args.paths or default_paths()
    files = find_json_files(paths)
    if not files:
        print("error: no JSON files found", file=sys.stderr)
        return 1

    validator = Validator(
        strict=args.strict,
        check_pretty=not args.no_pretty_check,
        text_output_dir=args.text_output_dir,
        max_issues_per_file=args.max_issues_per_file,
    )
    for file in files:
        validator.validate_file(file)

    print(f"Validated {len(files)} JSON file(s).")
    return validator.report()


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
