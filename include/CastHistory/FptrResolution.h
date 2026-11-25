#pragma once

// Function-pointer (fptr) extension: indirect-call resolution facet.
//
// Design: design/fptr-extension/02-design.md §4.2, decision D12 (accepted with
// owner amendments), backbone contract 06-backbone-contract.md (seam S8).
//
// M1 scope: reporting only.  The facet is *derived* from the pointer-use
// records the visitor already produced; it never changes history solving,
// categories or callbackMediation.  Later milestones (L1 slots, LIB, L2) add
// states/layers here without touching the visitor beyond their seams.
//
// Facts served (M1 claims): F01 F02 F25 F31 F38 F39 F40 F41.

#include "CastHistory/AnalysisState.h"
#include "CastHistory/HistoryMetrics.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/JSON.h>

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <tuple>

namespace cast_history {

enum class IndirectResolutionState {
  None,            // the subject never reaches an indirect call
  Unresolved,
  ExternalUnknown, // reserved (M7)
  Signature,       // reserved (M9, L2)
  ExternalModeled, // reserved (M7)
  Set,
  Parametric,
  Exact,
};

enum class IndirectHistoryComposition { Full, Partial, None };

inline const char *indirectResolutionStateName(IndirectResolutionState s) {
  switch (s) {
  case IndirectResolutionState::None: return "none";
  case IndirectResolutionState::Unresolved: return "unresolved";
  case IndirectResolutionState::ExternalUnknown: return "external-unknown";
  case IndirectResolutionState::Signature: return "signature";
  case IndirectResolutionState::ExternalModeled: return "external-modeled";
  case IndirectResolutionState::Set: return "set";
  case IndirectResolutionState::Parametric: return "parametric";
  case IndirectResolutionState::Exact: return "exact";
  }
  return "unresolved";
}

// D12 (owner amendment): unresolved < external-unknown < signature <
// external-modeled < set < parametric = exact.
inline int indirectResolutionRank(IndirectResolutionState s) {
  switch (s) {
  case IndirectResolutionState::None: return 100; // never the weakest
  case IndirectResolutionState::Unresolved: return 0;
  case IndirectResolutionState::ExternalUnknown: return 1;
  case IndirectResolutionState::Signature: return 2;
  case IndirectResolutionState::ExternalModeled: return 3;
  case IndirectResolutionState::Set: return 4;
  case IndirectResolutionState::Parametric: return 5;
  case IndirectResolutionState::Exact: return 5;
  }
  return 0;
}

inline const char *indirectHistoryCompositionName(IndirectHistoryComposition c) {
  switch (c) {
  case IndirectHistoryComposition::Full: return "full";
  case IndirectHistoryComposition::Partial: return "partial";
  case IndirectHistoryComposition::None: return "none";
  }
  return "none";
}

// Layers ordered by how much precision was given up to reach the answer.
inline int fptrLayerRank(const std::string &layer) {
  if (layer == "L0") return 1;
  if (layer == "L1") return 2;
  if (layer == "LIB") return 3;
  if (layer == "L2") return 4;
  return 0; // "-"
}

struct IndirectCallSiteFacet {
  std::string callee;
  IndirectResolutionState state = IndirectResolutionState::Unresolved;
  std::string layer = "-";
  std::set<std::string> targets; // ordered by name: deterministic (D21)
  bool guarded = false;
  IndirectHistoryComposition composition = IndirectHistoryComposition::None;
  std::optional<bool> parametricClosed; // set iff state == Parametric
  bool signatureMismatch = false;
  SourceSpan span;
};

struct IndirectCallResolutionFacet {
  IndirectResolutionState state = IndirectResolutionState::None;
  std::string layer = "-";
  bool signatureMismatch = false;
  llvm::SmallVector<IndirectCallSiteFacet, 2> sites;
};

namespace detail_fptr {

inline bool contains(const std::string &s, const char *needle) {
  return s.find(needle) != std::string::npos;
}

// An indirect call reached by the subject: the call-argument record the
// visitor writes for `cb(x)` ("arg0 to <indirect>"), possibly reached through
// a direct call ("via run.$1; arg0 to <indirect>").
inline bool isIndirectCallAnchor(const PointerUseRecord &u) {
  return u.kind == PointerUseKind::CallArgument && u.span.line != 0 &&
         contains(u.detail, "to <indirect>");
}

// A record that carries evidence composed from a resolved indirect target.
inline bool isComposedTargetRecord(const PointerUseRecord &u) {
  const PointerUseProvenance &p = u.provenance;
  if (p.targetFunction.empty() && p.targetQualifiedName.empty())
    return false;
  if (contains(p.edgeKind, "function-pointer") || contains(p.source, "fnptr") ||
      contains(p.transferKind, "fnptr"))
    return true;
  const std::set<std::string> labels = pointerUseEvidenceSourceLabels(u);
  return labels.count("resolved-higher-order-pointer-use") ||
         labels.count("imported-function-pointer-pointer-use");
}

inline std::string targetName(const PointerUseProvenance &p) {
  return p.targetQualifiedName.empty() ? p.targetFunction
                                       : p.targetQualifiedName;
}

inline std::tuple<std::string, unsigned, unsigned> spanKey(const SourceSpan &s) {
  return {s.file, s.line, s.column};
}

inline std::string calleeTextFromDetail(const std::string &detail) {
  // "arg0 to <indirect>" -> "<indirect>"; "via run.$1; arg0 to <indirect>"
  // -> "run.$1 -> <indirect>".  The callee expression itself is not recorded
  // on the anchor today; M2's resolver (seam S6) records it.
  const std::string via = "via ";
  if (detail.rfind(via, 0) == 0) {
    size_t semi = detail.find(';');
    if (semi != std::string::npos)
      return detail.substr(via.size(), semi - via.size()) + " -> <indirect>";
  }
  return "<indirect>";
}

} // namespace detail_fptr

// Build the facet for one subject from its pointer-use records.
//
// hasSymbolicCallbackRefs: the subject's own summary still contains a
//   higher-order reference ($k.$i), i.e. the callee is a formal parameter.
// enclosingCallbackFormalsClosed: whether every in-TU direct caller of the
//   subject's function resolves its function-pointer actuals (nullopt when
//   unknown); used for parametricClosure.
inline IndirectCallResolutionFacet buildIndirectCallResolutionFacet(
    const std::vector<PointerUseRecord> *uses, bool hasSymbolicCallbackRefs,
    std::optional<bool> enclosingCallbackFormalsClosed) {
  using namespace detail_fptr;
  IndirectCallResolutionFacet facet;
  if (!uses)
    return facet;

  // Group anchors and composed records by call-site span.
  std::map<std::tuple<std::string, unsigned, unsigned>, IndirectCallSiteFacet>
      bySpan;
  for (const PointerUseRecord &u : *uses) {
    if (!isIndirectCallAnchor(u))
      continue;
    auto &site = bySpan[spanKey(u.span)];
    site.span = u.span;
    if (site.callee.empty() || site.callee == "<indirect>")
      site.callee = calleeTextFromDetail(u.detail);
  }
  for (const PointerUseRecord &u : *uses) {
    if (u.span.line == 0 || !isComposedTargetRecord(u))
      continue;
    auto it = bySpan.find(spanKey(u.span));
    if (it == bySpan.end())
      continue;
    const std::string name = targetName(u.provenance);
    if (!name.empty())
      it->second.targets.insert(name);
    if (callbackGuardLooksNontrivial(u))
      it->second.guarded = true;
  }

  for (auto &entry : bySpan) {
    IndirectCallSiteFacet &site = entry.second;
    if (site.targets.size() == 1) {
      site.state = IndirectResolutionState::Exact;
      site.layer = "L0";
    } else if (site.targets.size() > 1) {
      site.state = IndirectResolutionState::Set;
      site.layer = "L0";
    } else if (hasSymbolicCallbackRefs) {
      site.state = IndirectResolutionState::Parametric;
      site.layer = "L0";
      site.parametricClosed = enclosingCallbackFormalsClosed.value_or(false);
    } else {
      site.state = IndirectResolutionState::Unresolved;
      site.layer = "-";
    }
    // M1: targets are only known through records that carry their evidence,
    // so a resolved site is fully composed.  "partial" appears once L1 slot
    // targets can be known without a transferred summary (M3).
    site.composition = site.targets.empty() ? IndirectHistoryComposition::None
                                            : IndirectHistoryComposition::Full;
    facet.sites.push_back(std::move(site));
  }

  if (facet.sites.empty())
    return facet;
  facet.state = facet.sites.front().state;
  for (const IndirectCallSiteFacet &site : facet.sites) {
    const int r = indirectResolutionRank(site.state);
    const int cur = indirectResolutionRank(facet.state);
    if (r < cur || (r == cur && site.state == IndirectResolutionState::Parametric))
      facet.state = site.state; // weakest; parametric wins a tie with exact
    if (fptrLayerRank(site.layer) > fptrLayerRank(facet.layer))
      facet.layer = site.layer;
    facet.signatureMismatch = facet.signatureMismatch || site.signatureMismatch;
  }
  return facet;
}

inline llvm::json::Value
indirectCallResolutionJson(const IndirectCallResolutionFacet &facet) {
  llvm::json::Array sites;
  for (const IndirectCallSiteFacet &site : facet.sites) {
    llvm::json::Array targets;
    for (const std::string &t : site.targets)
      targets.push_back(t);
    sites.push_back(llvm::json::Object{
        {"callee", site.callee},
        {"slot", nullptr},
        {"state", indirectResolutionStateName(site.state)},
        {"layer", site.layer},
        {"targets", std::move(targets)},
        {"guarded", site.guarded},
        {"indirectHistoryComposition",
         indirectHistoryCompositionName(site.composition)},
        {"parametricClosure",
         site.parametricClosed
             ? llvm::json::Value(*site.parametricClosed ? "closed" : "open")
             : llvm::json::Value(nullptr)},
        {"signatureMismatch", site.signatureMismatch},
        {"span", llvm::json::Object{{"file", site.span.file},
                                    {"line", static_cast<int64_t>(site.span.line)},
                                    {"column",
                                     static_cast<int64_t>(site.span.column)}}}});
  }
  return llvm::json::Object{{"state", indirectResolutionStateName(facet.state)},
                            {"layer", facet.layer},
                            {"signatureMismatch", facet.signatureMismatch},
                            {"sites", std::move(sites)}};
}

} // namespace cast_history
