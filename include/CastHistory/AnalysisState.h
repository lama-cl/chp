#pragma once

#include "CastHistory/HistoryIR.h"

#include <clang/AST/Decl.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cast_history {

using clang::FunctionDecl;
using clang::ValueDecl;

// Entity model
//
// Design goal linkage:
//   - Moves the tool away from "history only lives on a local variable name".
//   - Prepares for returns, fields, and later field sensitivity.
//   - Provides a natural hook for future alias-sensitive entity unification.
// ============================================================================

struct EntityKey {
  enum class Kind {
    ValueDeclEntity,
    ReturnEntity,
    FieldEntity
  };

  Kind kind = Kind::ValueDeclEntity;
  const ValueDecl *decl = nullptr;
  const FunctionDecl *function = nullptr;
  std::vector<std::string> fieldPath;

  bool operator<(const EntityKey &o) const {
    if (kind != o.kind) return kind < o.kind;
    if (decl != o.decl) return decl < o.decl;
    if (function != o.function) return function < o.function;
    return fieldPath < o.fieldPath;
  }
};

static EntityKey entityOfValueDecl(const ValueDecl *vd) {
  EntityKey k;
  k.kind = EntityKey::Kind::ValueDeclEntity;
  k.decl = vd;
  return k;
}

static EntityKey entityOfReturn(const FunctionDecl *fd) {
  EntityKey k;
  k.kind = EntityKey::Kind::ReturnEntity;
  k.function = fd;
  return k;
}

struct Observation {
  // STEP 5 ATTACHMENT-PATH REFINEMENT:
  //   attachPath describes how we reached the local value from the source entity.
  //   obs describes the continuation observed from that local value.
  //
  // Example:
  //   char *pcho = pvho;  pf(pcho);
  //   For pvho, the higher-order call is recorded as:
  //     attachPath = char*
  //     obs        = void* => $0.$0
  //   The entity-history builder inserts obs under attachPath, yielding:
  //     void* => char* => void* => $0.$0
  ObsHistPtr attachPath; // nullptr means attach at the entity root.
  ObsHistPtr obs;
  std::optional<Guard> guard;
  std::string provenance;
  SourceSpan span;
};

// STEP 2C ADDITION: origin state with strong updates.
//
// Design goal linkage:
//   - Tracks the *current reaching source entity* for the value stored in a
//     variable, rather than a sticky historical origin.
//   - This is what makes sequences like `p = x; p = y; q = p;` attach q's later
//     observations to y rather than x.
//   - Each alternative carries an optional path from the source to the current
//     variable (e.g. pvho --char*--> pcho), which is later composed with new
//     observations such as $0.$0.
struct OriginAlternative {
  EntityKey source;
  ObsHistPtr pathFromSource; // nullptr means identity/no extra interpretation yet.
  std::optional<Guard> guard; // Used after control-flow joins.

  // C6g4l: source-level transfer tokens already applied on this reaching
  // lineage. The history IR intentionally remains source-location agnostic;
  // these tokens are transient flow-state metadata used only to prevent a
  // cyclic replay from applying the same source operation (for example one
  // `p += n` expression) repeatedly to its own result. They are not emitted
  // in text/JSON summaries and do not change ordinary distinct-site sequences.
  std::vector<std::string> appliedTransferTokens;
};

using OriginInfo = std::vector<OriginAlternative>;

// ============================================================================
// Function summaries
//
// Design goal linkage:
//   - Separate per-function summaries from local AST traversal order.
//   - Needed later for recursion, mutual recursion, and multi-TU solving.
//   - Param summaries are ObsHist rather than FlowHist because summaries are
//     about *how parameters are interpreted later*, not only local sequential
//     flow at one program point.
// ============================================================================


// ============================================================================
// Pointer-use summaries
//
// Design goal linkage:
//   - Cast histories tell us which pointer types an entity experiences.
//   - Use summaries tell us how values are used at each pointer type.
//   - The pointerType field is the current type at the use site, so operations
//     on a char* alias are attached to char*, not to the earlier void* or T*.
// ============================================================================

enum class PointerUseKind {
  Dereference,
  DereferenceWrite,
  DereferenceReadWrite,
  // C6g5f: array indexing is active pointer access.  New AST-local records
  // preserve the enclosing expression's access direction; ArrayIndex remains
  // for backwards-compatible import of summaries written before C6g5f.
  ArrayIndex,
  ArrayIndexRead,
  ArrayIndexWrite,
  ArrayIndexReadWrite,
  PointerArithmetic,
  PointerComparison,
  Stored,
  InitRHS,
  Returned,
  CallArgument,
  MemoryLibCallArgument,
  StringLibCallArgument,
  Unknown
};

static inline std::string pointerUseKindName(PointerUseKind kind) {
  switch (kind) {
  case PointerUseKind::Dereference: return "deref";
  case PointerUseKind::DereferenceWrite: return "deref-write";
  case PointerUseKind::DereferenceReadWrite: return "deref-readwrite";
  case PointerUseKind::ArrayIndex: return "array-index";
  case PointerUseKind::ArrayIndexRead: return "array-index-read";
  case PointerUseKind::ArrayIndexWrite: return "array-index-write";
  case PointerUseKind::ArrayIndexReadWrite: return "array-index-readwrite";
  case PointerUseKind::PointerArithmetic: return "pointer-arithmetic";
  case PointerUseKind::PointerComparison: return "pointer-comparison";
  case PointerUseKind::Stored: return "stored";
  case PointerUseKind::InitRHS: return "init-rhs";
  case PointerUseKind::Returned: return "returned";
  case PointerUseKind::CallArgument: return "call-arg";
  case PointerUseKind::MemoryLibCallArgument: return "memory-lib-call-arg";
  case PointerUseKind::StringLibCallArgument: return "string-lib-call-arg";
  case PointerUseKind::Unknown: return "unknown";
  }
  return "unknown";
}

// C6a: normalized pointer-use provenance.
//
// Pointer operations are a separate analysis product from cast histories.  The
// operation payload below still records "what happened" (deref-write, returned,
// pointer-arithmetic, etc.); this provenance record explains "where this fact
// came from" without overloading the human-readable detail string.
//
// The fields are intentionally strings so per-TU summaries, project-index
// materialization, and future table export can preserve provenance even when a
// precise AST FunctionDecl or FieldDecl is no longer available.
struct PointerUseProvenance {
  // Examples: local, same-tu-call, cross-tu-direct, cross-tu-fnptr,
  // field-fnptr, project-scc, return-alias, solved-index.
  std::string originKind;

  // Target/callee that contributed the operation, when applicable.
  std::string targetFunction;
  std::string targetQualifiedName;
  std::string targetLookupKey;

  // Field-function-pointer context, when the operation came through a struct
  // field such as ops.process(x).
  std::string fieldEntity;
  std::string fieldPath;
  std::string calleeExpr;
  std::string signatureKey;

  // Optional project/index transfer metadata.
  std::string source;
  std::string transferKind;
  std::string edgeKind;

  // Monotone reporting/evidence qualifier.  It records that at least one
  // contributing parameter-transfer route lies on a recursive named-call
  // cycle (including a project-SCC cycle that crosses translation units).
  // It is intentionally not part of semantic pointer-use membership: facts
  // that differ only by this bit are merged and the retained representative is
  // upgraded to true.
  bool recursiveNamedCallEvidence = false;

  bool empty() const {
    return originKind.empty() && targetFunction.empty() &&
           targetQualifiedName.empty() && targetLookupKey.empty() &&
           fieldEntity.empty() && fieldPath.empty() && calleeExpr.empty() &&
           signatureKey.empty() && source.empty() && transferKind.empty() &&
           edgeKind.empty() && !recursiveNamedCallEvidence;
  }
};

struct PointerUseRecord {
  ObsHistPtr attachPath; // nullptr means the use happens at the entity root.
  std::string pointerType; // current pointer expression type at the use site.
  // For dereference-like uses, accessType records the type of the object
  // accessed through the pointer.  Example: for *p where p has type int*,
  // pointerType is int* and accessType is int.
  std::optional<std::string> accessType;
  PointerUseKind kind = PointerUseKind::Unknown;
  std::optional<Guard> guard;
  std::string detail;
  SourceSpan span;
  PointerUseProvenance provenance;

  // C6g4p: preserve the guard at the leaf operation's own source site.
  // `guard` may later accumulate caller/relay path conditions during
  // interprocedural transfer; this field remains the callee-local guard used
  // for recursive same-TU guard widening.  It is reporting/solver metadata,
  // not a new pointer-use operation dimension.
  std::optional<Guard> operationLocalGuard;
  // Distinguishes a known-unconditional local guard from a legacy record that
  // predates operationLocalGuard serialization.
  bool operationLocalGuardKnown = false;

  // C6g4s: immutable identity of the original AST pointer-use leaf.  `span`
  // above remains the latest transfer/reporting site and can legitimately be
  // rewritten at named-call, data-field, function-pointer, and project-index
  // boundaries.  This field is set at the AST leaf and then only copied.
  SourceSpan operationSiteSpan;

  // Explains where operationLocalGuard came from.  Values emitted by C6g4s are
  // intentionally simple strings so JSON summaries remain forward-compatible:
  //   ast-leaf, preserved-transfer, imported-summary, legacy-fallback.
  // It is diagnostic/provenance metadata only and must not affect solver
  // membership.
  std::string operationLocalGuardOrigin;
};

struct ParamSummary {
  TypeAtom declaredType;

  // Existing full/report summary.  This remains the only parameter-history
  // channel used by ordinary same-TU and cross-TU import.
  ObsHistPtr obsSummary;

  // C6g5v: immutable AST-local cast/PtrArith contribution used solely by the
  // display-only canonical project-history materializer.  It is serialized as
  // companion metadata and never replaces obsSummary in import or SCC solving.
  ObsHistPtr directLocalObsSummary;
};

struct ReturnSummary {
  TypeAtom declaredType;
  ObsHistPtr obsSummary;
  bool mayAliasParam = false;
  std::optional<unsigned> aliasedParamIndex;
};

struct FunctionSummary {
  std::string name;
  const FunctionDecl *fd = nullptr;
  std::vector<ParamSummary> params;
  ReturnSummary ret;
};


// ============================================================================
// C4a summary dependency edges
//
// Design goal linkage:
//   - Per-TU summaries already contain rich cast histories, pointer uses, and
//     behavior classifications.
//   - Cross-TU recursion/SCC solving needs an explicit project-level dependency
//     graph over those summaries.
//   - The visitor records these edges while it still has AST call-site context;
//     build_summary_index.py later merges them across translation units.
// ============================================================================

enum class SummaryDependencyKind {
  DirectCall,
  HigherOrderParamCall,
  KnownFunctionPointerTargetCall,
  MultiFunctionPointerTargetCall,
  FunctionPointerActual
};

static inline const char *summaryDependencyKindName(SummaryDependencyKind kind) {
  switch (kind) {
  case SummaryDependencyKind::DirectCall: return "direct-call";
  case SummaryDependencyKind::HigherOrderParamCall: return "higher-order-param-call";
  case SummaryDependencyKind::KnownFunctionPointerTargetCall:
    return "known-function-pointer-target-call";
  case SummaryDependencyKind::MultiFunctionPointerTargetCall:
    return "multi-function-pointer-target-call";
  case SummaryDependencyKind::FunctionPointerActual:
    return "function-pointer-actual";
  }
  return "unknown";
}

struct SummaryDependencyEdge {
  SummaryDependencyKind kind = SummaryDependencyKind::DirectCall;

  // Caller function summary that contains the dependency site.
  std::string caller;
  std::string callerQualifiedName;
  std::string callerSignatureKey;

  // Concrete callee if known.  For higher-order parameter calls, callee is a
  // symbolic name such as "$0" and calleeParamIndex identifies the callback
  // parameter being invoked.
  std::string callee;
  std::string calleeQualifiedName;
  std::string calleeSignatureKey;
  std::string calleePublicLookupKey;

  // Indirect-call / callback metadata.
  std::string calleeExpr;
  std::string via;
  std::optional<unsigned> callerArgumentIndex;
  std::optional<unsigned> calleeParamIndex;
  std::optional<unsigned> calleeArgumentIndex;

  // Selection/path metadata.  For C3c guarded multi-target dispatch, guard is
  // the branch condition that selected this target.
  std::optional<Guard> guard;
  SourceSpan span;

  // Classification flags useful to the project indexer and later SCC logic.
  bool calleeHasLocalDefinition = false;
  bool crossTUCandidate = false;
  bool importedSummaryCandidate = false;
  bool resolvedBySummaryIndex = false;

  std::string detail;
};

} // namespace cast_history
