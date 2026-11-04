#!/usr/bin/env python3
"""Aggregate C6g5f4g/h/i/j callback-slot, usage, and allocation-alias facts.

C6g5f4g adds two report-only arrays to schema-v6 paper-facts sidecars:

* ``nestedCallbackSlotRelations``: one row per nested void-pointer callback
  position, with dynamically observed correlations to direct local/parameter
  ``void *`` subjects where an indirect invocation is visible;
* ``voidPointerUsageFingerprints``: one row per eligible direct ``void *``
  local/parameter declaration, including syntactic usage flags, a conservative
  no-fact explanation, and generic-candidate confidence.

C6g5f4h extends every usage fingerprint in schema v7 with a report-only
``observedNoFinalHistory`` object. It classifies only the existing
``observed-no-final-history`` population using retained pointer-use records and
C6g5f4g syntactic flags; it does not create or modify analysis facts.

C6g5f4i adds schema-v8 ``allocationOriginDerivedAliasUses`` rows. They identify
recognized allocator-return origins stored in eligible direct ``void *``
locals/parameters and report uses through may-derived concrete local pointer
aliases. They are reporting-only and do not change the existing origin or
classification solver.

C6g5f4j adds schema-v9 implementation-availability eligibility. Prototype-only
function parameters and their nested callback ``void *`` positions remain in
raw API-shape inventory but are excluded from semantic and relational
denominators.

The analyzer intentionally does not use these facts to construct histories,
solve SCCs, or modify pointer-use membership. This script performs only
stable reporting aggregation. When a C6g5f4e/f classification table is
provided, it enriches rows with the existing semantic categories; it never
invents a category for no-fact declarations or callback slots.
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, MutableMapping, Optional, Sequence, Tuple

PAPER_SCHEMA = "cast-history-tu-paper-facts"
SUPPORTED_PAPER_VERSIONS = {6, 7, 8, 9}
VERSION = "v4-c6g5f4j-void-pointer-relational-facts"

OBSERVED_NO_FINAL_PATTERNS = (
    "active-dereference-or-array-index",
    "external-or-unresolved-escape",
    "callback-mediated-forwarding",
    "known-internal-forwarding",
    "return-or-storage-flow",
    "pointer-arithmetic-without-dereference",
    "pointer-comparison-or-null-test-only",
    "copy-assignment-or-cast-only",
    "observation-only-no-pointer-use",
    "mixed-or-other-retained-evidence",
)

OBSERVED_NO_FINAL_SUMMARY_FIELDS = {
    "active-dereference-or-array-index": "observed_no_final_active_deref_or_array_index",
    "external-or-unresolved-escape": "observed_no_final_external_or_unresolved_escape",
    "callback-mediated-forwarding": "observed_no_final_callback_mediated_forwarding",
    "known-internal-forwarding": "observed_no_final_known_internal_forwarding",
    "return-or-storage-flow": "observed_no_final_return_or_storage_flow",
    "pointer-arithmetic-without-dereference": "observed_no_final_pointer_arithmetic_without_dereference",
    "pointer-comparison-or-null-test-only": "observed_no_final_pointer_comparison_or_null_test_only",
    "copy-assignment-or-cast-only": "observed_no_final_copy_assignment_or_cast_only",
    "observation-only-no-pointer-use": "observed_no_final_observation_only_no_pointer_use",
    "mixed-or-other-retained-evidence": "observed_no_final_mixed_or_other_retained_evidence",
}

SLOT_FIELDS = (
    "project", "paper_facts_files", "slot_id", "owner_declaration_id",
    "owner_declaration_kind", "owner_name", "owner_function_qualified_name",
    "owner_function_signature_key", "owner_parameter_index",
    "relation_variable", "callback_role", "position_kind", "position_path",
    "callback_argument_index", "declared_type", "canonical_type",
    "eligible_for_relational_classification", "owner_definition_available",
    "relational_classification_scope", "eligibility_exclusion_reason",
    "invoked", "has_known_target", "has_unresolved_target", "relation_status",
    "correlated_direct_subject_count", "correlated_direct_subject_ids",
    "correlated_direct_entities", "correlated_direct_declaration_ids",
    "correlated_direct_categories", "correlation_evidence", "call_sites",
)

USAGE_FIELDS = (
    "project", "paper_facts_files", "usage_id", "subject_id", "declaration_id",
    "subject_kind", "entity", "source_file", "source_line", "source_column",
    "function_qualified_name", "function_signature_key", "declared_type",
    "declared_canonical_type", "outer_pointer_kind", "outer_void_pointer",
    "subject_fact_emitted", "has_final_history", "classification_status",
    "semantic_category", "semantic_family", "no_fact_primary_reason",
    "generic_candidate_confidence", "observed_no_final_primary_pattern",
    "observed_no_final_has_retained_pointer_use",
    "observed_no_final_has_retained_observation",
    "observed_no_final_has_deref_or_array_index",
    "observed_no_final_has_pointer_arithmetic",
    "observed_no_final_has_pointer_comparison_or_null_test",
    "observed_no_final_has_known_internal_forwarding",
    "observed_no_final_has_external_or_unresolved_escape",
    "observed_no_final_has_callback_mediated_forwarding",
    "observed_no_final_has_return_or_storage_flow",
    "observed_no_final_has_copy_assignment_or_cast_evidence",
    "observed_no_final_pointer_use_kinds",
    "observed_no_final_semantic_operation_kinds",
    "has_reference", "has_initializer",
    "has_assignment", "has_return", "has_known_internal_call",
    "has_external_or_unknown_call", "has_indirect_callback_call",
    "has_unresolved_indirect_call", "has_stored_into_field_or_global",
    "has_explicit_cast", "has_implicit_void_conversion", "has_dereference",
    "has_pointer_arithmetic", "has_array_index", "has_pointer_comparison",
    "has_null_test", "owner_has_body", "evidence_kinds", "call_sites",
)

SLOT_SUMMARY_FIELDS = (
    "project", "nested_callback_slots", "eligible_nested_callback_slots",
    "excluded_unavailable_api_nested_callback_slots", "invoked_slots",
    "slots_with_correlated_direct_subjects", "invoked_without_direct_subject",
    "declaration_only_slots", "known_target_slots", "unresolved_target_slots",
    "callback_argument_slots", "callback_return_slots",
    "eligible_callback_argument_slots", "eligible_callback_return_slots",
)

USAGE_SUMMARY_FIELDS = (
    "project", "eligible_direct_void_pointer_rows", "classified",
    "observed_no_final_history", "declared_no_observed_facts",
    "final_history_not_emitted",
    "observed_no_final_active_deref_or_array_index",
    "observed_no_final_external_or_unresolved_escape",
    "observed_no_final_callback_mediated_forwarding",
    "observed_no_final_known_internal_forwarding",
    "observed_no_final_return_or_storage_flow",
    "observed_no_final_pointer_arithmetic_without_dereference",
    "observed_no_final_pointer_comparison_or_null_test_only",
    "observed_no_final_copy_assignment_or_cast_only",
    "observed_no_final_observation_only_no_pointer_use",
    "observed_no_final_mixed_or_other_retained_evidence",
    "observed_no_final_pattern_not_available",
    "observed_no_final_patterns_accounted_for", "no_fact_unused",
    "no_fact_declaration_only_no_body", "no_fact_transparent_transport",
    "no_fact_known_internal_forwarding", "no_fact_returned_or_stored",
    "no_fact_callback_escape", "no_fact_external_or_unresolved_escape",
    "no_fact_untracked_or_unsupported", "generic_strong", "generic_conditional",
    "generic_blocked", "generic_insufficient_evidence",
)

OBSERVED_NO_FINAL_ROW_FIELDS = (
    "project", "paper_facts_files", "usage_id", "subject_id", "declaration_id",
    "subject_kind", "entity", "source_file", "source_line", "source_column",
    "function_qualified_name", "function_signature_key", "declared_type",
    "classification_status", "semantic_category", "semantic_family",
    "observed_no_final_primary_pattern",
    "observed_no_final_has_retained_pointer_use",
    "observed_no_final_has_retained_observation",
    "observed_no_final_has_deref_or_array_index",
    "observed_no_final_has_pointer_arithmetic",
    "observed_no_final_has_pointer_comparison_or_null_test",
    "observed_no_final_has_known_internal_forwarding",
    "observed_no_final_has_external_or_unresolved_escape",
    "observed_no_final_has_callback_mediated_forwarding",
    "observed_no_final_has_return_or_storage_flow",
    "observed_no_final_has_copy_assignment_or_cast_evidence",
    "observed_no_final_pointer_use_kinds",
    "observed_no_final_semantic_operation_kinds", "evidence_kinds", "call_sites",
)

OBSERVED_NO_FINAL_PROJECT_FIELDS = (
    "project", "observed_no_final_history",
    "observed_no_final_active_deref_or_array_index",
    "observed_no_final_external_or_unresolved_escape",
    "observed_no_final_callback_mediated_forwarding",
    "observed_no_final_known_internal_forwarding",
    "observed_no_final_return_or_storage_flow",
    "observed_no_final_pointer_arithmetic_without_dereference",
    "observed_no_final_pointer_comparison_or_null_test_only",
    "observed_no_final_copy_assignment_or_cast_only",
    "observed_no_final_observation_only_no_pointer_use",
    "observed_no_final_mixed_or_other_retained_evidence",
    "observed_no_final_pattern_not_available",
    "observed_no_final_patterns_accounted_for",
)

ALLOCATION_ALIAS_FIELDS = (
    "project", "paper_facts_files", "report_id",
    "origin_subject_id", "origin_declaration_id", "origin_entity",
    "origin_source_file", "origin_source_line", "origin_source_column",
    "origin_function_qualified_name", "origin_function_signature_key",
    "origin_declared_type", "origin_declared_canonical_type",
    "origin_semantic_category", "origin_semantic_family",
    "allocator_families", "allocator_functions", "allocation_call_sites",
    "origin_binding_kinds", "alias_declaration_id", "alias_entity", "alias_name",
    "alias_source_file", "alias_source_line", "alias_source_column",
    "alias_function_qualified_name", "alias_function_signature_key",
    "alias_declared_type", "alias_declared_canonical_type", "alias_outer_pointer_kind",
    "derivation_kinds", "derivation_sites", "may_alias",
    "has_reference", "has_assignment", "has_return", "has_call_argument",
    "has_external_or_unknown_call", "has_indirect_call",
    "has_stored_into_field_or_global", "has_explicit_cast",
    "has_implicit_void_conversion", "has_dereference", "has_pointer_arithmetic",
    "has_array_index", "has_pointer_comparison", "has_null_test",
    "has_field_access", "has_deallocation", "evidence_kinds", "use_sites",
)

ALLOCATION_ALIAS_SUMMARY_FIELDS = (
    "project", "allocation_origin_derived_alias_rows",
    "allocation_origin_subjects", "typed_derived_aliases",
    "malloc_family_origins", "calloc_family_origins", "realloc_family_origins",
    "string_duplicate_family_origins", "aliases_with_field_access",
    "aliases_with_dereference", "aliases_with_pointer_arithmetic",
    "aliases_with_array_index", "aliases_with_call_argument",
    "aliases_with_deallocation", "aliases_with_external_or_unknown_call",
    "aliases_with_indirect_call", "aliases_with_return",
    "aliases_stored_into_field_or_global",
)

CATEGORY_FAMILIES = {
    "OpaqueTransportOrViewOnly": "Structured / view-only",
    "MonomorphicActiveSpecialization": "Structured / bounded",
    "ByteOrientedActiveAccess": "Translation-sensitive",
    "GuardedActiveReinterpretation": "Translation-sensitive",
    "UnguardedMultiTargetActiveReinterpretation": "Semantically wild / high-risk",
    "MixedOrProvenanceSensitive": "Semantically wild / high-risk",
    "ExternalUnknown": "External unknown",
}


def bool_text(value: Any) -> str:
    return "yes" if value is True else "no" if value is False else ""


def bool_value(value: Any) -> bool:
    return value is True or (isinstance(value, str) and value.strip().lower() in {"1", "true", "yes"})


def write_csv(path: Path, fields: Sequence[str], rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(fields), extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow({field: bool_text(row.get(field)) if isinstance(row.get(field), bool) else row.get(field, "") for field in fields})


def write_jsonl(path: Path, rows: Iterable[Mapping[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as handle:
        for row in rows:
            handle.write(json.dumps(row, sort_keys=True) + "\n")


def as_obj(value: Any) -> Mapping[str, Any]:
    return value if isinstance(value, Mapping) else {}


def as_list(value: Any) -> List[Any]:
    return value if isinstance(value, list) else []


def strings(value: Any) -> List[str]:
    return sorted({str(item) for item in as_list(value) if str(item)})


def source_parts(value: Any) -> Tuple[str, int, int]:
    source = as_obj(value)
    return str(source.get("file") or ""), int(source.get("line") or 0), int(source.get("column") or 0)


def discover(root: Path) -> List[Path]:
    return sorted(path for path in root.rglob("*.cast-paper-facts.json") if path.is_file() and "partial-paper-facts" not in path.parts)


def project_name(path: Path, root: Path, document: Mapping[str, Any]) -> str:
    project = as_obj(document.get("project"))
    if str(project.get("id") or ""):
        return str(project["id"])
    declared_root = str(project.get("root") or "")
    if declared_root:
        return Path(declared_root).name or declared_root
    try:
        relative = path.relative_to(root)
        return relative.parts[0] if len(relative.parts) > 1 else "default"
    except ValueError:
        return "default"


def load_classifications(path: Optional[Path], errors: List[str]) -> Dict[Tuple[str, str], Dict[str, str]]:
    if path is None:
        return {}
    if not path.exists():
        errors.append(f"classification table does not exist: {path}")
        return {}
    result: Dict[Tuple[str, str], Dict[str, str]] = {}
    with path.open(encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            project = str(row.get("project") or "")
            subject_id = str(row.get("subject_id") or "")
            if not project or not subject_id:
                continue
            value = {
                "category": str(row.get("semantic_category") or ""),
                "family": str(row.get("semantic_family") or ""),
                "status": str(row.get("classification_status") or ""),
            }
            previous = result.get((project, subject_id))
            if previous and previous != value:
                errors.append(f"conflicting classification rows for {(project, subject_id)}")
            result[(project, subject_id)] = value
    return result


def merge_bool(target: MutableMapping[str, Any], name: str, source: Mapping[str, Any]) -> None:
    target[name] = bool_value(target.get(name)) or bool_value(source.get(name))


def union_values(target: MutableMapping[str, Any], name: str, values: Any) -> None:
    target[name] = sorted({*strings(target.get(name)), *strings(values)})


def merge_slot(existing: MutableMapping[str, Any], incoming: Mapping[str, Any], errors: List[str], context: str) -> None:
    immutable = (
        "ownerDeclarationId", "ownerDeclarationKind", "ownerName", "relationVariable",
        "callbackRole", "positionKind", "positionPath", "callbackArgumentIndex",
        "declaredType", "canonicalType", "eligibleForRelationalClassification",
        "ownerDefinitionAvailable", "relationalClassificationScope",
        "eligibilityExclusionReason",
    )
    for field in immutable:
        a, b = existing.get(field), incoming.get(field)
        if a not in (None, "") and b not in (None, "") and a != b:
            errors.append(f"{context}: conflicting callback slot {field}: {a!r} != {b!r}")
    prior_subjects = {str(as_obj(item).get("subjectId") or ""): as_obj(item) for item in as_list(existing.get("correlatedDirectSubjects"))}
    for item in as_list(incoming.get("correlatedDirectSubjects")):
        row = as_obj(item)
        sid = str(row.get("subjectId") or "")
        if sid:
            prior_subjects[sid] = row
    existing["correlatedDirectSubjects"] = [prior_subjects[key] for key in sorted(prior_subjects)]
    union_values(existing, "correlationEvidence", incoming.get("correlationEvidence"))
    union_values(existing, "callSites", incoming.get("callSites"))
    a, b = as_obj(existing.get("invocation")), as_obj(incoming.get("invocation"))
    existing["invocation"] = {
        "invoked": bool_value(a.get("invoked")) or bool_value(b.get("invoked")),
        "hasKnownTarget": bool_value(a.get("hasKnownTarget")) or bool_value(b.get("hasKnownTarget")),
        "hasUnresolvedTarget": bool_value(a.get("hasUnresolvedTarget")) or bool_value(b.get("hasUnresolvedTarget")),
    }
    existing.setdefault("_sourceFiles", [])
    existing["_sourceFiles"] = sorted({*strings(existing["_sourceFiles"]), *strings(incoming.get("_sourceFiles"))})


def merge_usage(existing: MutableMapping[str, Any], incoming: Mapping[str, Any], errors: List[str], context: str) -> None:
    for field in ("subjectId", "declarationId", "classificationStatus", "noFactPrimaryReason", "genericCandidateConfidence"):
        a, b = existing.get(field), incoming.get(field)
        if a not in (None, "") and b not in (None, "") and a != b:
            errors.append(f"{context}: conflicting usage fingerprint {field}: {a!r} != {b!r}")
    existing["usageFlags"] = {
        key: bool_value(as_obj(existing.get("usageFlags")).get(key)) or bool_value(as_obj(incoming.get("usageFlags")).get(key))
        for key in set(as_obj(existing.get("usageFlags"))) | set(as_obj(incoming.get("usageFlags")))
    }
    existing_no_final = dict(as_obj(existing.get("observedNoFinalHistory")))
    incoming_no_final = as_obj(incoming.get("observedNoFinalHistory"))
    if incoming_no_final:
        for field in ("primaryPattern",):
            a, b = existing_no_final.get(field), incoming_no_final.get(field)
            if a not in (None, "", "not-applicable") and b not in (None, "", "not-applicable") and a != b:
                errors.append(f"{context}: conflicting observed-no-final-history {field}: {a!r} != {b!r}")
            if not a or a == "not-applicable":
                existing_no_final[field] = b
        for field in (
            "hasRetainedPointerUse", "hasRetainedObservation",
            "hasDerefOrArrayIndex", "hasPointerArithmetic",
            "hasPointerComparisonOrNullTest", "hasKnownInternalForwarding",
            "hasExternalOrUnresolvedEscape", "hasCallbackMediatedForwarding",
            "hasReturnOrStorageFlow", "hasCopyAssignmentOrCastEvidence",
        ):
            existing_no_final[field] = bool_value(existing_no_final.get(field)) or bool_value(incoming_no_final.get(field))
        for field in ("pointerUseKinds", "semanticOperationKinds"):
            existing_no_final[field] = sorted({*strings(existing_no_final.get(field)), *strings(incoming_no_final.get(field))})
        existing["observedNoFinalHistory"] = existing_no_final
    union_values(existing, "evidenceKinds", incoming.get("evidenceKinds"))
    union_values(existing, "callSites", incoming.get("callSites"))
    existing["_sourceFiles"] = sorted({*strings(existing.get("_sourceFiles")), *strings(incoming.get("_sourceFiles"))})


def merge_allocation_alias(existing: MutableMapping[str, Any], incoming: Mapping[str, Any], errors: List[str], context: str) -> None:
    immutable = (
        "originSubjectId", "originDeclarationId", "originEntity",
        "aliasDeclarationId", "aliasEntity", "aliasName", "mayAlias",
    )
    for field in immutable:
        a, b = existing.get(field), incoming.get(field)
        if a not in (None, "") and b not in (None, "") and a != b:
            errors.append(f"{context}: conflicting allocation alias {field}: {a!r} != {b!r}")
    for field in (
        "allocatorFamilies", "allocatorFunctions", "allocationCallSites",
        "originBindingKinds", "derivationKinds", "derivationSites",
        "evidenceKinds", "useSites",
    ):
        union_values(existing, field, incoming.get(field))
    existing["usageFlags"] = {
        key: bool_value(as_obj(existing.get("usageFlags")).get(key)) or
             bool_value(as_obj(incoming.get("usageFlags")).get(key))
        for key in set(as_obj(existing.get("usageFlags"))) |
                   set(as_obj(incoming.get("usageFlags")))
    }
    existing["_sourceFiles"] = sorted({
        *strings(existing.get("_sourceFiles")), *strings(incoming.get("_sourceFiles"))
    })


def flatten_slot(project: str, slot: Mapping[str, Any], classifications: Mapping[Tuple[str, str], Mapping[str, str]]) -> Dict[str, Any]:
    owner_function = as_obj(slot.get("ownerFunction"))
    invocation = as_obj(slot.get("invocation"))
    correlated = [as_obj(item) for item in as_list(slot.get("correlatedDirectSubjects"))]
    ids = sorted({str(item.get("subjectId") or "") for item in correlated if str(item.get("subjectId") or "")})
    entities = sorted({str(item.get("entity") or "") for item in correlated if str(item.get("entity") or "")})
    declaration_ids = sorted({str(item.get("declarationId") or "") for item in correlated if str(item.get("declarationId") or "")})
    categories = []
    for subject_id in ids:
        value = classifications.get((project, subject_id), {})
        category = str(value.get("category") or "")
        categories.append(f"{subject_id}={category or '<unclassified>'}")
    # Pre-v9 sidecars did not carry an explicit eligibility bit; preserve
    # their historical semantics when aggregating legacy data.
    eligible = (bool_value(slot.get("eligibleForRelationalClassification"))
                if "eligibleForRelationalClassification" in slot else True)
    invoked = bool_value(invocation.get("invoked"))
    if not eligible:
        status = "out-of-scope-unavailable-api-slot"
    elif ids:
        status = "correlated-direct-subjects"
    elif invoked:
        status = "invoked-without-direct-subject"
    else:
        status = "declaration-only-slot"
    return {
        "project": project,
        "paper_facts_files": ";".join(strings(slot.get("_sourceFiles"))),
        "slot_id": slot.get("slotId", ""),
        "owner_declaration_id": slot.get("ownerDeclarationId", ""),
        "owner_declaration_kind": slot.get("ownerDeclarationKind", ""),
        "owner_name": slot.get("ownerName", ""),
        "owner_function_qualified_name": owner_function.get("qualifiedName", ""),
        "owner_function_signature_key": owner_function.get("signatureKey", ""),
        "owner_parameter_index": slot.get("ownerParameterIndex", ""),
        "relation_variable": slot.get("relationVariable", ""),
        "callback_role": slot.get("callbackRole", ""),
        "position_kind": slot.get("positionKind", ""),
        "position_path": slot.get("positionPath", ""),
        "callback_argument_index": slot.get("callbackArgumentIndex", ""),
        "declared_type": slot.get("declaredType", ""),
        "canonical_type": slot.get("canonicalType", ""),
        "eligible_for_relational_classification": eligible,
        "owner_definition_available": bool_value(slot.get("ownerDefinitionAvailable")),
        "relational_classification_scope":
            slot.get("relationalClassificationScope",
                     "legacy-eligibility-not-recorded"),
        "eligibility_exclusion_reason":
            slot.get("eligibilityExclusionReason", "not-applicable"),
        "invoked": invoked,
        "has_known_target": bool_value(invocation.get("hasKnownTarget")),
        "has_unresolved_target": bool_value(invocation.get("hasUnresolvedTarget")),
        "relation_status": status,
        "correlated_direct_subject_count": len(ids),
        "correlated_direct_subject_ids": ";".join(ids),
        "correlated_direct_entities": ";".join(entities),
        "correlated_direct_declaration_ids": ";".join(declaration_ids),
        "correlated_direct_categories": ";".join(categories),
        "correlation_evidence": ";".join(strings(slot.get("correlationEvidence"))),
        "call_sites": ";".join(strings(slot.get("callSites"))),
    }


def flatten_usage(project: str, usage: Mapping[str, Any], classifications: Mapping[Tuple[str, str], Mapping[str, str]]) -> Dict[str, Any]:
    source_file, source_line, source_column = source_parts(usage.get("location"))
    function = as_obj(usage.get("function"))
    declared = as_obj(usage.get("declaredType"))
    flags = as_obj(usage.get("usageFlags"))
    no_final = as_obj(usage.get("observedNoFinalHistory"))
    status = str(usage.get("classificationStatus") or "")
    no_final_pattern = str(no_final.get("primaryPattern") or (
        "not-applicable" if status != "observed-no-final-history" else "not-available"
    ))
    classified = classifications.get((project, str(usage.get("subjectId") or "")), {})
    category = str(classified.get("category") or "")
    family = str(classified.get("family") or CATEGORY_FAMILIES.get(category, ""))
    return {
        "project": project,
        "paper_facts_files": ";".join(strings(usage.get("_sourceFiles"))),
        "usage_id": usage.get("usageId", ""),
        "subject_id": usage.get("subjectId", ""),
        "declaration_id": usage.get("declarationId", ""),
        "subject_kind": usage.get("subjectKind", ""),
        "entity": usage.get("entity", ""),
        "source_file": source_file,
        "source_line": source_line,
        "source_column": source_column,
        "function_qualified_name": function.get("qualifiedName", ""),
        "function_signature_key": function.get("signatureKey", ""),
        "declared_type": declared.get("text", ""),
        "declared_canonical_type": declared.get("canonicalText", ""),
        "outer_pointer_kind": declared.get("outerPointerKind", ""),
        "outer_void_pointer": bool_value(declared.get("outerVoidPointer")),
        "subject_fact_emitted": bool_value(usage.get("subjectFactEmitted")),
        "has_final_history": bool_value(usage.get("hasFinalHistory")),
        "classification_status": usage.get("classificationStatus", ""),
        "semantic_category": category,
        "semantic_family": family,
        "no_fact_primary_reason": usage.get("noFactPrimaryReason", ""),
        "generic_candidate_confidence": usage.get("genericCandidateConfidence", ""),
        "observed_no_final_primary_pattern": no_final_pattern,
        "observed_no_final_has_retained_pointer_use": bool_value(no_final.get("hasRetainedPointerUse")),
        "observed_no_final_has_retained_observation": bool_value(no_final.get("hasRetainedObservation")),
        "observed_no_final_has_deref_or_array_index": bool_value(no_final.get("hasDerefOrArrayIndex")),
        "observed_no_final_has_pointer_arithmetic": bool_value(no_final.get("hasPointerArithmetic")),
        "observed_no_final_has_pointer_comparison_or_null_test": bool_value(no_final.get("hasPointerComparisonOrNullTest")),
        "observed_no_final_has_known_internal_forwarding": bool_value(no_final.get("hasKnownInternalForwarding")),
        "observed_no_final_has_external_or_unresolved_escape": bool_value(no_final.get("hasExternalOrUnresolvedEscape")),
        "observed_no_final_has_callback_mediated_forwarding": bool_value(no_final.get("hasCallbackMediatedForwarding")),
        "observed_no_final_has_return_or_storage_flow": bool_value(no_final.get("hasReturnOrStorageFlow")),
        "observed_no_final_has_copy_assignment_or_cast_evidence": bool_value(no_final.get("hasCopyAssignmentOrCastEvidence")),
        "observed_no_final_pointer_use_kinds": ";".join(strings(no_final.get("pointerUseKinds"))),
        "observed_no_final_semantic_operation_kinds": ";".join(strings(no_final.get("semanticOperationKinds"))),
        "has_reference": bool_value(flags.get("hasReference")),
        "has_initializer": bool_value(flags.get("hasInitializer")),
        "has_assignment": bool_value(flags.get("hasAssignment")),
        "has_return": bool_value(flags.get("hasReturn")),
        "has_known_internal_call": bool_value(flags.get("hasKnownInternalCall")),
        "has_external_or_unknown_call": bool_value(flags.get("hasExternalOrUnknownCall")),
        "has_indirect_callback_call": bool_value(flags.get("hasIndirectCallbackCall")),
        "has_unresolved_indirect_call": bool_value(flags.get("hasUnresolvedIndirectCall")),
        "has_stored_into_field_or_global": bool_value(flags.get("hasStoredIntoFieldOrGlobal")),
        "has_explicit_cast": bool_value(flags.get("hasExplicitCast")),
        "has_implicit_void_conversion": bool_value(flags.get("hasImplicitVoidConversion")),
        "has_dereference": bool_value(flags.get("hasDereference")),
        "has_pointer_arithmetic": bool_value(flags.get("hasPointerArithmetic")),
        "has_array_index": bool_value(flags.get("hasArrayIndex")),
        "has_pointer_comparison": bool_value(flags.get("hasPointerComparison")),
        "has_null_test": bool_value(flags.get("hasNullTest")),
        "owner_has_body": bool_value(flags.get("ownerHasBody")),
        "evidence_kinds": ";".join(strings(usage.get("evidenceKinds"))),
        "call_sites": ";".join(strings(usage.get("callSites"))),
    }


def slot_summary(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    grouped: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    for row in rows:
        grouped[str(row.get("project") or "")].append(row)
    result: List[Dict[str, Any]] = []
    for project in sorted(grouped):
        values = grouped[project]
        eligible = [
            row for row in values
            if bool_value(row.get("eligible_for_relational_classification"))
        ]
        result.append({
            "project": project,
            "nested_callback_slots": len(values),
            "eligible_nested_callback_slots": len(eligible),
            "excluded_unavailable_api_nested_callback_slots": sum(
                str(row.get("eligibility_exclusion_reason") or "") ==
                "unavailable-owner-function-body"
                for row in values),
            # Invocation/correlation coverage is intentionally computed only
            # over slots whose owner implementation is available.
            "invoked_slots": sum(bool_value(r.get("invoked")) for r in eligible),
            "slots_with_correlated_direct_subjects": sum(
                int(r.get("correlated_direct_subject_count") or 0) > 0
                for r in eligible),
            "invoked_without_direct_subject": sum(
                r.get("relation_status") == "invoked-without-direct-subject"
                for r in eligible),
            "declaration_only_slots": sum(
                r.get("relation_status") == "declaration-only-slot"
                for r in eligible),
            "known_target_slots": sum(
                bool_value(r.get("has_known_target")) for r in eligible),
            "unresolved_target_slots": sum(
                bool_value(r.get("has_unresolved_target")) for r in eligible),
            # These first two counts retain raw API shape.  The following two
            # are the relational-analysis denominator.
            "callback_argument_slots": sum(
                str(r.get("position_kind") or "") == "function-parameter"
                for r in values),
            "callback_return_slots": sum(
                str(r.get("position_kind") or "") == "function-return"
                for r in values),
            "eligible_callback_argument_slots": sum(
                str(r.get("position_kind") or "") == "function-parameter"
                for r in eligible),
            "eligible_callback_return_slots": sum(
                str(r.get("position_kind") or "") == "function-return"
                for r in eligible),
        })
    return result


def flatten_allocation_alias(project: str, report: Mapping[str, Any], classifications: Mapping[Tuple[str, str], Mapping[str, str]]) -> Dict[str, Any]:
    origin_file, origin_line, origin_column = source_parts(report.get("originLocation"))
    alias_file, alias_line, alias_column = source_parts(report.get("aliasLocation"))
    origin_function = as_obj(report.get("originFunction"))
    alias_function = as_obj(report.get("aliasFunction"))
    origin_type = as_obj(report.get("originDeclaredType"))
    alias_type = as_obj(report.get("aliasDeclaredType"))
    flags = as_obj(report.get("usageFlags"))
    origin_subject_id = str(report.get("originSubjectId") or "")
    classified = classifications.get((project, origin_subject_id), {})
    category = str(classified.get("category") or "")
    family = str(classified.get("family") or CATEGORY_FAMILIES.get(category, ""))
    return {
        "project": project,
        "paper_facts_files": ";".join(strings(report.get("_sourceFiles"))),
        "report_id": report.get("reportId", ""),
        "origin_subject_id": origin_subject_id,
        "origin_declaration_id": report.get("originDeclarationId", ""),
        "origin_entity": report.get("originEntity", ""),
        "origin_source_file": origin_file,
        "origin_source_line": origin_line,
        "origin_source_column": origin_column,
        "origin_function_qualified_name": origin_function.get("qualifiedName", ""),
        "origin_function_signature_key": origin_function.get("signatureKey", ""),
        "origin_declared_type": origin_type.get("text", ""),
        "origin_declared_canonical_type": origin_type.get("canonicalText", ""),
        "origin_semantic_category": category,
        "origin_semantic_family": family,
        "allocator_families": ";".join(strings(report.get("allocatorFamilies"))),
        "allocator_functions": ";".join(strings(report.get("allocatorFunctions"))),
        "allocation_call_sites": ";".join(strings(report.get("allocationCallSites"))),
        "origin_binding_kinds": ";".join(strings(report.get("originBindingKinds"))),
        "alias_declaration_id": report.get("aliasDeclarationId", ""),
        "alias_entity": report.get("aliasEntity", ""),
        "alias_name": report.get("aliasName", ""),
        "alias_source_file": alias_file,
        "alias_source_line": alias_line,
        "alias_source_column": alias_column,
        "alias_function_qualified_name": alias_function.get("qualifiedName", ""),
        "alias_function_signature_key": alias_function.get("signatureKey", ""),
        "alias_declared_type": alias_type.get("text", ""),
        "alias_declared_canonical_type": alias_type.get("canonicalText", ""),
        "alias_outer_pointer_kind": alias_type.get("outerPointerKind", ""),
        "derivation_kinds": ";".join(strings(report.get("derivationKinds"))),
        "derivation_sites": ";".join(strings(report.get("derivationSites"))),
        "may_alias": bool_value(report.get("mayAlias")),
        "has_reference": bool_value(flags.get("hasReference")),
        "has_assignment": bool_value(flags.get("hasAssignment")),
        "has_return": bool_value(flags.get("hasReturn")),
        "has_call_argument": bool_value(flags.get("hasCallArgument")),
        "has_external_or_unknown_call": bool_value(flags.get("hasExternalOrUnknownCall")),
        "has_indirect_call": bool_value(flags.get("hasIndirectCall")),
        "has_stored_into_field_or_global": bool_value(flags.get("hasStoredIntoFieldOrGlobal")),
        "has_explicit_cast": bool_value(flags.get("hasExplicitCast")),
        "has_implicit_void_conversion": bool_value(flags.get("hasImplicitVoidConversion")),
        "has_dereference": bool_value(flags.get("hasDereference")),
        "has_pointer_arithmetic": bool_value(flags.get("hasPointerArithmetic")),
        "has_array_index": bool_value(flags.get("hasArrayIndex")),
        "has_pointer_comparison": bool_value(flags.get("hasPointerComparison")),
        "has_null_test": bool_value(flags.get("hasNullTest")),
        "has_field_access": bool_value(flags.get("hasFieldAccess")),
        "has_deallocation": bool_value(flags.get("hasDeallocation")),
        "evidence_kinds": ";".join(strings(report.get("evidenceKinds"))),
        "use_sites": ";".join(strings(report.get("useSites"))),
    }


def allocation_alias_summary(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    grouped: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    for row in rows:
        grouped[str(row.get("project") or "")].append(row)
    result: List[Dict[str, Any]] = []
    for project in sorted(grouped):
        values = grouped[project]
        origin_families: Dict[str, set[str]] = defaultdict(set)
        for row in values:
            origin = str(row.get("origin_subject_id") or "")
            for family in str(row.get("allocator_families") or "").split(";"):
                if family:
                    origin_families[family].add(origin)
        row: Dict[str, Any] = {
            "project": project,
            "allocation_origin_derived_alias_rows": len(values),
            "allocation_origin_subjects": len({str(r.get("origin_subject_id") or "") for r in values}),
            "typed_derived_aliases": len({(str(r.get("origin_subject_id") or ""), str(r.get("alias_declaration_id") or "")) for r in values}),
            "malloc_family_origins": len(origin_families["malloc-family"]),
            "calloc_family_origins": len(origin_families["calloc-family"]),
            "realloc_family_origins": len(origin_families["realloc-family"]),
            "string_duplicate_family_origins": len(origin_families["string-duplicate-family"]),
            "aliases_with_field_access": sum(bool_value(r.get("has_field_access")) for r in values),
            "aliases_with_dereference": sum(bool_value(r.get("has_dereference")) for r in values),
            "aliases_with_pointer_arithmetic": sum(bool_value(r.get("has_pointer_arithmetic")) for r in values),
            "aliases_with_array_index": sum(bool_value(r.get("has_array_index")) for r in values),
            "aliases_with_call_argument": sum(bool_value(r.get("has_call_argument")) for r in values),
            "aliases_with_deallocation": sum(bool_value(r.get("has_deallocation")) for r in values),
            "aliases_with_external_or_unknown_call": sum(bool_value(r.get("has_external_or_unknown_call")) for r in values),
            "aliases_with_indirect_call": sum(bool_value(r.get("has_indirect_call")) for r in values),
            "aliases_with_return": sum(bool_value(r.get("has_return")) for r in values),
            "aliases_stored_into_field_or_global": sum(bool_value(r.get("has_stored_into_field_or_global")) for r in values),
        }
        result.append(row)
    return result


def observed_no_final_summary(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    grouped: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    for row in rows:
        if str(row.get("classification_status") or "") == "observed-no-final-history":
            grouped[str(row.get("project") or "")].append(row)
    result: List[Dict[str, Any]] = []
    for project in sorted(grouped):
        values = grouped[project]
        patterns = Counter(str(r.get("observed_no_final_primary_pattern") or "not-available") for r in values)
        row: Dict[str, Any] = {
            "project": project,
            "observed_no_final_history": len(values),
        }
        for pattern, field in OBSERVED_NO_FINAL_SUMMARY_FIELDS.items():
            row[field] = patterns[pattern]
        row["observed_no_final_pattern_not_available"] = patterns["not-available"]
        row["observed_no_final_patterns_accounted_for"] = sum(patterns.values())
        result.append(row)
    return result


def usage_summary(rows: Sequence[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    grouped: Dict[str, List[Mapping[str, Any]]] = defaultdict(list)
    for row in rows:
        grouped[str(row.get("project") or "")].append(row)
    result: List[Dict[str, Any]] = []
    for project in sorted(grouped):
        values = grouped[project]
        status = Counter(str(r.get("classification_status") or "") for r in values)
        reason = Counter(str(r.get("no_fact_primary_reason") or "") for r in values)
        confidence = Counter(str(r.get("generic_candidate_confidence") or "") for r in values)
        patterns = Counter(
            str(r.get("observed_no_final_primary_pattern") or "not-available")
            for r in values
            if str(r.get("classification_status") or "") == "observed-no-final-history"
        )
        row: Dict[str, Any] = {
            "project": project,
            "eligible_direct_void_pointer_rows": len(values),
            "classified": status["classified"],
            "observed_no_final_history": status["observed-no-final-history"],
            "declared_no_observed_facts": status["declared-no-observed-facts"],
            "final_history_not_emitted": status["final-history-not-emitted"],
            "no_fact_unused": reason["unused"],
            "no_fact_declaration_only_no_body": reason["declaration-only-no-body"],
            "no_fact_transparent_transport": reason["transparent-local-transport-only"],
            "no_fact_known_internal_forwarding": reason["known-internal-forwarding-only"],
            "no_fact_returned_or_stored": reason["returned-or-stored-without-local-interpretation"],
            "no_fact_callback_escape": reason["callback-or-function-pointer-escape"],
            "no_fact_external_or_unresolved_escape": reason["external-or-unresolved-call-escape"],
            "no_fact_untracked_or_unsupported": reason["untracked-or-unsupported-use-pattern"],
            "generic_strong": confidence["strong"],
            "generic_conditional": confidence["conditional"],
            "generic_blocked": confidence["blocked"],
            "generic_insufficient_evidence": confidence["insufficient-evidence"],
        }
        for pattern, field in OBSERVED_NO_FINAL_SUMMARY_FIELDS.items():
            row[field] = patterns[pattern]
        row["observed_no_final_pattern_not_available"] = patterns["not-available"]
        row["observed_no_final_patterns_accounted_for"] = sum(patterns.values())
        result.append(row)
    return result


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Aggregate C6g5f4g/h/i callback-slot, usage, and allocation-derived-alias facts.")
    parser.add_argument("--root", required=True, type=Path, help="Root containing schema-v6/v7/v8/v9 *.cast-paper-facts.json sidecars")
    parser.add_argument("--out-dir", required=True, type=Path, help="Output directory")
    parser.add_argument("--void-pointer-classification-rows", type=Path, help="Optional C6g5f4e classification CSV for semantic-category enrichment")
    parser.add_argument("--strict", action="store_true", help="Fail on malformed documents or duplicate conflicts")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    out_dir = args.out_dir.resolve()
    paths = discover(root)
    if not paths:
        print(f"error: no *.cast-paper-facts.json sidecars under {root}", file=sys.stderr)
        return 2
    out_dir.mkdir(parents=True, exist_ok=True)
    errors: List[str] = []
    classifications = load_classifications(args.void_pointer_classification_rows.resolve() if args.void_pointer_classification_rows else None, errors)
    slots: Dict[Tuple[str, str], Dict[str, Any]] = {}
    usages: Dict[Tuple[str, str], Dict[str, Any]] = {}
    allocation_aliases: Dict[Tuple[str, str], Dict[str, Any]] = {}
    accepted: List[str] = []

    for path in paths:
        try:
            document = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{path}: cannot read JSON: {exc}")
            continue
        if not isinstance(document, Mapping) or document.get("schemaName") != PAPER_SCHEMA:
            errors.append(f"{path}: not a {PAPER_SCHEMA} document")
            continue
        version = document.get("schemaVersion")
        if version not in SUPPORTED_PAPER_VERSIONS:
            errors.append(f"{path}: requires schemaVersion 6, 7, 8, or 9, found {version!r}")
            continue
        project = project_name(path, root, document)
        accepted.append(path.as_posix())
        for value in as_list(document.get("nestedCallbackSlotRelations")):
            slot = dict(as_obj(value))
            slot_id = str(slot.get("slotId") or "")
            if not slot_id:
                errors.append(f"{path}: nested callback slot lacks slotId")
                continue
            slot["_sourceFiles"] = [path.as_posix()]
            key = (project, slot_id)
            if key not in slots:
                slots[key] = slot
            else:
                merge_slot(slots[key], slot, errors, f"{path}: {key}")
        for value in as_list(document.get("voidPointerUsageFingerprints")):
            usage = dict(as_obj(value))
            usage_id = str(usage.get("usageId") or "")
            if not usage_id:
                errors.append(f"{path}: void-pointer usage fingerprint lacks usageId")
                continue
            usage["_sourceFiles"] = [path.as_posix()]
            key = (project, usage_id)
            if key not in usages:
                usages[key] = usage
            else:
                merge_usage(usages[key], usage, errors, f"{path}: {key}")
        for value in as_list(document.get("allocationOriginDerivedAliasUses")):
            report = dict(as_obj(value))
            report_id = str(report.get("reportId") or "")
            if not report_id:
                errors.append(f"{path}: allocation-origin alias report lacks reportId")
                continue
            report["_sourceFiles"] = [path.as_posix()]
            key = (project, report_id)
            if key not in allocation_aliases:
                allocation_aliases[key] = report
            else:
                merge_allocation_alias(allocation_aliases[key], report, errors, f"{path}: {key}")

    slot_rows = [flatten_slot(project, slot, classifications) for (project, _), slot in sorted(slots.items())]
    usage_rows = [flatten_usage(project, usage, classifications) for (project, _), usage in sorted(usages.items())]
    allocation_alias_rows = [
        flatten_allocation_alias(project, report, classifications)
        for (project, _), report in sorted(allocation_aliases.items())
    ]
    observed_no_final_rows = [
        row for row in usage_rows
        if str(row.get("classification_status") or "") == "observed-no-final-history"
    ]
    slot_rows_by_project = slot_summary(slot_rows)
    usage_rows_by_project = usage_summary(usage_rows)
    observed_no_final_rows_by_project = observed_no_final_summary(usage_rows)
    allocation_alias_rows_by_project = allocation_alias_summary(allocation_alias_rows)
    if slot_rows_by_project:
        total = {field: sum(int(row.get(field) or 0) for row in slot_rows_by_project) for field in SLOT_SUMMARY_FIELDS if field != "project"}
        total["project"] = "Total"
        slot_rows_by_project.append(total)
    if usage_rows_by_project:
        total = {field: sum(int(row.get(field) or 0) for row in usage_rows_by_project) for field in USAGE_SUMMARY_FIELDS if field != "project"}
        total["project"] = "Total"
        usage_rows_by_project.append(total)
    if observed_no_final_rows_by_project:
        total = {field: sum(int(row.get(field) or 0) for row in observed_no_final_rows_by_project) for field in OBSERVED_NO_FINAL_PROJECT_FIELDS if field != "project"}
        total["project"] = "Total"
        observed_no_final_rows_by_project.append(total)
    if allocation_alias_rows_by_project:
        total = {field: sum(int(row.get(field) or 0) for row in allocation_alias_rows_by_project) for field in ALLOCATION_ALIAS_SUMMARY_FIELDS if field != "project"}
        total["project"] = "Total"
        allocation_alias_rows_by_project.append(total)

    write_csv(out_dir / "nested_callback_slot_relational_facts.csv", SLOT_FIELDS, slot_rows)
    write_jsonl(out_dir / "nested_callback_slot_relational_facts.jsonl", slot_rows)
    write_csv(out_dir / "nested_callback_slot_relational_facts_by_project.csv", SLOT_SUMMARY_FIELDS, slot_rows_by_project)
    write_csv(out_dir / "void_pointer_usage_fingerprint_rows.csv", USAGE_FIELDS, usage_rows)
    write_jsonl(out_dir / "void_pointer_usage_fingerprint_rows.jsonl", usage_rows)
    write_csv(out_dir / "void_pointer_usage_fingerprint_by_project.csv", USAGE_SUMMARY_FIELDS, usage_rows_by_project)
    write_csv(out_dir / "observed_no_final_history_pattern_rows.csv", OBSERVED_NO_FINAL_ROW_FIELDS, observed_no_final_rows)
    write_jsonl(out_dir / "observed_no_final_history_pattern_rows.jsonl", observed_no_final_rows)
    write_csv(out_dir / "observed_no_final_history_patterns_by_project.csv", OBSERVED_NO_FINAL_PROJECT_FIELDS, observed_no_final_rows_by_project)
    write_csv(out_dir / "allocation_origin_derived_alias_use_rows.csv", ALLOCATION_ALIAS_FIELDS, allocation_alias_rows)
    write_jsonl(out_dir / "allocation_origin_derived_alias_use_rows.jsonl", allocation_alias_rows)
    write_csv(out_dir / "allocation_origin_derived_alias_use_by_project.csv", ALLOCATION_ALIAS_SUMMARY_FIELDS, allocation_alias_rows_by_project)
    manifest = {
        "schemaName": "cast-history-void-pointer-relational-facts-manifest",
        "schemaVersion": 3,
        "toolVersion": VERSION,
        "createdAt": datetime.now(timezone.utc).isoformat(),
        "root": root.as_posix(),
        "inputFiles": accepted,
        "nestedCallbackSlotRelations": len(slot_rows),
        "voidPointerUsageFingerprints": len(usage_rows),
        "observedNoFinalHistoryRows": len(observed_no_final_rows),
        "allocationOriginDerivedAliasUses": len(allocation_alias_rows),
        "classificationEnrichment": bool(classifications),
        "errors": errors,
    }
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for error in errors:
        print(f"warning: {error}", file=sys.stderr)
    if args.strict and errors:
        return 1
    print(f"aggregated {len(accepted)} schema-v6/v7/v8/v9 sidecars into {len(slot_rows)} callback-slot rows, {len(usage_rows)} usage fingerprints, {len(observed_no_final_rows)} observed-no-final-history rows, and {len(allocation_alias_rows)} allocation-derived-alias rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
