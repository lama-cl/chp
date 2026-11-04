#!/usr/bin/env python3
"""Validate C6g5f4 compact `*.cast-paper-facts.json` sidecars.

This dependency-free validator checks the fields required for paper aggregation,
subject-ID uniqueness within a TU, callback subject references, the fixed
primary-category vocabulary, AST-derived outer pointer kind, and C6g5f4
callback-mediation state.
It intentionally does not inspect the verbose report or compact C2 link-summary
artifacts.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Iterable, Mapping, Optional, Sequence

SCHEMA_NAME = "cast-history-tu-paper-facts"
CURRENT_SCHEMA_VERSION = 9
SUPPORTED_SCHEMA_VERSIONS = {1, 2, 3, 4, 5, 6, 7, 8, 9}
OUTER_POINTER_KINDS = {
    "void-data-pointer", "nonvoid-data-pointer", "function-pointer",
    "not-pointer", "unknown",
}
CALLBACK_MEDIATION_STATES = {
    "None", "FormalCallbackParameter", "UnresolvedCallback", "FieldCallback",
    "ResolvedCallback", "ResolvedFieldCallback", "GuardedResolvedFieldCallback",
    "Unknown",
}
CATEGORIES = {
    "OpaqueTransportOrViewOnly",
    "MonomorphicActiveSpecialization",
    "ByteOrientedActiveAccess",
    "GuardedActiveReinterpretation",
    "UnguardedMultiTargetActiveReinterpretation",
    "MixedOrProvenanceSensitive",
    "ExternalUnknown",
}

OBSERVED_NO_FINAL_HISTORY_PATTERNS = {
    "not-applicable",
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
}

INVENTORY_STATUSES = {
    "classified",
    "observed-no-final-history",
    "final-history-not-emitted",
    "declared-no-observed-facts",
    "out-of-scope-unavailable-api-parameter",
}

SEMANTIC_CLASSIFICATION_SCOPES = {
    "implementation-body",
    "external-or-unavailable-api",
    "unknown",
}

RELATIONAL_CLASSIFICATION_SCOPES = {
    "implementation-body",
    "field-or-aggregate-interface",
    "global-callback-carrier",
    "external-or-unavailable-api",
    "unknown",
}

ELIGIBILITY_EXCLUSION_REASONS = {
    "not-applicable",
    "unavailable-owner-function-body",
    "not-local-or-parameter",
}


def discover(paths: Iterable[Path], root: Optional[Path]) -> list[Path]:
    found: list[Path] = []
    if root:
        found.extend(root.rglob("*.cast-paper-facts.json"))
    for path in paths:
        if path.is_dir():
            found.extend(path.rglob("*.cast-paper-facts.json"))
        elif path.name.endswith(".cast-paper-facts.json"):
            found.append(path)
    return sorted({path.resolve() for path in found})


def error(path: Path, message: str) -> None:
    print(f"ERROR: {path}: {message}", file=sys.stderr)


def validate_document(path: Path, data: Any) -> int:
    errors = 0
    if not isinstance(data, Mapping):
        error(path, "top-level JSON must be an object")
        return 1
    if data.get("schemaName") != SCHEMA_NAME:
        error(path, f"schemaName must be {SCHEMA_NAME!r}")
        errors += 1
    schema_version = data.get("schemaVersion")
    if schema_version not in SUPPORTED_SCHEMA_VERSIONS:
        error(path, f"schemaVersion must be one of {sorted(SUPPORTED_SCHEMA_VERSIONS)}")
        errors += 1
    if not isinstance(data.get("tu"), Mapping) or not str(data["tu"].get("sourceFile") or ""):
        error(path, "tu.sourceFile must be a non-empty string")
        errors += 1
    coverage = data.get("coverage")
    if not isinstance(coverage, Mapping):
        error(path, "coverage must be an object")
        errors += 1
    elif schema_version is not None and schema_version >= 3:
        if not str(coverage.get("declaredOuterPointerKindModel") or ""):
            error(path, "coverage.declaredOuterPointerKindModel must be a non-empty string in schema version 3+")
            errors += 1
        if not str(coverage.get("callbackMediationModel") or ""):
            error(path, "coverage.callbackMediationModel must be a non-empty string in schema version 3+")
            errors += 1
        if schema_version >= 5 and not str(coverage.get("fieldStorageClassificationModel") or ""):
            error(path, "coverage.fieldStorageClassificationModel must be a non-empty string in schema version 5+")
            errors += 1
        if schema_version >= 4:
            for field in (
                "declaredPointerInventoryModel",
                "declaredPointerInventoryScope",
            ):
                if not str(coverage.get(field) or ""):
                    error(path, f"coverage.{field} must be a non-empty string in schema version 4+")
                    errors += 1
            if coverage.get("declaredPointerInventoryScope") not in {
                "project-root-v1", "main-tu-fallback-v1",
            }:
                error(path, "coverage.declaredPointerInventoryScope is outside the fixed vocabulary")
                errors += 1
            if not isinstance(coverage.get("declaredPointerInventoryScopeComplete"), bool):
                error(path, "coverage.declaredPointerInventoryScopeComplete must be boolean in schema version 4+")
                errors += 1
            for field in (
                "declaredPointerInventoryRows",
                "declaredEligibleDataPointerRows",
                "declaredEligibleVoidPointerRows",
                "declaredEligibleVoidPointerSubjectFacts",
                "declaredEligibleVoidPointerFinalHistory",
                "declaredEligibleVoidPointerClassified",
                "declaredEligibleVoidPointerNoObservedFacts",
                "finalHistoryWithoutSubjectRow",
            ):
                value = coverage.get(field)
                if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                    error(path, f"coverage.{field} must be a non-negative integer in schema version 4+")
                    errors += 1
    if schema_version is not None and schema_version >= 6:
        for field in (
            "nestedCallbackSlotRelations",
            "voidPointerUsageFingerprints",
        ):
            if not isinstance(coverage.get(field), int) or isinstance(coverage.get(field), bool) or coverage.get(field) < 0:
                error(path, f"coverage.{field} must be a non-negative integer in schema version 6+")
                errors += 1
        for field in (
            "nestedCallbackSlotRelationModel",
            "voidPointerUsageFingerprintModel",
        ):
            if not str(coverage.get(field) or ""):
                error(path, f"coverage.{field} must be a non-empty string in schema version 6+")
                errors += 1
    if schema_version is not None and schema_version >= 7:
        if not str(coverage.get("observedNoFinalHistoryPatternModel") or ""):
            error(path, "coverage.observedNoFinalHistoryPatternModel must be a non-empty string in schema version 7+")
            errors += 1
    if schema_version is not None and schema_version >= 8:
        if not str(coverage.get("allocationOriginDerivedAliasModel") or ""):
            error(path, "coverage.allocationOriginDerivedAliasModel must be a non-empty string in schema version 8+")
            errors += 1
        value = coverage.get("allocationOriginDerivedAliasUses")
        if not isinstance(value, int) or isinstance(value, bool) or value < 0:
            error(path, "coverage.allocationOriginDerivedAliasUses must be a non-negative integer in schema version 8+")
            errors += 1
    if schema_version is not None and schema_version >= 9:
        for field in (
            "declaredDirectVoidPointerInventoryRows",
            "declaredExcludedUnavailableApiVoidPointerRows",
            "eligibleNestedCallbackSlotRelations",
            "excludedUnavailableApiNestedCallbackSlots",
        ):
            value = coverage.get(field)
            if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                error(path, f"coverage.{field} must be a non-negative integer in schema version 9+")
                errors += 1

    subjects = data.get("subjects")
    callbacks = data.get("callbackUseRecords")
    if not isinstance(subjects, list):
        error(path, "subjects must be an array")
        return errors + 1
    if not isinstance(callbacks, list):
        error(path, "callbackUseRecords must be an array")
        return errors + 1
    nested_slots = data.get("nestedCallbackSlotRelations", [])
    usage_fingerprints = data.get("voidPointerUsageFingerprints", [])
    allocation_alias_uses = data.get("allocationOriginDerivedAliasUses", [])
    if schema_version is not None and schema_version >= 6:
        if not isinstance(nested_slots, list):
            error(path, "nestedCallbackSlotRelations must be an array in schema version 6+")
            errors += 1
            nested_slots = []
        if not isinstance(usage_fingerprints, list):
            error(path, "voidPointerUsageFingerprints must be an array in schema version 6+")
            errors += 1
            usage_fingerprints = []
    if schema_version is not None and schema_version >= 8:
        if not isinstance(allocation_alias_uses, list):
            error(path, "allocationOriginDerivedAliasUses must be an array in schema version 8+")
            errors += 1
            allocation_alias_uses = []

    subject_ids: set[str] = set()
    for index, subject in enumerate(subjects):
        where = f"subjects[{index}]"
        if not isinstance(subject, Mapping):
            error(path, f"{where} must be an object")
            errors += 1
            continue
        subject_id = str(subject.get("subjectId") or "")
        if not subject_id:
            error(path, f"{where}.subjectId must be non-empty")
            errors += 1
        elif subject_id in subject_ids:
            error(path, f"duplicate subjectId {subject_id!r}")
            errors += 1
        else:
            subject_ids.add(subject_id)
        for field in ("rootSubjectId", "entity", "subjectKind", "primaryCategory"):
            if not str(subject.get(field) or ""):
                error(path, f"{where}.{field} must be non-empty")
                errors += 1
        if subject.get("primaryCategory") not in CATEGORIES:
            error(path, f"{where}.primaryCategory is outside the fixed vocabulary")
            errors += 1
        if not isinstance(subject.get("historyInstances"), list):
            error(path, f"{where}.historyInstances must be an array")
            errors += 1
        if not isinstance(subject.get("history"), Mapping):
            error(path, f"{where}.history must be an object")
            errors += 1
        if not isinstance(subject.get("operations"), Mapping):
            error(path, f"{where}.operations must be an object")
            errors += 1
        if not isinstance(subject.get("refinementFacts"), Mapping):
            error(path, f"{where}.refinementFacts must be an object")
            errors += 1
        callback_mediation = subject.get("callbackMediation")
        if schema_version is not None and schema_version >= 3:
            if not isinstance(callback_mediation, Mapping):
                error(path, f"{where}.callbackMediation must be an object in schema version 3+")
                errors += 1
            else:
                state = str(callback_mediation.get("state") or "")
                if state not in CALLBACK_MEDIATION_STATES:
                    error(path, f"{where}.callbackMediation.state is outside the fixed vocabulary")
                    errors += 1
                flags = callback_mediation.get("flags")
                if not isinstance(flags, Mapping):
                    error(path, f"{where}.callbackMediation.flags must be an object")
                    errors += 1
                else:
                    for flag in ("callback", "resolvedTarget", "fieldCarrier", "guardedSelection"):
                        if not isinstance(flags.get(flag), bool):
                            error(path, f"{where}.callbackMediation.flags.{flag} must be boolean")
                            errors += 1
        if schema_version is not None and schema_version >= 5 and subject.get("subjectKind") == "field-storage":
            field_storage = subject.get("fieldStorage")
            if not isinstance(field_storage, Mapping):
                error(path, f"{where}.fieldStorage must be an object for field-storage subjects")
                errors += 1
            else:
                if field_storage.get("classificationScope") != "instance":
                    error(path, f"{where}.fieldStorage.classificationScope must be instance")
                    errors += 1
                if not str(field_storage.get("baseEntity") or ""):
                    error(path, f"{where}.fieldStorage.baseEntity must be non-empty")
                    errors += 1
                if not str(field_storage.get("baseSubjectId") or ""):
                    error(path, f"{where}.fieldStorage.baseSubjectId must be non-empty")
                    errors += 1
                if not isinstance(field_storage.get("storageLocation"), Mapping):
                    error(path, f"{where}.fieldStorage.storageLocation must be an object")
                    errors += 1
                if not isinstance(field_storage.get("fieldPath"), list) or not field_storage.get("fieldPath"):
                    error(path, f"{where}.fieldStorage.fieldPath must be a non-empty array")
                    errors += 1
                declaring = field_storage.get("declaringField")
                if not isinstance(declaring, Mapping):
                    error(path, f"{where}.fieldStorage.declaringField must be an object")
                    errors += 1
                elif not isinstance(declaring.get("location"), Mapping):
                    error(path, f"{where}.fieldStorage.declaringField.location must be an object")
                    errors += 1
        declared_type = subject.get("declaredType")
        if not isinstance(declared_type, Mapping):
            error(path, f"{where}.declaredType must be an object")
            errors += 1
        elif schema_version is not None and schema_version >= 3:
            outer_kind = str(declared_type.get("outerPointerKind") or "")
            if outer_kind not in OUTER_POINTER_KINDS:
                error(path, f"{where}.declaredType.outerPointerKind is outside the fixed vocabulary")
                errors += 1
            outer_void = declared_type.get("outerVoidPointer")
            if not isinstance(outer_void, bool):
                error(path, f"{where}.declaredType.outerVoidPointer must be boolean")
                errors += 1
            elif (outer_kind == "void-data-pointer") != outer_void:
                error(path, f"{where}.declaredType outerVoidPointer and outerPointerKind disagree")
                errors += 1

    callback_ids: set[str] = set()
    if schema_version is not None and schema_version >= 4:
        inventory = data.get("declaredPointerInventory")
        if not isinstance(inventory, list):
            error(path, "declaredPointerInventory must be an array in schema version 4+")
            errors += 1
        else:
            declaration_ids: set[str] = set()
            statuses = INVENTORY_STATUSES
            inventory_counts = {
                "rows": 0,
                "eligibleData": 0,
                "eligibleVoid": 0,
                "directVoidInventory": 0,
                "excludedUnavailableApiVoid": 0,
                "voidSubjectFacts": 0,
                "voidFinalHistory": 0,
                "voidClassified": 0,
                "voidNoObservedFacts": 0,
                "finalHistoryWithoutSubjectRow": 0,
            }
            for index, row in enumerate(inventory):
                where = f"declaredPointerInventory[{index}]"
                if not isinstance(row, Mapping):
                    error(path, f"{where} must be an object")
                    errors += 1
                    continue
                inventory_counts["rows"] += 1
                declaration_id = str(row.get("declarationId") or "")
                if not declaration_id:
                    error(path, f"{where}.declarationId must be non-empty")
                    errors += 1
                elif declaration_id in declaration_ids:
                    error(path, f"duplicate declarationId {declaration_id!r}")
                    errors += 1
                else:
                    declaration_ids.add(declaration_id)
                if not str(row.get("subjectId") or ""):
                    error(path, f"{where}.subjectId must be non-empty")
                    errors += 1
                if row.get("subjectKind") not in {"local", "parameter"}:
                    error(path, f"{where}.subjectKind must be local or parameter")
                    errors += 1
                if not isinstance(row.get("location"), Mapping):
                    error(path, f"{where}.location must be an object")
                    errors += 1
                if not isinstance(row.get("function"), Mapping):
                    error(path, f"{where}.function must be an object")
                    errors += 1
                declared_type = row.get("declaredType")
                if not isinstance(declared_type, Mapping):
                    error(path, f"{where}.declaredType must be an object")
                    errors += 1
                    outer_kind = ""
                    outer_void = None
                else:
                    outer_kind = str(declared_type.get("outerPointerKind") or "")
                    outer_void = declared_type.get("outerVoidPointer")
                    if outer_kind not in OUTER_POINTER_KINDS:
                        error(path, f"{where}.declaredType.outerPointerKind is outside the fixed vocabulary")
                        errors += 1
                    if not isinstance(outer_void, bool):
                        error(path, f"{where}.declaredType.outerVoidPointer must be boolean")
                        errors += 1
                    elif (outer_kind == "void-data-pointer") != outer_void:
                        error(path, f"{where}.declaredType outerVoidPointer and outerPointerKind disagree")
                        errors += 1
                eligible = row.get("eligibleForCastHistoryClassification")
                emitted = row.get("subjectFactEmitted")
                final_history = row.get("hasFinalHistory")
                for field, value in (
                    ("eligibleForCastHistoryClassification", eligible),
                    ("subjectFactEmitted", emitted),
                    ("hasFinalHistory", final_history),
                ):
                    if not isinstance(value, bool):
                        error(path, f"{where}.{field} must be boolean")
                        errors += 1
                status = str(row.get("classificationStatus") or "")
                if status not in statuses:
                    error(path, f"{where}.classificationStatus is outside the fixed vocabulary")
                    errors += 1
                expected = {
                    "classified": (True, True),
                    "observed-no-final-history": (True, False),
                    "final-history-not-emitted": (False, True),
                    "declared-no-observed-facts": (False, False),
                    "out-of-scope-unavailable-api-parameter": (False, False),
                }
                if status in expected and (emitted, final_history) != expected[status]:
                    error(path, f"{where}.{status} has inconsistent subjectFactEmitted/hasFinalHistory flags")
                    errors += 1
                if schema_version is not None and schema_version >= 9:
                    scope = str(row.get("semanticClassificationScope") or "")
                    owner_definition_available = row.get("ownerDefinitionAvailable")
                    exclusion_reason = str(row.get("eligibilityExclusionReason") or "")
                    if scope not in SEMANTIC_CLASSIFICATION_SCOPES:
                        error(path, f"{where}.semanticClassificationScope is outside the fixed vocabulary")
                        errors += 1
                    if not isinstance(owner_definition_available, bool):
                        error(path, f"{where}.ownerDefinitionAvailable must be boolean")
                        errors += 1
                    if exclusion_reason not in ELIGIBILITY_EXCLUSION_REASONS:
                        error(path, f"{where}.eligibilityExclusionReason is outside the fixed vocabulary")
                        errors += 1
                    if status == "out-of-scope-unavailable-api-parameter":
                        if eligible is not False or scope != "external-or-unavailable-api" or \
                                owner_definition_available is not False or \
                                exclusion_reason != "unavailable-owner-function-body":
                            error(path, f"{where}.out-of-scope-unavailable-api-parameter lacks the required eligibility boundary")
                            errors += 1
                    elif eligible is True and scope != "implementation-body":
                        error(path, f"{where}.eligible semantic row must have implementation-body scope")
                        errors += 1
                if eligible is True:
                    inventory_counts["eligibleData"] += 1
                if outer_kind == "void-data-pointer":
                    inventory_counts["directVoidInventory"] += 1
                    if status == "out-of-scope-unavailable-api-parameter":
                        inventory_counts["excludedUnavailableApiVoid"] += 1
                    if eligible is True:
                        inventory_counts["eligibleVoid"] += 1
                        inventory_counts["voidSubjectFacts"] += emitted is True
                        inventory_counts["voidFinalHistory"] += final_history is True
                        inventory_counts["voidClassified"] += status == "classified"
                        inventory_counts["voidNoObservedFacts"] += status == "declared-no-observed-facts"
                        inventory_counts["finalHistoryWithoutSubjectRow"] += status == "final-history-not-emitted"
            if isinstance(coverage, Mapping):
                expected_coverage = {
                    "declaredPointerInventoryRows": inventory_counts["rows"],
                    "declaredEligibleDataPointerRows": inventory_counts["eligibleData"],
                    "declaredEligibleVoidPointerRows": inventory_counts["eligibleVoid"],
                    "declaredEligibleVoidPointerSubjectFacts": inventory_counts["voidSubjectFacts"],
                    "declaredEligibleVoidPointerFinalHistory": inventory_counts["voidFinalHistory"],
                    "declaredEligibleVoidPointerClassified": inventory_counts["voidClassified"],
                    "declaredEligibleVoidPointerNoObservedFacts": inventory_counts["voidNoObservedFacts"],
                    "finalHistoryWithoutSubjectRow": inventory_counts["finalHistoryWithoutSubjectRow"],
                }
                if schema_version is not None and schema_version >= 9:
                    expected_coverage.update({
                        "declaredDirectVoidPointerInventoryRows": inventory_counts["directVoidInventory"],
                        "declaredExcludedUnavailableApiVoidPointerRows": inventory_counts["excludedUnavailableApiVoid"],
                    })
                for field, expected_value in expected_coverage.items():
                    if coverage.get(field) != expected_value:
                        error(path, f"coverage.{field}={coverage.get(field)!r} does not match inventory value {expected_value}")
                        errors += 1

    for index, callback in enumerate(callbacks):
        where = f"callbackUseRecords[{index}]"
        if not isinstance(callback, Mapping):
            error(path, f"{where} must be an object")
            errors += 1
            continue
        callback_id = str(callback.get("callbackUseId") or "")
        if not callback_id:
            error(path, f"{where}.callbackUseId must be non-empty")
            errors += 1
        elif callback_id in callback_ids:
            error(path, f"duplicate callbackUseId {callback_id!r}")
            errors += 1
        else:
            callback_ids.add(callback_id)
        subject_id = str(callback.get("subjectId") or "")
        if subject_id not in subject_ids:
            error(path, f"{where}.subjectId does not reference a TU subject")
            errors += 1
        if not isinstance(callback.get("flags"), Mapping):
            error(path, f"{where}.flags must be an object")
            errors += 1
    if schema_version is not None and schema_version >= 6:
        slot_ids: set[str] = set()
        eligible_slot_count = 0
        excluded_api_slot_count = 0
        for index, slot in enumerate(nested_slots):
            where = f"nestedCallbackSlotRelations[{index}]"
            if not isinstance(slot, Mapping):
                error(path, f"{where} must be an object")
                errors += 1
                continue
            slot_id = str(slot.get("slotId") or "")
            if not slot_id or slot_id in slot_ids:
                error(path, f"{where}.slotId must be unique and non-empty")
                errors += 1
            slot_ids.add(slot_id)
            for field in (
                "ownerDeclarationId", "relationVariable", "callbackRole",
                "positionKind", "positionPath",
            ):
                if not str(slot.get(field) or ""):
                    error(path, f"{where}.{field} must be non-empty")
                    errors += 1
            if not isinstance(slot.get("correlatedDirectSubjects"), list):
                error(path, f"{where}.correlatedDirectSubjects must be an array")
                errors += 1
            if not isinstance(slot.get("invocation"), Mapping):
                error(path, f"{where}.invocation must be an object")
                errors += 1
            if schema_version is not None and schema_version >= 9:
                eligible = slot.get("eligibleForRelationalClassification")
                owner_definition_available = slot.get("ownerDefinitionAvailable")
                scope = str(slot.get("relationalClassificationScope") or "")
                exclusion_reason = str(slot.get("eligibilityExclusionReason") or "")
                if not isinstance(eligible, bool):
                    error(path, f"{where}.eligibleForRelationalClassification must be boolean")
                    errors += 1
                if not isinstance(owner_definition_available, bool):
                    error(path, f"{where}.ownerDefinitionAvailable must be boolean")
                    errors += 1
                if scope not in RELATIONAL_CLASSIFICATION_SCOPES:
                    error(path, f"{where}.relationalClassificationScope is outside the fixed vocabulary")
                    errors += 1
                if exclusion_reason not in ELIGIBILITY_EXCLUSION_REASONS:
                    error(path, f"{where}.eligibilityExclusionReason is outside the fixed vocabulary")
                    errors += 1
                if eligible is True:
                    eligible_slot_count += 1
                elif exclusion_reason == "unavailable-owner-function-body":
                    excluded_api_slot_count += 1
                    if scope != "external-or-unavailable-api" or owner_definition_available is not False:
                        error(path, f"{where}.unavailable API slot lacks the required eligibility boundary")
                        errors += 1
        if schema_version is not None and schema_version >= 9 and isinstance(coverage, Mapping):
            if coverage.get("eligibleNestedCallbackSlotRelations") != eligible_slot_count:
                error(path, "coverage.eligibleNestedCallbackSlotRelations must equal the eligible nested callback-slot count")
                errors += 1
            if coverage.get("excludedUnavailableApiNestedCallbackSlots") != excluded_api_slot_count:
                error(path, "coverage.excludedUnavailableApiNestedCallbackSlots must equal the unavailable API nested callback-slot count")
                errors += 1
        usage_ids: set[str] = set()
        for index, usage in enumerate(usage_fingerprints):
            where = f"voidPointerUsageFingerprints[{index}]"
            if not isinstance(usage, Mapping):
                error(path, f"{where} must be an object")
                errors += 1
                continue
            usage_id = str(usage.get("usageId") or "")
            if not usage_id or usage_id in usage_ids:
                error(path, f"{where}.usageId must be unique and non-empty")
                errors += 1
            usage_ids.add(usage_id)
            if str(usage.get("classificationStatus") or "") not in INVENTORY_STATUSES:
                error(path, f"{where}.classificationStatus is outside the fixed vocabulary")
                errors += 1
            if str(usage.get("genericCandidateConfidence") or "") not in {
                "not-applicable", "strong", "conditional", "blocked",
                "insufficient-evidence",
            }:
                error(path, f"{where}.genericCandidateConfidence is outside the fixed vocabulary")
                errors += 1
            if not isinstance(usage.get("usageFlags"), Mapping):
                error(path, f"{where}.usageFlags must be an object")
                errors += 1
            if schema_version is not None and schema_version >= 7:
                no_final = usage.get("observedNoFinalHistory")
                if not isinstance(no_final, Mapping):
                    error(path, f"{where}.observedNoFinalHistory must be an object in schema version 7+")
                    errors += 1
                else:
                    pattern = str(no_final.get("primaryPattern") or "")
                    if pattern not in OBSERVED_NO_FINAL_HISTORY_PATTERNS:
                        error(path, f"{where}.observedNoFinalHistory.primaryPattern is outside the fixed vocabulary")
                        errors += 1
                    status = str(usage.get("classificationStatus") or "")
                    if status == "observed-no-final-history" and pattern == "not-applicable":
                        error(path, f"{where}.observedNoFinalHistory primary pattern must explain observed-no-final-history")
                        errors += 1
                    if status != "observed-no-final-history" and pattern != "not-applicable":
                        error(path, f"{where}.observedNoFinalHistory primary pattern must be not-applicable outside observed-no-final-history")
                        errors += 1
                    for field in (
                        "hasRetainedPointerUse", "hasRetainedObservation",
                        "hasDerefOrArrayIndex", "hasPointerArithmetic",
                        "hasPointerComparisonOrNullTest", "hasKnownInternalForwarding",
                        "hasExternalOrUnresolvedEscape", "hasCallbackMediatedForwarding",
                        "hasReturnOrStorageFlow", "hasCopyAssignmentOrCastEvidence",
                    ):
                        if not isinstance(no_final.get(field), bool):
                            error(path, f"{where}.observedNoFinalHistory.{field} must be boolean")
                            errors += 1
                    for field in ("pointerUseKinds", "semanticOperationKinds"):
                        if not isinstance(no_final.get(field), list) or not all(isinstance(x, str) for x in no_final.get(field, [])):
                            error(path, f"{where}.observedNoFinalHistory.{field} must be an array of strings")
                            errors += 1

    if schema_version is not None and schema_version >= 8:
        if isinstance(coverage.get("allocationOriginDerivedAliasUses"), int) and \
                coverage.get("allocationOriginDerivedAliasUses") != len(allocation_alias_uses):
            error(path, "coverage.allocationOriginDerivedAliasUses must equal allocationOriginDerivedAliasUses length")
            errors += 1
        report_ids: set[str] = set()
        for index, report in enumerate(allocation_alias_uses):
            where = f"allocationOriginDerivedAliasUses[{index}]"
            if not isinstance(report, Mapping):
                error(path, f"{where} must be an object")
                errors += 1
                continue
            report_id = str(report.get("reportId") or "")
            if not report_id or report_id in report_ids:
                error(path, f"{where}.reportId must be unique and non-empty")
                errors += 1
            report_ids.add(report_id)
            for field in (
                "originSubjectId", "originDeclarationId", "originEntity",
                "aliasDeclarationId", "aliasEntity",
            ):
                if not str(report.get(field) or ""):
                    error(path, f"{where}.{field} must be non-empty")
                    errors += 1
            for field in ("originLocation", "originFunction", "originDeclaredType",
                          "aliasLocation", "aliasFunction", "aliasDeclaredType",
                          "usageFlags"):
                if not isinstance(report.get(field), Mapping):
                    error(path, f"{where}.{field} must be an object")
                    errors += 1
            for field in ("allocatorFamilies", "allocatorFunctions", "allocationCallSites",
                          "originBindingKinds", "derivationKinds", "derivationSites",
                          "evidenceKinds", "useSites"):
                value = report.get(field)
                if not isinstance(value, list) or not all(isinstance(x, str) for x in value):
                    error(path, f"{where}.{field} must be an array of strings")
                    errors += 1
            if report.get("mayAlias") is not True:
                error(path, f"{where}.mayAlias must be true for this report-only may-alias model")
                errors += 1

    return errors


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Validate C6g5f4 compact paper-facts sidecars.")
    parser.add_argument("paths", nargs="*", type=Path)
    parser.add_argument("--root", type=Path)
    args = parser.parse_args(argv)
    paths = discover(args.paths, args.root)
    if not paths:
        print("error: no *.cast-paper-facts.json files found", file=sys.stderr)
        return 2
    failures = 0
    for path in paths:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            error(path, f"cannot parse JSON: {exc}")
            failures += 1
            continue
        failures += validate_document(path, data)
    if failures:
        return 1
    print(f"validated {len(paths)} C6g5f4 paper-facts sidecar(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
