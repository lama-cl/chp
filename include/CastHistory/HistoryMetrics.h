#pragma once

#include "CastHistory/AnalysisState.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace cast_history {

// ============================================================================
// Cast-history metrics and behavior classification.
//
// Design goal linkage:
//   - Shape metrics are computed purely over the cast-history IR, so they remain
//     useful after field sensitivity, recursive summaries, and multi-TU support.
//   - History risk is also structural: it summarizes how translation-relevant
//     and potentially difficult the cast-history shape/type-view pattern is,
//     independent of dereference/read/write evidence.
//   - Behavior classification is operation-aware.  Cast histories answer
//     "which interpretations are reached?"; pointer-use summaries answer
//     "what operations are performed under those interpretations?"  The two
//     dimensions are intentionally reported separately as historyShape/
//     historyRisk versus behavior.
// ============================================================================

struct HistoryMetrics {
  int seqDepth = 0;
  int maxParallelBreadth = 0;
  int maxBranchBreadth = 0;
  int numSeqNodes = 0;
  int numParallelNodes = 0;
  int numBranchNodes = 0;
  int numCallWraps = 0;
  int numPtrArithNodes = 0;
  int numHORefs = 0;
  int numDistinctTypes = 0;

  int numPointerTypes = 0;
  int numNonVoidPointerTypes = 0;

  bool hasVoid = false;
  bool hasCharPtr = false;
  bool hasIntToPtr = false;
  bool hasPtrToInt = false;
  bool hasExternalSink = false;
  bool hasIndirectCallUse = false;
  bool hasPointerArithmetic = false;

  // Direction-sensitive byte-view/reinterpretation patterns.
  bool hasByteViewToConcrete = false;
  bool hasByteMediatedReinterpretation = false;
  bool hasSequentialConcreteReinterpretation = false;
  bool hasByteOffsetRoundTrip = false;
  bool hasByteOffsetToDifferentConcrete = false;
  bool hasTypeErasureToVoid = false;
  bool hasConcreteSpecializationFromVoid = false;
  bool hasParallelSequentialArm = false;

  // Human-readable representatives for output.
  std::set<std::string> allTypes;
  std::set<std::string> pointerTypes;
  std::set<std::string> nonVoidPointerTypes;
  std::set<std::string> ptrArithTypes;

  // Semantic keys used for metric decisions/counts. These are populated from
  // TypeAtom when available and fall back to rendered text for legacy nodes.
  std::set<std::string> allTypeKeys;
  std::set<std::string> pointerTypeKeys;
  std::set<std::string> nonVoidPointerTypeKeys;
  std::set<std::string> ptrArithTypeKeys;

  int numStructuredTypeAtoms = 0;
  int numLegacyTextTypes = 0;
};

// Shape: what the history looks like structurally.
// FunctionParameterDependent is used for symbolic higher-order effects such as
// $0.$0 in a function summary.  This is parametric behavior, not necessarily
// an unresolved analysis failure at a concrete call site.
enum class HistoryShapeClass {
  StableViaVoid,
  ByteView,
  // A non-branching history crosses two or more distinct concrete pointer
  // views in sequence (for example double* => void* => int*).  This is
  // structurally meaningful even when it is wrapped by a direct call and has
  // no dereference evidence.
  SequentialConcreteReinterpretation,
  ParallelPolymorphic,
  BranchPolymorphic,
  FunctionParameterDependent,
  HigherOrderAffected,
  Unknown
};

// Risk: structural translation concern from the cast history only.
// Operation-aware labels such as ByteViewWrite are retained as legacy enum
// values, but classifyHistoryRisk intentionally does not inspect pointer-use
// records. FunctionParameterDependent means the history depends on a
// function-valued parameter. HigherOrderUnresolved is kept for future use when
// a concrete call-site callback target remains unknown after resolution.
enum class HistoryRiskClass {
  Low,
  Moderate,
  ByteViewRead,
  ByteViewWrite,
  BranchPolymorphicWrite,
  SuspiciousReinterpretation,
  DangerousReinterpretation,
  DangerousProvenance,
  FunctionParameterDependent,
  HigherOrderUnresolved,
  ExternalSink,
  Unknown
};

// Backward-compatible coarse class name used by earlier output.  Newer output
// should prefer shape+risk.
enum class HistoryClass {
  StableViaVoid,
  ByteView,
  ParallelPolymorphic,
  BranchPolymorphic,
  DangerousProvenance,
  FunctionParameterDependent,
  HigherOrderUnresolved,
  ExternalSink,
  Unknown
};

static inline std::string historyShapeClassName(HistoryShapeClass c) {
  switch (c) {
  case HistoryShapeClass::StableViaVoid: return "StableViaVoid";
  case HistoryShapeClass::ByteView: return "ByteView";
  case HistoryShapeClass::SequentialConcreteReinterpretation:
    return "SequentialConcreteReinterpretation";
  case HistoryShapeClass::ParallelPolymorphic: return "ParallelPolymorphic";
  case HistoryShapeClass::BranchPolymorphic: return "BranchPolymorphic";
  case HistoryShapeClass::FunctionParameterDependent: return "FunctionParameterDependent";
  case HistoryShapeClass::HigherOrderAffected: return "HigherOrderAffected";
  case HistoryShapeClass::Unknown: return "Unknown";
  }
  return "Unknown";
}

static inline std::string historyRiskClassName(HistoryRiskClass c) {
  switch (c) {
  case HistoryRiskClass::Low: return "Low";
  case HistoryRiskClass::Moderate: return "Moderate";
  case HistoryRiskClass::ByteViewRead: return "ByteViewRead";
  case HistoryRiskClass::ByteViewWrite: return "ByteViewWrite";
  case HistoryRiskClass::BranchPolymorphicWrite: return "BranchPolymorphicWrite";
  case HistoryRiskClass::SuspiciousReinterpretation: return "SuspiciousReinterpretation";
  case HistoryRiskClass::DangerousReinterpretation: return "DangerousReinterpretation";
  case HistoryRiskClass::DangerousProvenance: return "DangerousProvenance";
  case HistoryRiskClass::FunctionParameterDependent: return "FunctionParameterDependent";
  case HistoryRiskClass::HigherOrderUnresolved: return "HigherOrderUnresolved";
  case HistoryRiskClass::ExternalSink: return "ExternalSink";
  case HistoryRiskClass::Unknown: return "Unknown";
  }
  return "Unknown";
}

static inline std::string historyClassName(HistoryClass c) {
  switch (c) {
  case HistoryClass::StableViaVoid: return "StableViaVoid";
  case HistoryClass::ByteView: return "ByteView";
  case HistoryClass::ParallelPolymorphic: return "ParallelPolymorphic";
  case HistoryClass::BranchPolymorphic: return "BranchPolymorphic";
  case HistoryClass::DangerousProvenance: return "DangerousProvenance";
  case HistoryClass::FunctionParameterDependent: return "FunctionParameterDependent";
  case HistoryClass::HigherOrderUnresolved: return "HigherOrderUnresolved";
  case HistoryClass::ExternalSink: return "ExternalSink";
  case HistoryClass::Unknown: return "Unknown";
  }
  return "Unknown";
}

namespace detail_metrics {

static inline int countBranchAlternativesInRenderedBase(const std::string &s) {
  if (s.find("⟨[") == std::string::npos)
    return 0;

  int count = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] != '[')
      continue;

    // Count [guard] markers only when they occur inside the rendered branch
    // delimiters.  This avoids counting array type syntax such as int[10].
    size_t before = s.rfind("⟨", i);
    size_t after = s.find("⟩", i);
    if (before != std::string::npos && after != std::string::npos)
      ++count;
  }
  return count;
}

static inline bool renderedBaseLooksBranchy(const std::string &s) {
  // Only our explicit Join/branch rendering should count as branchy.
  // Array type syntax such as int[10] must not be treated as a branch.
  return s.find("⟨[") != std::string::npos;
}

static inline bool startsWithAt(const std::string &s, size_t pos,
                                const std::string &pat) {
  return pos + pat.size() <= s.size() && s.compare(pos, pat.size(), pat) == 0;
}


static inline std::string trim(std::string s) {
  auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
  s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
  return s;
}

static inline std::vector<std::string> splitTypeChain(const std::string &text) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start < text.size()) {
    size_t pos = text.find("=>", start);
    std::string part = pos == std::string::npos
                           ? text.substr(start)
                           : text.substr(start, pos - start);
    part = trim(part);
    if (!part.empty())
      out.push_back(part);
    if (pos == std::string::npos)
      break;
    start = pos + 2;
  }
  return out;
}

static inline std::vector<std::string>
splitRenderedBranchAlternatives(const std::string &s) {
  std::vector<std::string> out;
  const std::string open = "⟨";
  const std::string close = "⟩";

  size_t begin = s.find(open);
  size_t end = s.rfind(close);
  if (begin == std::string::npos || end == std::string::npos ||
      begin + open.size() > end) {
    out.push_back(s);
    return out;
  }

  std::string body = s.substr(begin + open.size(),
                              end - (begin + open.size()));

  int angleDepth = 0;
  int squareDepth = 0;
  size_t start = 0;

  for (size_t i = 0; i < body.size();) {
    if (startsWithAt(body, i, open)) {
      ++angleDepth;
      i += open.size();
      continue;
    }
    if (startsWithAt(body, i, close)) {
      if (angleDepth > 0)
        --angleDepth;
      i += close.size();
      continue;
    }

    char c = body[i];
    if (c == '[') {
      ++squareDepth;
    } else if (c == ']') {
      if (squareDepth > 0)
        --squareDepth;
    } else if (c == ',' && angleDepth == 0 && squareDepth == 0) {
      out.push_back(body.substr(start, i - start));
      start = i + 1;
    }
    ++i;
  }

  out.push_back(body.substr(start));
  return out;
}

static inline std::string stripRenderedBranchGuard(std::string alt) {
  alt = trim(std::move(alt));
  if (alt.empty() || alt.front() != '[')
    return alt;

  int depth = 0;
  for (size_t i = 0; i < alt.size(); ++i) {
    if (alt[i] == '[') {
      ++depth;
    } else if (alt[i] == ']') {
      if (depth > 0)
        --depth;
      if (depth == 0)
        return trim(alt.substr(i + 1));
    }
  }
  return alt;
}

static inline std::vector<std::vector<std::string>>
renderedBranchTypeChains(const std::string &s) {
  std::vector<std::vector<std::string>> chains;
  if (!renderedBaseLooksBranchy(s)) {
    chains.push_back(splitTypeChain(s));
    return chains;
  }

  const std::string open = "⟨";
  const std::string close = "⟩";

  size_t begin = s.find(open);
  size_t end = s.rfind(close);

  // Preserve visible sequential context around the rendered branch.  For a
  // printed history such as:
  //   ⟨[g] int * => void *, [!g] char * => void *⟩ => void *
  // each branch alternative should be interpreted as:
  //   int *  => void * => void *
  //   char * => void * => void *
  // rather than dropping the suffix after the branch close.
  std::vector<std::string> prefixParts;
  std::vector<std::string> suffixParts;
  if (begin != std::string::npos)
    prefixParts = splitTypeChain(s.substr(0, begin));
  if (end != std::string::npos && end + close.size() <= s.size())
    suffixParts = splitTypeChain(s.substr(end + close.size()));

  for (auto alt : splitRenderedBranchAlternatives(s)) {
    std::string hist = stripRenderedBranchGuard(std::move(alt));
    auto chain = splitTypeChain(hist);

    std::vector<std::string> full = prefixParts;
    full.insert(full.end(), chain.begin(), chain.end());
    full.insert(full.end(), suffixParts.begin(), suffixParts.end());

    if (!full.empty())
      chains.push_back(std::move(full));
  }

  if (chains.empty())
    chains.push_back({});
  return chains;
}

static inline int renderedBaseTypeChainDepth(const std::string &s) {
  int best = 0;
  for (const auto &chain : renderedBranchTypeChains(s))
    best = std::max<int>(best, chain.size());
  return best;
}

static inline bool containsWordLike(const std::string &s, const std::string &needle) {
  return s.find(needle) != std::string::npos;
}

static inline bool isPointerTypeText(const std::string &ty) {
  return ty.find('*') != std::string::npos;
}

static inline bool isVoidPointerTypeText(const std::string &ty) {
  return isPointerTypeText(ty) && containsWordLike(ty, "void");
}

static inline bool isArrayTypeText(const std::string &ty) {
  // C array types in Clang pretty-print like int[10], char[32], etc.
  // Do not confuse these brackets with our branch syntax ⟨[guard] ...⟩.
  return !isPointerTypeText(ty) &&
         ty.find('[') != std::string::npos &&
         ty.find(']') != std::string::npos &&
         ty.find("⟨[") == std::string::npos;
}

static inline bool isCharPointerTypeText(const std::string &ty) {
  if (!isPointerTypeText(ty))
    return false;
  return containsWordLike(ty, "char") ||
         containsWordLike(ty, "uint8_t") ||
         containsWordLike(ty, "int8_t") ||
         containsWordLike(ty, "byte");
}

static inline bool isLikelyIntegerTypeText(const std::string &ty) {
  if (isPointerTypeText(ty) || isArrayTypeText(ty))
    return false;
  return containsWordLike(ty, "int") ||
         containsWordLike(ty, "long") ||
         containsWordLike(ty, "short") ||
         containsWordLike(ty, "char") ||
         containsWordLike(ty, "size_t") ||
         containsWordLike(ty, "uintptr_t") ||
         containsWordLike(ty, "intptr_t") ||
         containsWordLike(ty, "uint") ||
         containsWordLike(ty, "signed") ||
         containsWordLike(ty, "unsigned");
}

static inline bool isDerefLike(PointerUseKind k) {
  return k == PointerUseKind::Dereference ||
         k == PointerUseKind::DereferenceWrite ||
         k == PointerUseKind::DereferenceReadWrite ||
         k == PointerUseKind::ArrayIndex ||
         k == PointerUseKind::ArrayIndexRead ||
         k == PointerUseKind::ArrayIndexWrite ||
         k == PointerUseKind::ArrayIndexReadWrite;
}

static inline bool isWriteLike(PointerUseKind k) {
  return k == PointerUseKind::DereferenceWrite ||
         k == PointerUseKind::DereferenceReadWrite ||
         k == PointerUseKind::ArrayIndexWrite ||
         k == PointerUseKind::ArrayIndexReadWrite;
}

static inline bool isIndirectCallUse(const PointerUseRecord &u) {
  return u.kind == PointerUseKind::CallArgument &&
         u.detail.find("<indirect>") != std::string::npos;
}

static inline bool isExternalLikeUse(const PointerUseRecord &u) {
  // Memory/string library calls are sinks, but not necessarily unresolved
  // external sinks.  We keep hasExternalSink for uses whose target/effect is
  // genuinely unknown to this analysis.  Do not key off the substring
  // "external": resolved/imported functions are often named external_handle,
  // external_process, etc.  Once summary import or project-SCC propagation has
  // materialized the callee's pointer operations, those records should be
  // classified from the imported operations, not from the callee name.
  if (u.kind == PointerUseKind::CallArgument) {
    return u.detail.find("unknown") != std::string::npos ||
           u.detail.find("<unknown>") != std::string::npos ||
           u.detail.find("unresolved") != std::string::npos ||
           u.detail.find("<unresolved>") != std::string::npos;
  }
  return false;
}


static inline std::string normalizeTypeTextForMetricKey(std::string ty) {
  ty = trim(std::move(ty));
  while (ty.find("  ") != std::string::npos)
    ty.replace(ty.find("  "), 2, " ");
  // Normalize the common spacing around pointer stars.
  while (ty.find(" *") != std::string::npos)
    ty.replace(ty.find(" *"), 2, "*");
  while (ty.find("* ") != std::string::npos)
    ty.replace(ty.find("* "), 2, "*");
  return ty;
}

static inline std::string stripLeadingQualifiersForMetricKey(std::string ty) {
  ty = trim(std::move(ty));
  bool changed = true;
  while (changed) {
    changed = false;
    const char *quals[] = {"const ", "volatile ", "restrict "};
    for (const char *q : quals) {
      std::string qs(q);
      if (ty.rfind(qs, 0) == 0) {
        ty = trim(ty.substr(qs.size()));
        changed = true;
      }
    }
  }
  return ty;
}

static inline std::string pointerPointeeTextForMetricKey(std::string ty) {
  ty = normalizeTypeTextForMetricKey(std::move(ty));
  while (!ty.empty() && ty.back() == '*') {
    ty.pop_back();
    ty = trim(std::move(ty));
  }
  return stripLeadingQualifiersForMetricKey(ty);
}

static inline std::string textTypeMetricKey(const std::string &rawTy) {
  std::string ty = trim(rawTy);
  if (ty.empty())
    return "text:<empty>";

  if (isVoidPointerTypeText(ty))
    return "ptr:void";

  if (isCharPointerTypeText(ty))
    return "ptr:byte";

  if (isPointerTypeText(ty)) {
    std::string pointee = pointerPointeeTextForMetricKey(ty);
    std::string normalized = normalizeTypeTextForMetricKey(pointee);
    if (normalized.rfind("struct ", 0) == 0)
      return "ptr:record:struct:" + trim(normalized.substr(7));
    if (normalized.rfind("union ", 0) == 0)
      return "ptr:record:union:" + trim(normalized.substr(6));
    return "ptr:" + normalized;
  }

  if (isArrayTypeText(ty))
    return "array:" + normalizeTypeTextForMetricKey(ty);

  if (isLikelyIntegerTypeText(ty))
    return "int:" + stripLeadingQualifiersForMetricKey(ty);

  return "text:" + normalizeTypeTextForMetricKey(ty);
}

static inline TypeAtom typeAtomFromTextFallback(const std::string &rawTy) {
  TypeAtom atom;
  atom.text = trim(rawTy);
  atom.canonicalText = atom.text;

  if (atom.text.empty()) {
    atom.kind = TypeKind::Unknown;
    return atom;
  }

  if (atom.text == "void") {
    atom.kind = TypeKind::Void;
    atom.isVoid = true;
    return atom;
  }

  if (isVoidPointerTypeText(atom.text)) {
    atom.kind = TypeKind::VoidPointer;
    atom.isPointer = true;
    atom.isVoidPointer = true;
    atom.pointeeText = "void";
    atom.pointeeCanonicalText = "void";
    return atom;
  }

  if (isCharPointerTypeText(atom.text)) {
    atom.kind = TypeKind::CharPointer;
    atom.isPointer = true;
    atom.isCharPointer = true;
    atom.pointeeText = pointerPointeeTextForMetricKey(atom.text);
    atom.pointeeCanonicalText = atom.pointeeText;
    return atom;
  }

  if (isPointerTypeText(atom.text)) {
    atom.isPointer = true;
    std::string pointee = pointerPointeeTextForMetricKey(atom.text);
    atom.pointeeText = pointee;
    atom.pointeeCanonicalText = pointee;
    if (pointee.rfind("struct ", 0) == 0) {
      atom.kind = TypeKind::RecordPointer;
      atom.isRecordPointer = true;
      atom.recordTag = "struct";
      atom.recordName = trim(pointee.substr(7));
    } else if (pointee.rfind("union ", 0) == 0) {
      atom.kind = TypeKind::RecordPointer;
      atom.isRecordPointer = true;
      atom.recordTag = "union";
      atom.recordName = trim(pointee.substr(6));
    } else {
      atom.kind = TypeKind::OtherPointer;
    }
    return atom;
  }

  if (isArrayTypeText(atom.text)) {
    atom.kind = TypeKind::Array;
    atom.isArray = true;
    return atom;
  }

  if (isLikelyIntegerTypeText(atom.text)) {
    atom.kind = TypeKind::Integer;
    atom.isInteger = true;
    return atom;
  }

  atom.kind = TypeKind::Other;
  return atom;
}

static inline std::string typeAtomDisplayLabel(const TypeAtom &atom) {
  std::string s = trim(atom.displayText());
  return s.empty() ? "<unknown-type>" : s;
}

static inline std::string typeAtomMetricKey(const TypeAtom &atom) {
  if (!atom.hasStructuredInfo())
    return textTypeMetricKey(atom.text);
  std::string s = trim(atom.compatibilityKey());
  return s.empty() ? textTypeMetricKey(typeAtomDisplayLabel(atom)) : s;
}

static inline void addType(HistoryMetrics &m, const std::string &rawTy) {
  std::string ty = trim(rawTy);
  if (ty.empty())
    return;

  std::string key = textTypeMetricKey(ty);
  ++m.numLegacyTextTypes;

  m.allTypes.insert(ty);
  m.allTypeKeys.insert(key);

  if (isVoidPointerTypeText(ty))
    m.hasVoid = true;
  if (isCharPointerTypeText(ty))
    m.hasCharPtr = true;

  if (isPointerTypeText(ty)) {
    m.pointerTypes.insert(ty);
    m.pointerTypeKeys.insert(key);
    if (!isVoidPointerTypeText(ty)) {
      m.nonVoidPointerTypes.insert(ty);
      m.nonVoidPointerTypeKeys.insert(key);
    }
  }
}

static inline void addType(HistoryMetrics &m, const TypeAtom &atom) {
  if (!atom.hasStructuredInfo()) {
    addType(m, atom.text);
    return;
  }

  ++m.numStructuredTypeAtoms;

  std::string label = typeAtomDisplayLabel(atom);
  std::string key = typeAtomMetricKey(atom);

  m.allTypes.insert(label);
  m.allTypeKeys.insert(key);

  if (atom.isVoidPointer)
    m.hasVoid = true;
  if (atom.isCharPointer)
    m.hasCharPtr = true;

  if (atom.isPointer) {
    m.pointerTypes.insert(label);
    m.pointerTypeKeys.insert(key);
    if (!atom.isVoidPointer) {
      m.nonVoidPointerTypes.insert(label);
      m.nonVoidPointerTypeKeys.insert(key);
    }
  }
}

static inline int typeChainDepth(const std::string &text) {
  int n = 0;
  for (const auto &ty : splitTypeChain(text)) {
    if (!ty.empty())
      ++n;
  }
  return n;
}

static inline void addTypeChain(HistoryMetrics &m, const std::string &text) {
  auto parts = splitTypeChain(text);
  for (const auto &ty : parts)
    addType(m, ty);

  for (size_t i = 1; i < parts.size(); ++i) {
    const std::string &prev = parts[i - 1];
    const std::string &cur = parts[i];
    if (!isArrayTypeText(prev) && isLikelyIntegerTypeText(prev) && isPointerTypeText(cur))
      m.hasIntToPtr = true;
    if (isPointerTypeText(prev) && !isArrayTypeText(cur) && isLikelyIntegerTypeText(cur))
      m.hasPtrToInt = true;
  }
}

static inline void addRenderedBaseTypeChains(HistoryMetrics &m,
                                             const std::string &text) {
  for (const auto &parts : renderedBranchTypeChains(text)) {
    for (const auto &ty : parts)
      addType(m, ty);

    for (size_t i = 1; i < parts.size(); ++i) {
      const std::string &prev = parts[i - 1];
      const std::string &cur = parts[i];
      if (!isArrayTypeText(prev) && isLikelyIntegerTypeText(prev) && isPointerTypeText(cur))
        m.hasIntToPtr = true;
      if (isPointerTypeText(prev) && !isArrayTypeText(cur) && isLikelyIntegerTypeText(cur))
        m.hasPtrToInt = true;
    }
  }
}


static inline void addIntegerPointerTransitions(HistoryMetrics &m,
                                                const std::vector<std::string> &parts) {
  for (size_t i = 1; i < parts.size(); ++i) {
    const std::string prev = trim(parts[i - 1]);
    const std::string cur = trim(parts[i]);
    if (!isArrayTypeText(prev) && isLikelyIntegerTypeText(prev) && isPointerTypeText(cur))
      m.hasIntToPtr = true;
    if (isPointerTypeText(prev) && !isArrayTypeText(cur) && isLikelyIntegerTypeText(cur))
      m.hasPtrToInt = true;
  }
}

static inline std::string typeSequencePart(const TypeAtom &atom) {
  return trim(atom.text);
}

static inline bool isIntegerPointerTransition(const TypeAtom &prev,
                                              const TypeAtom &cur) {
  if (prev.hasStructuredInfo() && cur.hasStructuredInfo())
    return prev.isInteger && !prev.isArray && cur.isPointer;
  return !isArrayTypeText(prev.text) && isLikelyIntegerTypeText(prev.text) &&
         isPointerTypeText(cur.text);
}

static inline bool isPointerIntegerTransition(const TypeAtom &prev,
                                              const TypeAtom &cur) {
  if (prev.hasStructuredInfo() && cur.hasStructuredInfo())
    return prev.isPointer && cur.isInteger && !cur.isArray;
  return isPointerTypeText(prev.text) && !isArrayTypeText(cur.text) &&
         isLikelyIntegerTypeText(cur.text);
}

static inline std::vector<std::vector<std::string>> typeSequences(const ObsHistPtr &h) {
  if (!h)
    return {{}};

  switch (h->kind) {
  case ObsKind::AsType:
    if (h->ty.hasStructuredInfo())
      return {std::vector<std::string>{h->ty.text}};
    return {splitTypeChain(h->ty.text)};

  case ObsKind::PtrArith:
    return {std::vector<std::string>{"ptrarith(" + h->ty.text + ")"}};

  case ObsKind::Seq: {
    auto left = typeSequences(h->first);
    auto right = typeSequences(h->second);
    std::vector<std::vector<std::string>> out;
    for (auto l : left) {
      for (const auto &r : right) {
        auto combined = l;
        combined.insert(combined.end(), r.begin(), r.end());
        out.push_back(std::move(combined));
      }
    }
    return out;
  }

  case ObsKind::Parallel: {
    std::vector<std::vector<std::string>> out;
    for (const auto &arm : h->arms) {
      auto seqs = typeSequences(arm);
      out.insert(out.end(), seqs.begin(), seqs.end());
    }
    return out.empty() ? std::vector<std::vector<std::string>>{{}} : out;
  }

  case ObsKind::Join: {
    std::vector<std::vector<std::string>> out;
    for (const auto &alt : h->alts) {
      auto seqs = typeSequences(alt.hist);
      out.insert(out.end(), seqs.begin(), seqs.end());
    }
    return out.empty() ? std::vector<std::vector<std::string>>{{}} : out;
  }

  case ObsKind::CallWrap:
    return typeSequences(h->inner);

  case ObsKind::FieldRef:
    return typeSequences(h->base);

  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::HOFParamRef:
  case ObsKind::Unknown:
    return {{}};
  }
  return {{}};
}

static inline void addCrossBoundaryTransitions(HistoryMetrics &m,
                                               const std::optional<std::string> &baseText,
                                               const ObsHistPtr &h) {
  std::vector<std::string> baseParts;
  if (baseText)
    baseParts = splitTypeChain(*baseText);

  for (auto seq : typeSequences(h)) {
    std::vector<std::string> full = baseParts;
    full.insert(full.end(), seq.begin(), seq.end());
    addIntegerPointerTransitions(m, full);
  }
}

static inline std::vector<TypeAtom> typeAtomsFromTextChain(const std::string &text) {
  std::vector<TypeAtom> out;
  for (const auto &part : splitTypeChain(text))
    out.push_back(typeAtomFromTextFallback(part));
  return out;
}


struct TypeAtomStep {
  TypeAtom atom;
  bool isPtrArith = false;
};

static inline bool typeAtomIsPointer(const TypeAtom &a) {
  return a.hasStructuredInfo() ? a.isPointer : isPointerTypeText(a.text);
}

static inline bool typeAtomIsVoidPointer(const TypeAtom &a) {
  return a.hasStructuredInfo() ? a.isVoidPointer : isVoidPointerTypeText(a.text);
}

static inline bool typeAtomIsCharPointer(const TypeAtom &a) {
  return a.hasStructuredInfo() ? a.isCharPointer : isCharPointerTypeText(a.text);
}

static inline bool typeAtomIsConcreteNonBytePointer(const TypeAtom &a) {
  return typeAtomIsPointer(a) &&
         !typeAtomIsVoidPointer(a) &&
         !typeAtomIsCharPointer(a) &&
         !(a.hasStructuredInfo() && a.isFunctionPointer);
}

static inline std::optional<std::string>
concreteNonBytePointerAtomKey(const TypeAtom &a) {
  if (!typeAtomIsConcreteNonBytePointer(a))
    return std::nullopt;
  return typeAtomMetricKey(a);
}

static inline bool historyHasStructuredTypeAtoms(const ObsHistPtr &h) {
  if (!h)
    return false;

  switch (h->kind) {
  case ObsKind::AsType:
  case ObsKind::PtrArith:
    return h->ty.hasStructuredInfo();

  case ObsKind::Seq:
    return historyHasStructuredTypeAtoms(h->first) ||
           historyHasStructuredTypeAtoms(h->second);

  case ObsKind::Parallel:
    for (const auto &arm : h->arms)
      if (historyHasStructuredTypeAtoms(arm))
        return true;
    return false;

  case ObsKind::Join:
    for (const auto &alt : h->alts)
      if (historyHasStructuredTypeAtoms(alt.hist))
        return true;
    return false;

  case ObsKind::CallWrap:
    return historyHasStructuredTypeAtoms(h->inner);

  case ObsKind::FieldRef:
    return historyHasStructuredTypeAtoms(h->base);

  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::HOFParamRef:
  case ObsKind::Unknown:
    return false;
  }
  return false;
}

static inline std::vector<std::vector<TypeAtomStep>>
typeAtomStepSequences(const ObsHistPtr &h) {
  if (!h)
    return {{}};

  switch (h->kind) {
  case ObsKind::AsType:
    if (h->ty.hasStructuredInfo())
      return {std::vector<TypeAtomStep>{{h->ty, false}}};
    {
      std::vector<TypeAtomStep> out;
      for (const auto &a : typeAtomsFromTextChain(h->ty.text))
        out.push_back(TypeAtomStep{a, false});
      return {out};
    }

  case ObsKind::PtrArith:
    return {std::vector<TypeAtomStep>{{h->ty, true}}};

  case ObsKind::Seq: {
    auto left = typeAtomStepSequences(h->first);
    auto right = typeAtomStepSequences(h->second);
    std::vector<std::vector<TypeAtomStep>> out;
    for (auto l : left) {
      for (const auto &r : right) {
        auto combined = l;
        combined.insert(combined.end(), r.begin(), r.end());
        out.push_back(std::move(combined));
      }
    }
    return out;
  }

  case ObsKind::Parallel: {
    std::vector<std::vector<TypeAtomStep>> out;
    for (const auto &arm : h->arms) {
      auto seqs = typeAtomStepSequences(arm);
      out.insert(out.end(), seqs.begin(), seqs.end());
    }
    return out.empty() ? std::vector<std::vector<TypeAtomStep>>{{}} : out;
  }

  case ObsKind::Join: {
    std::vector<std::vector<TypeAtomStep>> out;
    for (const auto &alt : h->alts) {
      auto seqs = typeAtomStepSequences(alt.hist);
      out.insert(out.end(), seqs.begin(), seqs.end());
    }
    return out.empty() ? std::vector<std::vector<TypeAtomStep>>{{}} : out;
  }

  case ObsKind::CallWrap:
    return typeAtomStepSequences(h->inner);

  case ObsKind::FieldRef:
    return typeAtomStepSequences(h->base);

  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::HOFParamRef:
  case ObsKind::Unknown:
    return {{}};
  }
  return {{}};
}

static inline std::vector<std::vector<TypeAtomStep>>
renderedBranchBaseStepSequences(const std::string &text) {
  std::vector<std::vector<TypeAtomStep>> out;
  for (const auto &chain : renderedBranchTypeChains(text)) {
    std::vector<TypeAtomStep> seq;
    for (const auto &part : chain)
      seq.push_back(TypeAtomStep{typeAtomFromTextFallback(part), false});
    out.push_back(std::move(seq));
  }

  if (out.empty())
    out.push_back({});
  return out;
}


static inline std::vector<std::vector<TypeAtom>> typeAtomSequences(const ObsHistPtr &h) {
  if (!h)
    return {{}};

  switch (h->kind) {
  case ObsKind::AsType:
    if (h->ty.hasStructuredInfo())
      return {std::vector<TypeAtom>{h->ty}};
    return {typeAtomsFromTextChain(h->ty.text)};

  case ObsKind::PtrArith:
    // PtrArith is a type-bearing step. Keep the structured pointer TypeAtom
    // rather than re-parsing "ptrarith(T *)" later.
    return {std::vector<TypeAtom>{h->ty}};

  case ObsKind::Seq: {
    auto left = typeAtomSequences(h->first);
    auto right = typeAtomSequences(h->second);
    std::vector<std::vector<TypeAtom>> out;
    for (auto l : left) {
      for (const auto &r : right) {
        auto combined = l;
        combined.insert(combined.end(), r.begin(), r.end());
        out.push_back(std::move(combined));
      }
    }
    return out;
  }

  case ObsKind::Parallel: {
    std::vector<std::vector<TypeAtom>> out;
    for (const auto &arm : h->arms) {
      auto seqs = typeAtomSequences(arm);
      out.insert(out.end(), seqs.begin(), seqs.end());
    }
    return out.empty() ? std::vector<std::vector<TypeAtom>>{{}} : out;
  }

  case ObsKind::Join: {
    std::vector<std::vector<TypeAtom>> out;
    for (const auto &alt : h->alts) {
      auto seqs = typeAtomSequences(alt.hist);
      out.insert(out.end(), seqs.begin(), seqs.end());
    }
    return out.empty() ? std::vector<std::vector<TypeAtom>>{{}} : out;
  }

  case ObsKind::CallWrap:
    return typeAtomSequences(h->inner);

  case ObsKind::FieldRef:
    return typeAtomSequences(h->base);

  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::HOFParamRef:
  case ObsKind::Unknown:
    return {{}};
  }
  return {{}};
}

static inline void addIntegerPointerTransitions(HistoryMetrics &m,
                                                const std::vector<TypeAtom> &parts) {
  for (size_t i = 1; i < parts.size(); ++i) {
    if (isIntegerPointerTransition(parts[i - 1], parts[i]))
      m.hasIntToPtr = true;
    if (isPointerIntegerTransition(parts[i - 1], parts[i]))
      m.hasPtrToInt = true;
  }
}

static inline void addIntegerPointerTransitionsStructured(
    HistoryMetrics &m,
    const std::vector<TypeAtomStep> &parts);

static inline void addCrossBoundaryTransitionsStructured(
    HistoryMetrics &m,
    const std::optional<TypeAtom> &baseAtom,
    const std::optional<std::string> &baseText,
    const ObsHistPtr &h) {
  std::vector<std::vector<TypeAtomStep>> baseSeqs;

  if (baseAtom) {
    baseSeqs.push_back(std::vector<TypeAtomStep>{TypeAtomStep{*baseAtom, false}});
  } else if (baseText) {
    baseSeqs = renderedBranchBaseStepSequences(*baseText);
  } else {
    baseSeqs.push_back({});
  }

  for (const auto &baseSeq : baseSeqs) {
    for (auto seq : typeAtomStepSequences(h)) {
      std::vector<TypeAtomStep> full = baseSeq;
      full.insert(full.end(), seq.begin(), seq.end());
      addIntegerPointerTransitionsStructured(m, full);
    }
  }
}

static inline int computeSeqDepth(const ObsHistPtr &h) {
  if (!h)
    return 0;
  switch (h->kind) {
  case ObsKind::AsType:
  case ObsKind::PtrArith:
    return 1;
  case ObsKind::Seq:
    return computeSeqDepth(h->first) + computeSeqDepth(h->second);
  case ObsKind::Parallel: {
    int best = 0;
    for (const auto &arm : h->arms)
      best = std::max(best, computeSeqDepth(arm));
    return best;
  }
  case ObsKind::Join: {
    int best = 0;
    for (const auto &alt : h->alts)
      best = std::max(best, computeSeqDepth(alt.hist));
    return best;
  }
  case ObsKind::CallWrap:
    return computeSeqDepth(h->inner);
  case ObsKind::FieldRef:
    return computeSeqDepth(h->base);
  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::HOFParamRef:
  case ObsKind::Unknown:
    return 0;
  }
  return 0;
}

static inline void collect(const ObsHistPtr &h, HistoryMetrics &m) {
  if (!h)
    return;

  switch (h->kind) {
  case ObsKind::AsType:
    if (h->ty.hasStructuredInfo())
      addType(m, h->ty);
    else
      addTypeChain(m, h->ty.text);
    return;

  case ObsKind::PtrArith:
    ++m.numPtrArithNodes;
    m.hasPointerArithmetic = true;
    m.ptrArithTypes.insert(h->ty.text);
    m.ptrArithTypeKeys.insert(typeAtomMetricKey(h->ty));
    addType(m, h->ty);
    return;

  case ObsKind::Seq:
    ++m.numSeqNodes;
    collect(h->first, m);
    collect(h->second, m);
    return;

  case ObsKind::Parallel:
    ++m.numParallelNodes;
    m.maxParallelBreadth = std::max<int>(m.maxParallelBreadth, h->arms.size());
    for (const auto &arm : h->arms)
      collect(arm, m);
    return;

  case ObsKind::Join:
    ++m.numBranchNodes;
    m.maxBranchBreadth = std::max<int>(m.maxBranchBreadth, h->alts.size());
    for (const auto &alt : h->alts)
      collect(alt.hist, m);
    return;

  case ObsKind::CallWrap:
    ++m.numCallWraps;
    collect(h->inner, m);
    return;

  case ObsKind::HOFParamRef:
    ++m.numHORefs;
    return;

  case ObsKind::FieldRef:
    collect(h->base, m);
    return;

  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::Unknown:
    return;
  }
}

static inline void addPointerUseSinkFlags(HistoryMetrics &m,
                                             const std::vector<PointerUseRecord> *uses) {
  if (!uses)
    return;
  for (const auto &u : *uses) {
    if (isIndirectCallUse(u))
      m.hasIndirectCallUse = true;
    if (isExternalLikeUse(u))
      m.hasExternalSink = true;
  }
}

static inline std::optional<std::string>
firstNonVoidPointerType(const std::optional<std::string> &baseText,
                        const HistoryMetrics &m) {
  if (baseText) {
    for (const auto &part : splitTypeChain(*baseText)) {
      if (isPointerTypeText(part) && !isVoidPointerTypeText(part))
        return part;
    }
  }
  if (m.nonVoidPointerTypes.size() == 1)
    return *m.nonVoidPointerTypes.begin();
  return std::nullopt;
}

static inline std::string typePartCore(const std::string &part) {
  std::string p = trim(part);
  const std::string prefix = "ptrarith(";
  if (p.rfind(prefix, 0) == 0 && !p.empty() && p.back() == ')')
    return trim(p.substr(prefix.size(), p.size() - prefix.size() - 1));
  return p;
}

static inline bool isPtrArithPart(const std::string &part) {
  return trim(part).rfind("ptrarith(", 0) == 0;
}

static inline bool isCharPointerPart(const std::string &part) {
  return isCharPointerTypeText(typePartCore(part));
}

static inline bool isVoidPointerPart(const std::string &part) {
  return isVoidPointerTypeText(typePartCore(part));
}

static inline bool isConcreteNonBytePointerPart(const std::string &part) {
  std::string ty = typePartCore(part);
  return isPointerTypeText(ty) &&
         !isVoidPointerTypeText(ty) &&
         !isCharPointerTypeText(ty);
}

static inline std::optional<std::string>
concreteNonBytePointerPart(const std::string &part) {
  if (!isConcreteNonBytePointerPart(part))
    return std::nullopt;
  return typePartCore(part);
}

static inline void analyzeByteDirectionSequence(HistoryMetrics &m,
                                                const std::vector<std::string> &seq) {
  // Detect directional byte-view patterns.
  //
  // We distinguish:
  //   char* => T*
  //       ByteViewToConcreteNoDeref
  //
  //   T1* => char* => T2*
  //       ByteMediatedReinterpretationNoDeref  (when T1 != T2)
  //
  //   char* => T1* => T2*
  //   T1*   => T2*
  //       SequentialConcreteReinterpretationNoDeref
  //
  //   T* => char* => ptrarith(char*) => T*
  //       ByteOffsetRoundTripNoDeref
  //
  //   T1* => char* => ptrarith(char*) => T2*
  //       ByteOffsetToDifferentConcreteNoDeref
  std::vector<std::pair<size_t, std::string>> concretePositions;
  std::vector<size_t> charPositions;

  for (size_t i = 0; i < seq.size(); ++i) {
    if (isCharPointerPart(seq[i]))
      charPositions.push_back(i);
    if (auto ty = concreteNonBytePointerPart(seq[i]))
      concretePositions.push_back({i, *ty});
  }

  // Any sequential transition from one concrete non-byte pointer type to a
  // different concrete non-byte pointer type indicates sequential concrete
  // reinterpretation, unless the transition is specifically mediated by a char*
  // between them, which is handled below as byte-mediated.
  for (size_t i = 0; i + 1 < concretePositions.size(); ++i) {
    size_t aPos = concretePositions[i].first;
    size_t bPos = concretePositions[i + 1].first;
    const std::string &aTy = concretePositions[i].second;
    const std::string &bTy = concretePositions[i + 1].second;

    bool charBetween = false;
    for (size_t c : charPositions) {
      if (aPos < c && c < bPos) {
        charBetween = true;
        break;
      }
    }

    if (trim(aTy) != trim(bTy) && !charBetween)
      m.hasSequentialConcreteReinterpretation = true;
  }

  for (size_t c : charPositions) {
    std::optional<std::string> concreteBefore;
    for (size_t i = 0; i < c; ++i) {
      if (auto ty = concreteNonBytePointerPart(seq[i]))
        concreteBefore = *ty;
    }

    bool sawPtrArithAfterChar = false;
    for (size_t j = c + 1; j < seq.size(); ++j) {
      if (isPtrArithPart(seq[j])) {
        sawPtrArithAfterChar = true;
        continue;
      }

      auto concreteAfter = concreteNonBytePointerPart(seq[j]);
      if (!concreteAfter)
        continue;

      if (sawPtrArithAfterChar) {
        if (concreteBefore && trim(*concreteBefore) == trim(*concreteAfter))
          m.hasByteOffsetRoundTrip = true;
        else
          m.hasByteOffsetToDifferentConcrete = true;
      } else if (concreteBefore) {
        if (trim(*concreteBefore) != trim(*concreteAfter))
          m.hasByteMediatedReinterpretation = true;
        // If same type with no ptrarith, this is a byte-mediated same-type
        // round trip. Keep it as ByteViewToConcrete/ByteViewNoDeref rather
        // than creating another class for now.
      } else {
        m.hasByteViewToConcrete = true;
      }
    }
  }
}

static inline bool parallelHasSequentialArm(const ObsHistPtr &h) {
  if (!h)
    return false;

  switch (h->kind) {
  case ObsKind::Parallel:
    for (const auto &arm : h->arms) {
      if (computeSeqDepth(arm) > 1)
        return true;
      if (parallelHasSequentialArm(arm))
        return true;
    }
    return false;

  case ObsKind::Seq:
    return parallelHasSequentialArm(h->first) ||
           parallelHasSequentialArm(h->second);

  case ObsKind::Join:
    for (const auto &alt : h->alts)
      if (parallelHasSequentialArm(alt.hist))
        return true;
    return false;

  case ObsKind::CallWrap:
    return parallelHasSequentialArm(h->inner);

  case ObsKind::FieldRef:
    return parallelHasSequentialArm(h->base);

  case ObsKind::AsType:
  case ObsKind::PtrArith:
  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::HOFParamRef:
  case ObsKind::Unknown:
    return false;
  }
  return false;
}

static inline void analyzeVoidDirectionSequence(HistoryMetrics &m,
                                               const std::vector<std::string> &seq) {
  for (size_t i = 1; i < seq.size(); ++i) {
    const std::string prev = typePartCore(seq[i - 1]);
    const std::string cur = typePartCore(seq[i]);

    if (isConcreteNonBytePointerPart(prev) && isVoidPointerTypeText(cur))
      m.hasTypeErasureToVoid = true;

    if (isVoidPointerTypeText(prev) && isConcreteNonBytePointerPart(cur))
      m.hasConcreteSpecializationFromVoid = true;
  }
}

static inline void analyzeVoidDirectionAtomSequence(
    HistoryMetrics &m,
    const std::vector<TypeAtomStep> &seq) {
  for (size_t i = 1; i < seq.size(); ++i) {
    const TypeAtom &prev = seq[i - 1].atom;
    const TypeAtom &cur = seq[i].atom;

    if (typeAtomIsConcreteNonBytePointer(prev) && typeAtomIsVoidPointer(cur))
      m.hasTypeErasureToVoid = true;

    if (typeAtomIsVoidPointer(prev) && typeAtomIsConcreteNonBytePointer(cur))
      m.hasConcreteSpecializationFromVoid = true;
  }
}

static inline void analyzeByteDirectionAtomSequence(
    HistoryMetrics &m,
    const std::vector<TypeAtomStep> &seq) {
  std::vector<std::pair<size_t, std::string>> concretePositions;
  std::vector<size_t> charPositions;

  for (size_t i = 0; i < seq.size(); ++i) {
    if (typeAtomIsCharPointer(seq[i].atom))
      charPositions.push_back(i);
    if (auto key = concreteNonBytePointerAtomKey(seq[i].atom))
      concretePositions.push_back({i, *key});
  }

  for (size_t i = 0; i + 1 < concretePositions.size(); ++i) {
    size_t aPos = concretePositions[i].first;
    size_t bPos = concretePositions[i + 1].first;
    const std::string &aTy = concretePositions[i].second;
    const std::string &bTy = concretePositions[i + 1].second;

    bool charBetween = false;
    for (size_t c : charPositions) {
      if (aPos < c && c < bPos) {
        charBetween = true;
        break;
      }
    }

    if (aTy != bTy && !charBetween)
      m.hasSequentialConcreteReinterpretation = true;
  }

  for (size_t c : charPositions) {
    std::optional<std::string> concreteBefore;
    for (size_t i = 0; i < c; ++i) {
      if (auto key = concreteNonBytePointerAtomKey(seq[i].atom))
        concreteBefore = *key;
    }

    bool sawPtrArithAfterChar = false;
    for (size_t j = c + 1; j < seq.size(); ++j) {
      if (seq[j].isPtrArith) {
        sawPtrArithAfterChar = true;
        continue;
      }

      auto concreteAfter = concreteNonBytePointerAtomKey(seq[j].atom);
      if (!concreteAfter)
        continue;

      if (sawPtrArithAfterChar) {
        if (concreteBefore && *concreteBefore == *concreteAfter)
          m.hasByteOffsetRoundTrip = true;
        else
          m.hasByteOffsetToDifferentConcrete = true;
      } else if (concreteBefore) {
        if (*concreteBefore != *concreteAfter)
          m.hasByteMediatedReinterpretation = true;
      } else {
        m.hasByteViewToConcrete = true;
      }
    }
  }
}

static inline void addIntegerPointerTransitionsStructured(
    HistoryMetrics &m,
    const std::vector<TypeAtomStep> &parts) {
  for (size_t i = 1; i < parts.size(); ++i) {
    if (isIntegerPointerTransition(parts[i - 1].atom, parts[i].atom))
      m.hasIntToPtr = true;
    if (isPointerIntegerTransition(parts[i - 1].atom, parts[i].atom))
      m.hasPtrToInt = true;
  }
}

static inline void analyzeDirectionalBehaviorPatterns(
    HistoryMetrics &m,
    const std::optional<TypeAtom> &baseAtom,
    const std::optional<std::string> &baseText,
    const ObsHistPtr &h) {
  bool useStructured = (baseAtom && baseAtom->hasStructuredInfo()) ||
                       historyHasStructuredTypeAtoms(h) ||
                       (baseText && renderedBaseLooksBranchy(*baseText));

  if (useStructured) {
    std::vector<std::vector<TypeAtomStep>> baseSeqs;
    if (baseAtom)
      baseSeqs.push_back(std::vector<TypeAtomStep>{TypeAtomStep{*baseAtom, false}});
    else if (baseText)
      baseSeqs = renderedBranchBaseStepSequences(*baseText);
    else
      baseSeqs.push_back({});

    for (const auto &baseSeq : baseSeqs) {
      for (auto seq : typeAtomStepSequences(h)) {
        std::vector<TypeAtomStep> full = baseSeq;
        full.insert(full.end(), seq.begin(), seq.end());
        analyzeVoidDirectionAtomSequence(m, full);
        analyzeByteDirectionAtomSequence(m, full);
        addIntegerPointerTransitionsStructured(m, full);
      }
    }
  } else {
    std::vector<std::vector<std::string>> baseSeqs;
    if (baseText)
      baseSeqs = renderedBranchTypeChains(*baseText);
    else
      baseSeqs.push_back({});

    for (const auto &baseSeq : baseSeqs) {
      for (auto seq : typeSequences(h)) {
        std::vector<std::string> full = baseSeq;
        full.insert(full.end(), seq.begin(), seq.end());
        analyzeVoidDirectionSequence(m, full);
        analyzeByteDirectionSequence(m, full);
        addIntegerPointerTransitions(m, full);
      }
    }
  }

  m.hasParallelSequentialArm = parallelHasSequentialArm(h);
}

static inline bool useIsDerefAs(const PointerUseRecord &u, const std::string &ty) {
  return isDerefLike(u.kind) && trim(u.pointerType) == trim(ty);
}

} // namespace detail_metrics

static inline HistoryMetrics
computeHistoryMetrics(const ObsHistPtr &h,
                      const std::optional<std::string> &baseText = std::nullopt,
                      const std::vector<PointerUseRecord> *uses = nullptr,
                      const std::optional<TypeAtom> &baseAtom = std::nullopt) {
  HistoryMetrics m;

  int baseDepth = 0;
  if (baseAtom) {
    detail_metrics::addType(m, *baseAtom);
    baseDepth = 1;
  } else if (baseText) {
    if (detail_metrics::renderedBaseLooksBranchy(*baseText)) {
      detail_metrics::addRenderedBaseTypeChains(m, *baseText);
      baseDepth = detail_metrics::renderedBaseTypeChainDepth(*baseText);

      // Storage-location histories may use a rendered branch-sensitive base such
      // as:
      //   ⟨[g] int * => void *, [!g] char * => void *⟩
      // Treat this as a branch node so shape/risk classification can recognize
      // storage-location branch polymorphism.
      int n = detail_metrics::countBranchAlternativesInRenderedBase(*baseText);
      m.numBranchNodes += 1;
      m.maxBranchBreadth = std::max(m.maxBranchBreadth, std::max(1, n));
    } else {
      detail_metrics::addTypeChain(m, *baseText);
      baseDepth = detail_metrics::typeChainDepth(*baseText);
    }
  }

  detail_metrics::collect(h, m);
  detail_metrics::addCrossBoundaryTransitionsStructured(m, baseAtom, baseText, h);
  detail_metrics::analyzeDirectionalBehaviorPatterns(m, baseAtom, baseText, h);
  detail_metrics::addPointerUseSinkFlags(m, uses);
  m.seqDepth = baseDepth + detail_metrics::computeSeqDepth(h);
  m.numDistinctTypes = static_cast<int>(m.allTypeKeys.size());
  m.numPointerTypes = static_cast<int>(m.pointerTypeKeys.size());
  m.numNonVoidPointerTypes = static_cast<int>(m.nonVoidPointerTypeKeys.size());
  return m;
}


enum class BranchCastClass {
  NoBranchCast,
  SameTypeBranch,
  GuardedTypeSplit,
  TagLikeTypeSplit,
  BranchSpecificWrite,
  BranchSpecificByteWrite,
  BranchSpecificConcreteWrite,
  IncompleteBranchInfo,
  Unknown
};

static inline std::string branchCastClassName(BranchCastClass c) {
  switch (c) {
  case BranchCastClass::NoBranchCast: return "NoBranchCast";
  case BranchCastClass::SameTypeBranch: return "SameTypeBranch";
  case BranchCastClass::GuardedTypeSplit: return "GuardedTypeSplit";
  case BranchCastClass::TagLikeTypeSplit: return "TagLikeTypeSplit";
  case BranchCastClass::BranchSpecificWrite: return "BranchSpecificWrite";
  case BranchCastClass::BranchSpecificByteWrite: return "BranchSpecificByteWrite";
  case BranchCastClass::BranchSpecificConcreteWrite: return "BranchSpecificConcreteWrite";
  case BranchCastClass::IncompleteBranchInfo: return "IncompleteBranchInfo";
  case BranchCastClass::Unknown: return "Unknown";
  }
  return "Unknown";
}

struct BranchCastMetrics {
  int numBranchNodes = 0;
  int totalAlternatives = 0;
  int maxBranchBreadth = 0;

  // Branch position metrics. Depth is measured as the number of sequential
  // observation steps before reaching the branch node. This helps distinguish:
  //   void* => ⟨[g] int*, [!g] char*⟩
  // from:
  //   void* => int* => ptrarith(int*) => ⟨[g] char*, [!g] double*⟩
  int earliestBranchDepth = -1;
  int latestBranchDepth = -1;
  int maxBranchDepth = 0;

  // Per-alternative path lengths inside branch nodes.
  int minAltSeqDepth = -1;
  int maxAltSeqDepth = 0;
  int totalAltSeqDepth = 0;

  int numDistinctBranchTypes = 0;
  int numDistinctBranchNonVoidTypes = 0;
  int maxTypesPerBranchNode = 0;

  bool hasDifferentTypesAcrossBranches = false;
  bool hasSameTypeAllBranches = false;
  bool hasVoidOnlyBranch = false;
  bool hasTagLikeGuard = false;
  bool hasNegatedPairGuards = false;
  bool hasMissingElseAlternative = false;

  bool hasBranchSpecificWrite = false;
  bool hasBranchSpecificByteWrite = false;
  bool hasBranchSpecificConcreteWrite = false;

  std::set<std::string> guards;
  std::set<std::string> branchTypes;
  std::set<std::string> branchNonVoidTypes;
  std::set<std::string> branchWriteTypes;
};

namespace detail_branch_metrics {

static inline std::string lowerCopy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

static inline bool guardLooksTagLike(const std::string &g) {
  std::string s = lowerCopy(g);
  return s.find(".tag") != std::string::npos ||
         s.find("->tag") != std::string::npos ||
         s.find("tag") != std::string::npos ||
         s.find("kind") != std::string::npos ||
         s.find("type") != std::string::npos ||
         s.find("opcode") != std::string::npos ||
         s.find("variant") != std::string::npos ||
         s.find("class") != std::string::npos;
}

static inline std::string normalizeGuardText(std::string g) {
  g = detail_metrics::trim(std::move(g));
  while (!g.empty() && g.front() == '(' && g.back() == ')') {
    g = detail_metrics::trim(g.substr(1, g.size() - 2));
  }
  return g;
}

static inline bool guardsLookNegatedPair(const std::set<std::string> &guards) {
  for (const auto &g : guards) {
    std::string n = "!(" + g + ")";
    if (guards.count(n))
      return true;
    std::string nn = normalizeGuardText(g);
    if (nn.rfind("!", 0) == 0) {
      std::string base = normalizeGuardText(nn.substr(1));
      if (guards.count(base))
        return true;
    }
  }
  return false;
}

static inline bool guardTextMatches(const std::string &useGuard,
                                    const std::set<std::string> &branchGuards) {
  if (branchGuards.empty())
    return false;
  std::string ug = normalizeGuardText(useGuard);
  for (const auto &bgRaw : branchGuards) {
    std::string bg = normalizeGuardText(bgRaw);
    if (ug == bg)
      return true;
    if (!ug.empty() && !bg.empty() &&
        (ug.find(bg) != std::string::npos || bg.find(ug) != std::string::npos))
      return true;
  }
  return false;
}

static inline void noteAltDepth(BranchCastMetrics &bm, int d) {
  if (bm.minAltSeqDepth < 0 || d < bm.minAltSeqDepth)
    bm.minAltSeqDepth = d;
  bm.maxAltSeqDepth = std::max(bm.maxAltSeqDepth, d);
  bm.totalAltSeqDepth += d;
}

static inline void noteBranchDepth(BranchCastMetrics &bm, int d) {
  if (bm.earliestBranchDepth < 0 || d < bm.earliestBranchDepth)
    bm.earliestBranchDepth = d;
  bm.latestBranchDepth = std::max(bm.latestBranchDepth, d);
  bm.maxBranchDepth = std::max(bm.maxBranchDepth, d);
}

static inline std::string typeFromSeqPart(const std::string &part) {
  std::string p = detail_metrics::trim(part);
  const std::string prefix = "ptrarith(";
  if (p.rfind(prefix, 0) == 0 && !p.empty() && p.back() == ')')
    return detail_metrics::trim(p.substr(prefix.size(),
                                         p.size() - prefix.size() - 1));
  return p;
}

static inline size_t commonPrefixLen(
    const std::vector<std::vector<std::string>> &seqs) {
  if (seqs.empty())
    return 0;
  size_t n = seqs.front().size();
  for (const auto &s : seqs)
    n = std::min(n, s.size());

  size_t k = 0;
  for (; k < n; ++k) {
    const std::string &p = seqs.front()[k];
    bool allSame = true;
    for (size_t i = 1; i < seqs.size(); ++i) {
      if (seqs[i][k] != p) {
        allSame = false;
        break;
      }
    }
    if (!allSame)
      break;
  }
  return k;
}

static inline std::vector<std::string> primaryTypeSequence(const ObsHistPtr &h) {
  auto seqs = detail_metrics::typeSequences(h);
  if (seqs.empty())
    return {};
  return seqs.front();
}

static inline std::vector<TypeAtom> primaryTypeAtomSequence(const ObsHistPtr &h) {
  auto seqs = detail_metrics::typeAtomSequences(h);
  if (seqs.empty())
    return {};
  return seqs.front();
}

static inline std::vector<std::string>
renderTypeAtomSequence(const std::vector<TypeAtom> &seq) {
  std::vector<std::string> out;
  for (const auto &a : seq)
    out.push_back(a.text);
  return out;
}

static inline void noteResidualBranchTypes(
    const std::vector<TypeAtom> &seq,
    size_t commonLen,
    std::set<std::string> &nodeTypes,
    std::set<std::string> &nodeNonVoidTypes,
    BranchCastMetrics &bm) {
  for (size_t i = commonLen; i < seq.size(); ++i) {
    const TypeAtom &ty = seq[i];

    bool isPtr = ty.hasStructuredInfo()
                     ? ty.isPointer
                     : detail_metrics::isPointerTypeText(ty.text);
    bool isVoidPtr = ty.hasStructuredInfo()
                         ? ty.isVoidPointer
                         : detail_metrics::isVoidPointerTypeText(ty.text);
    if (!isPtr)
      continue;

    nodeTypes.insert(ty.text);
    bm.branchTypes.insert(ty.text);

    if (!isVoidPtr) {
      nodeNonVoidTypes.insert(ty.text);
      bm.branchNonVoidTypes.insert(ty.text);
    }
  }
}

static inline void noteResidualBranchTypes(
    const std::vector<std::string> &seq,
    size_t commonLen,
    std::set<std::string> &nodeTypes,
    std::set<std::string> &nodeNonVoidTypes,
    BranchCastMetrics &bm) {
  for (size_t i = commonLen; i < seq.size(); ++i) {
    std::string ty = typeFromSeqPart(seq[i]);
    if (!detail_metrics::isPointerTypeText(ty))
      continue;

    nodeTypes.insert(ty);
    bm.branchTypes.insert(ty);

    if (!detail_metrics::isVoidPointerTypeText(ty)) {
      nodeNonVoidTypes.insert(ty);
      bm.branchNonVoidTypes.insert(ty);
    }
  }
}

static inline void collectBranchMetricsFromObs(const ObsHistPtr &h,
                                               int prefixDepth,
                                               BranchCastMetrics &bm) {
  if (!h)
    return;

  switch (h->kind) {
  case ObsKind::AsType:
  case ObsKind::PtrArith:
  case ObsKind::ParamRef:
  case ObsKind::ReturnRef:
  case ObsKind::HOFParamRef:
  case ObsKind::Unknown:
    return;

  case ObsKind::Seq: {
    collectBranchMetricsFromObs(h->first, prefixDepth, bm);
    int leftDepth = detail_metrics::computeSeqDepth(h->first);
    collectBranchMetricsFromObs(h->second, prefixDepth + leftDepth, bm);
    return;
  }

  case ObsKind::Parallel:
    for (const auto &arm : h->arms)
      collectBranchMetricsFromObs(arm, prefixDepth, bm);
    return;

  case ObsKind::CallWrap:
    collectBranchMetricsFromObs(h->inner, prefixDepth, bm);
    return;

  case ObsKind::FieldRef:
    collectBranchMetricsFromObs(h->base, prefixDepth, bm);
    return;

  case ObsKind::Join: {
    ++bm.numBranchNodes;
    bm.totalAlternatives += static_cast<int>(h->alts.size());
    bm.maxBranchBreadth =
        std::max<int>(bm.maxBranchBreadth, h->alts.size());

    // Metric-only factoring:
    //
    // The printed history may be:
    //   void* => ⟨[g] A=>B, [!g] A=>C⟩
    //
    // For branch-location metrics, treat this as:
    //   void* => A => ⟨[g] B, [!g] C⟩
    //
    // This gives branchPosition after the common prefix and branchAltSeqLen
    // over only the branch-specific suffix.
    std::vector<std::vector<TypeAtom>> altAtomSeqs;
    std::vector<std::vector<std::string>> altSeqs;
    for (const auto &alt : h->alts) {
      auto atomSeq = primaryTypeAtomSequence(alt.hist);
      altSeqs.push_back(renderTypeAtomSequence(atomSeq));
      altAtomSeqs.push_back(std::move(atomSeq));
    }

    size_t commonLen = commonPrefixLen(altSeqs);
    int factoredBranchDepth = prefixDepth + static_cast<int>(commonLen);
    noteBranchDepth(bm, factoredBranchDepth);

    std::set<std::string> nodeTypes;
    std::set<std::string> nodeNonVoidTypes;
    bool nodeHasVoidOnlyAlt = false;

    for (size_t ai = 0; ai < h->alts.size(); ++ai) {
      const auto &alt = h->alts[ai];
      std::string guard = normalizeGuardText(alt.guard.text);
      if (!guard.empty())
        bm.guards.insert(guard);
      if (guardLooksTagLike(guard))
        bm.hasTagLikeGuard = true;

      const std::vector<TypeAtom> &seq =
          ai < altAtomSeqs.size() ? altAtomSeqs[ai] : primaryTypeAtomSequence(alt.hist);
      int residualDepth = 0;
      if (seq.size() > commonLen)
        residualDepth = static_cast<int>(seq.size() - commonLen);
      noteAltDepth(bm, residualDepth);

      noteResidualBranchTypes(seq, commonLen, nodeTypes, nodeNonVoidTypes, bm);

      bool hasPointer = false;
      bool hasNonVoidPointer = false;
      for (size_t i = commonLen; i < seq.size(); ++i) {
        const TypeAtom &ty = seq[i];
        bool isPtr = ty.hasStructuredInfo()
                         ? ty.isPointer
                         : detail_metrics::isPointerTypeText(ty.text);
        bool isVoidPtr = ty.hasStructuredInfo()
                             ? ty.isVoidPointer
                             : detail_metrics::isVoidPointerTypeText(ty.text);
        if (isPtr) {
          hasPointer = true;
          if (!isVoidPtr)
            hasNonVoidPointer = true;
        }
      }
      if (hasPointer && !hasNonVoidPointer)
        nodeHasVoidOnlyAlt = true;

      // Nested branch positions should include the factored prefix plus the
      // residual prefix inside this alternative.
      collectBranchMetricsFromObs(
          alt.hist,
          factoredBranchDepth,
          bm);
    }

    bm.maxTypesPerBranchNode =
        std::max<int>(bm.maxTypesPerBranchNode, nodeNonVoidTypes.size());
    if (nodeNonVoidTypes.size() > 1)
      bm.hasDifferentTypesAcrossBranches = true;
    if (nodeNonVoidTypes.size() == 1 && h->alts.size() > 1)
      bm.hasSameTypeAllBranches = true;
    if (nodeHasVoidOnlyAlt)
      bm.hasVoidOnlyBranch = true;

    return;
  }
  }
}

static inline void addBranchWriteMetrics(BranchCastMetrics &bm,
                                         const std::vector<PointerUseRecord> *uses) {
  if (!uses)
    return;
  for (const auto &u : *uses) {
    if (!u.guard)
      continue;
    if (!guardTextMatches(u.guard->text, bm.guards))
      continue;
    if (!detail_metrics::isDerefLike(u.kind) ||
        !detail_metrics::isWriteLike(u.kind))
      continue;

    std::string ty = detail_metrics::trim(u.pointerType);
    if (!detail_metrics::isPointerTypeText(ty) ||
        detail_metrics::isVoidPointerTypeText(ty))
      continue;

    bm.hasBranchSpecificWrite = true;
    bm.branchWriteTypes.insert(ty);

    if (detail_metrics::isCharPointerTypeText(ty))
      bm.hasBranchSpecificByteWrite = true;
    else
      bm.hasBranchSpecificConcreteWrite = true;
  }
}

} // namespace detail_branch_metrics

static inline BranchCastMetrics
computeBranchCastMetrics(const ObsHistPtr &h,
                         const std::optional<std::string> &baseText = std::nullopt,
                         const std::vector<PointerUseRecord> *uses = nullptr) {
  BranchCastMetrics bm;

  int baseDepth = 0;
  if (baseText) {
    baseDepth = detail_metrics::typeChainDepth(*baseText);
    if (detail_metrics::renderedBaseLooksBranchy(*baseText)) {
      ++bm.numBranchNodes;
      int n = detail_metrics::countBranchAlternativesInRenderedBase(*baseText);
      bm.totalAlternatives += std::max(1, n);
      bm.maxBranchBreadth = std::max(bm.maxBranchBreadth, std::max(1, n));
      bm.earliestBranchDepth = 0;
      bm.latestBranchDepth = 0;
      bm.maxBranchDepth = 0;
      bm.hasDifferentTypesAcrossBranches = true; // approximate rendered branch base
    }
  }

  detail_branch_metrics::collectBranchMetricsFromObs(h, baseDepth, bm);
  detail_branch_metrics::addBranchWriteMetrics(bm, uses);

  bm.numDistinctBranchTypes = static_cast<int>(bm.branchTypes.size());
  bm.numDistinctBranchNonVoidTypes =
      static_cast<int>(bm.branchNonVoidTypes.size());
  bm.hasNegatedPairGuards =
      detail_branch_metrics::guardsLookNegatedPair(bm.guards);
  if (bm.numBranchNodes > 0 && bm.totalAlternatives < bm.numBranchNodes * 2)
    bm.hasMissingElseAlternative = true;

  return bm;
}

static inline BranchCastClass classifyBranchCast(const BranchCastMetrics &bm) {
  if (bm.numBranchNodes == 0)
    return BranchCastClass::NoBranchCast;
  if (bm.hasBranchSpecificConcreteWrite)
    return BranchCastClass::BranchSpecificConcreteWrite;
  if (bm.hasBranchSpecificByteWrite)
    return BranchCastClass::BranchSpecificByteWrite;
  if (bm.hasBranchSpecificWrite)
    return BranchCastClass::BranchSpecificWrite;
  if (bm.hasTagLikeGuard && bm.hasDifferentTypesAcrossBranches)
    return BranchCastClass::TagLikeTypeSplit;
  if (bm.hasDifferentTypesAcrossBranches)
    return BranchCastClass::GuardedTypeSplit;
  if (bm.hasSameTypeAllBranches)
    return BranchCastClass::SameTypeBranch;
  if (bm.hasMissingElseAlternative)
    return BranchCastClass::IncompleteBranchInfo;
  return BranchCastClass::Unknown;
}

static inline std::string branchDepthSummary(const BranchCastMetrics &bm) {
  if (bm.numBranchNodes == 0 || bm.earliestBranchDepth < 0)
    return "<none>";
  std::ostringstream os;
  os << "earliest=" << bm.earliestBranchDepth
     << ", latest=" << bm.latestBranchDepth
     << ", max=" << bm.maxBranchDepth;
  return os.str();
}

static inline std::string branchAltLengthSummary(const BranchCastMetrics &bm) {
  if (bm.totalAlternatives == 0 || bm.minAltSeqDepth < 0)
    return "<none>";
  std::ostringstream os;
  os << "min=" << bm.minAltSeqDepth
     << ", max=" << bm.maxAltSeqDepth
     << ", avg=";
  double avg = static_cast<double>(bm.totalAltSeqDepth) /
               static_cast<double>(bm.totalAlternatives);
  os.setf(std::ios::fixed);
  os.precision(2);
  os << avg;
  return os.str();
}


static inline HistoryShapeClass classifyHistoryShape(const HistoryMetrics &m) {
  if (m.numHORefs > 0)
    return HistoryShapeClass::FunctionParameterDependent;
  if (m.numBranchNodes > 0 && m.numNonVoidPointerTypes > 1)
    return HistoryShapeClass::BranchPolymorphic;
  if (m.numParallelNodes > 0 && m.numNonVoidPointerTypes > 1)
    return HistoryShapeClass::ParallelPolymorphic;
  if (m.hasSequentialConcreteReinterpretation)
    return HistoryShapeClass::SequentialConcreteReinterpretation;
  if (m.hasCharPtr && m.numNonVoidPointerTypes > 1)
    return HistoryShapeClass::ByteView;
  if (m.numPointerTypes > 0 && m.numNonVoidPointerTypes <= 1)
    return HistoryShapeClass::StableViaVoid;
  return HistoryShapeClass::Unknown;
}

static inline HistoryRiskClass
classifyHistoryRisk(const HistoryMetrics &m,
                    const std::optional<std::string> &baseText = std::nullopt) {
  (void)baseText;

  // historyRisk is intentionally independent of pointer-use operations.
  // Reads/writes/dereferences are reflected in WildPointerBehaviorClass instead.
  if (m.hasIntToPtr || m.hasPtrToInt)
    return HistoryRiskClass::DangerousProvenance;
  if (m.hasExternalSink)
    return HistoryRiskClass::ExternalSink;
  if (m.numHORefs > 0)
    return HistoryRiskClass::FunctionParameterDependent;

  // Structurally dangerous reinterpretations: the cast-history itself moves
  // through byte-level or pointer-arithmetic views into a different concrete
  // type, even before considering whether that view is dereferenced.
  if (m.hasByteOffsetToDifferentConcrete ||
      m.hasByteMediatedReinterpretation)
    return HistoryRiskClass::DangerousReinterpretation;

  // Moderate structural complexity: multiple alternatives, multiple concrete
  // target views, branch/parallel polymorphism, byte-to-concrete conversion,
  // direct concrete-to-concrete reinterpretation, or pointer arithmetic that is
  // not already classified as a different-concrete dangerous case.
  if ((m.numBranchNodes > 0 && m.numNonVoidPointerTypes > 1) ||
      (m.numParallelNodes > 0 && m.numNonVoidPointerTypes > 1) ||
      m.numNonVoidPointerTypes > 1 ||
      m.hasParallelSequentialArm ||
      m.hasByteViewToConcrete ||
      m.hasSequentialConcreteReinterpretation ||
      m.hasByteOffsetRoundTrip ||
      m.hasPointerArithmetic)
    return HistoryRiskClass::Moderate;

  if (m.numPointerTypes > 0)
    return HistoryRiskClass::Low;
  return HistoryRiskClass::Unknown;
}

static inline HistoryClass classifyHistory(const HistoryMetrics &m) {
  if (m.hasIntToPtr || m.hasPtrToInt)
    return HistoryClass::DangerousProvenance;
  if (m.numHORefs > 0)
    return HistoryClass::FunctionParameterDependent;
  if (m.numBranchNodes > 0 && m.numNonVoidPointerTypes > 1)
    return HistoryClass::BranchPolymorphic;
  if (m.numParallelNodes > 0 && m.numNonVoidPointerTypes > 1)
    return HistoryClass::ParallelPolymorphic;
  if (m.hasCharPtr && m.numNonVoidPointerTypes > 1)
    return HistoryClass::ByteView;
  if (m.numPointerTypes > 0 && m.numNonVoidPointerTypes <= 1)
    return HistoryClass::StableViaVoid;
  if (m.hasExternalSink)
    return HistoryClass::ExternalSink;
  return HistoryClass::Unknown;
}


// Behavior-oriented refinement for void*/cast-heavy ("wild-like") pointers.
// This is intentionally separate from risk: it describes what kind of type
// protocol/use pattern the pointer exhibits within the current translation unit.
enum class WildPointerBehaviorClass {
  NotWildLike,
  OpaqueForwarding,
  TypeErasedToVoidNoDeref,
  ConcreteSpecialized,
  ConcreteSpecializedRead,
  ConcreteSpecializedWrite,
  ByteViewNoDeref,
  ByteViewToConcreteNoDeref,
  ByteMediatedReinterpretationNoDeref,
  SequentialConcreteReinterpretationNoDeref,
  ByteOffsetRoundTripNoDeref,
  ByteOffsetToDifferentConcreteNoDeref,
  ByteViewWrite,
  MultiTypeNoDeref,
  MultiTargetParallelNoDeref,
  ParallelSequentialReinterpretationNoDeref,
  BranchTypeSplitNoDeref,
  MultiTypeRead,
  MultiTypeWrite,
  BranchTaggedUnionLike,
  CallbackDependentPointer,
  CallbackDependentByteViewPointer,
  ExternalUnknown,
  DangerousProvenance,
  Unknown
};

static inline std::string wildPointerBehaviorClassName(WildPointerBehaviorClass c) {
  switch (c) {
  case WildPointerBehaviorClass::NotWildLike: return "NotWildLike";
  case WildPointerBehaviorClass::OpaqueForwarding: return "OpaqueForwarding";
  case WildPointerBehaviorClass::TypeErasedToVoidNoDeref: return "TypeErasedToVoidNoDeref";
  case WildPointerBehaviorClass::ConcreteSpecialized: return "ConcreteSpecialized";
  case WildPointerBehaviorClass::ConcreteSpecializedRead: return "ConcreteSpecializedRead";
  case WildPointerBehaviorClass::ConcreteSpecializedWrite: return "ConcreteSpecializedWrite";
  case WildPointerBehaviorClass::ByteViewNoDeref: return "ByteViewNoDeref";
  case WildPointerBehaviorClass::ByteViewToConcreteNoDeref: return "ByteViewToConcreteNoDeref";
  case WildPointerBehaviorClass::ByteMediatedReinterpretationNoDeref: return "ByteMediatedReinterpretationNoDeref";
  case WildPointerBehaviorClass::SequentialConcreteReinterpretationNoDeref: return "SequentialConcreteReinterpretationNoDeref";
  case WildPointerBehaviorClass::ByteOffsetRoundTripNoDeref: return "ByteOffsetRoundTripNoDeref";
  case WildPointerBehaviorClass::ByteOffsetToDifferentConcreteNoDeref: return "ByteOffsetToDifferentConcreteNoDeref";
  case WildPointerBehaviorClass::ByteViewWrite: return "ByteViewWrite";
  case WildPointerBehaviorClass::MultiTypeNoDeref: return "MultiTypeNoDeref";
  case WildPointerBehaviorClass::MultiTargetParallelNoDeref: return "MultiTargetParallelNoDeref";
  case WildPointerBehaviorClass::ParallelSequentialReinterpretationNoDeref: return "ParallelSequentialReinterpretationNoDeref";
  case WildPointerBehaviorClass::BranchTypeSplitNoDeref: return "BranchTypeSplitNoDeref";
  case WildPointerBehaviorClass::MultiTypeRead: return "MultiTypeRead";
  case WildPointerBehaviorClass::MultiTypeWrite: return "MultiTypeWrite";
  case WildPointerBehaviorClass::BranchTaggedUnionLike: return "BranchTaggedUnionLike";
  case WildPointerBehaviorClass::CallbackDependentPointer: return "CallbackDependentPointer";
  case WildPointerBehaviorClass::CallbackDependentByteViewPointer: return "CallbackDependentByteViewPointer";
  case WildPointerBehaviorClass::ExternalUnknown: return "ExternalUnknown";
  case WildPointerBehaviorClass::DangerousProvenance: return "DangerousProvenance";
  case WildPointerBehaviorClass::Unknown: return "Unknown";
  }
  return "Unknown";
}


enum class ViewCompletenessClass {
  FullView,
  PartialView,
  LocalView,
  SinkOnlyView,
  DebugOnlyView,
  Unknown
};

static inline std::string viewCompletenessClassName(ViewCompletenessClass c) {
  switch (c) {
  case ViewCompletenessClass::FullView: return "FullView";
  case ViewCompletenessClass::PartialView: return "PartialView";
  case ViewCompletenessClass::LocalView: return "LocalView";
  case ViewCompletenessClass::SinkOnlyView: return "SinkOnlyView";
  case ViewCompletenessClass::DebugOnlyView: return "DebugOnlyView";
  case ViewCompletenessClass::Unknown: return "Unknown";
  }
  return "Unknown";
}


// Callback mediation is intentionally separate from the shape and operation
// classifiers.  A generic helper summary may retain a symbolic $fn.$arg
// reference, whereas a caller that supplies a known callback is resolved and
// should retain callback provenance without being labelled callback-dependent.
enum class CallbackMediationState {
  None,
  FormalCallbackParameter,
  UnresolvedCallback,
  FieldCallback,
  ResolvedCallback,
  ResolvedFieldCallback,
  GuardedResolvedFieldCallback,
  Unknown
};

static inline std::string
callbackMediationStateName(CallbackMediationState state) {
  switch (state) {
  case CallbackMediationState::None: return "None";
  case CallbackMediationState::FormalCallbackParameter:
    return "FormalCallbackParameter";
  case CallbackMediationState::UnresolvedCallback:
    return "UnresolvedCallback";
  case CallbackMediationState::FieldCallback: return "FieldCallback";
  case CallbackMediationState::ResolvedCallback: return "ResolvedCallback";
  case CallbackMediationState::ResolvedFieldCallback:
    return "ResolvedFieldCallback";
  case CallbackMediationState::GuardedResolvedFieldCallback:
    return "GuardedResolvedFieldCallback";
  case CallbackMediationState::Unknown: return "Unknown";
  }
  return "Unknown";
}

static inline bool callbackMediationStateIsResolved(
    CallbackMediationState state) {
  return state == CallbackMediationState::ResolvedCallback ||
         state == CallbackMediationState::ResolvedFieldCallback ||
         state == CallbackMediationState::GuardedResolvedFieldCallback;
}

static inline bool callbackMediationStateIsFieldCarrier(
    CallbackMediationState state) {
  return state == CallbackMediationState::FieldCallback ||
         state == CallbackMediationState::ResolvedFieldCallback ||
         state == CallbackMediationState::GuardedResolvedFieldCallback;
}

static inline bool callbackMediationStateIsDependent(
    CallbackMediationState state) {
  return state == CallbackMediationState::FormalCallbackParameter ||
         state == CallbackMediationState::UnresolvedCallback ||
         state == CallbackMediationState::FieldCallback;
}

enum class FunctionParameterRoleClass {
  OrdinaryParameter,
  CallbackBehaviorProvider,
  CallbackDependentDataPointer,
  FieldCallbackDependentDataPointer,
  ResolvedCallbackMediatedDataPointer,
  ResolvedFieldCallbackMediatedDataPointer,
  GuardedResolvedFieldCallbackMediatedDataPointer,
  Unknown
};

static inline std::string
functionParameterRoleClassName(FunctionParameterRoleClass c) {
  switch (c) {
  case FunctionParameterRoleClass::OrdinaryParameter:
    return "OrdinaryParameter";
  case FunctionParameterRoleClass::CallbackBehaviorProvider:
    return "CallbackBehaviorProvider";
  case FunctionParameterRoleClass::CallbackDependentDataPointer:
    return "CallbackDependentDataPointer";
  case FunctionParameterRoleClass::FieldCallbackDependentDataPointer:
    return "FieldCallbackDependentDataPointer";
  case FunctionParameterRoleClass::ResolvedCallbackMediatedDataPointer:
    return "ResolvedCallbackMediatedDataPointer";
  case FunctionParameterRoleClass::ResolvedFieldCallbackMediatedDataPointer:
    return "ResolvedFieldCallbackMediatedDataPointer";
  case FunctionParameterRoleClass::GuardedResolvedFieldCallbackMediatedDataPointer:
    return "GuardedResolvedFieldCallbackMediatedDataPointer";
  case FunctionParameterRoleClass::Unknown:
    return "Unknown";
  }
  return "Unknown";
}

static inline FunctionParameterRoleClass
classifyFunctionParameterRole(const TypeAtom &declaredType,
                              const HistoryMetrics &m,
                              CallbackMediationState callbackState =
                                  CallbackMediationState::Unknown) {
  if (declaredType.hasStructuredInfo() && declaredType.isFunctionPointer)
    return FunctionParameterRoleClass::CallbackBehaviorProvider;

  // Preserve the old conservative fallback for callers that have not yet
  // supplied operation provenance.  Normal reporting paths pass an explicit
  // state computed from pointer-use evidence.
  if (callbackState == CallbackMediationState::Unknown) {
    if (m.numHORefs > 0 || m.hasIndirectCallUse)
      return FunctionParameterRoleClass::CallbackDependentDataPointer;
    return FunctionParameterRoleClass::OrdinaryParameter;
  }

  switch (callbackState) {
  case CallbackMediationState::FormalCallbackParameter:
  case CallbackMediationState::UnresolvedCallback:
    return FunctionParameterRoleClass::CallbackDependentDataPointer;
  case CallbackMediationState::FieldCallback:
    return FunctionParameterRoleClass::FieldCallbackDependentDataPointer;
  case CallbackMediationState::ResolvedCallback:
    return FunctionParameterRoleClass::ResolvedCallbackMediatedDataPointer;
  case CallbackMediationState::ResolvedFieldCallback:
    return FunctionParameterRoleClass::ResolvedFieldCallbackMediatedDataPointer;
  case CallbackMediationState::GuardedResolvedFieldCallback:
    return FunctionParameterRoleClass::GuardedResolvedFieldCallbackMediatedDataPointer;
  case CallbackMediationState::None:
  case CallbackMediationState::Unknown:
    return FunctionParameterRoleClass::OrdinaryParameter;
  }
  return FunctionParameterRoleClass::OrdinaryParameter;
}


enum class FunctionBehaviorClass {
  OrdinaryFunction,
  OpaquePolymorphicFunction,
  BytewiseGenericFunction,
  ConcreteSpecializerFunction,
  CallbackAdapterFunction,
  // The callback carrier is a struct/union field rather than an ordinary
  // function-pointer formal parameter.
  FieldCallbackAdapterFunction,
  // This function locally selects guarded field-held targets and then routes a
  // data pointer through an adapter/helper.
  GuardedFieldCallbackDispatcherFunction,
  ReinterpretingWriteFunction,
  TaggedUnionLikeFunction,
  MultiTargetReinterpretationFunction,
  UnsafeReinterpretationFunction,
  MixedPointerBehaviorFunction,
  Unknown
};

static inline std::string functionBehaviorClassName(FunctionBehaviorClass c) {
  switch (c) {
  case FunctionBehaviorClass::OrdinaryFunction:
    return "OrdinaryFunction";
  case FunctionBehaviorClass::OpaquePolymorphicFunction:
    return "OpaquePolymorphicFunction";
  case FunctionBehaviorClass::BytewiseGenericFunction:
    return "BytewiseGenericFunction";
  case FunctionBehaviorClass::ConcreteSpecializerFunction:
    return "ConcreteSpecializerFunction";
  case FunctionBehaviorClass::CallbackAdapterFunction:
    return "CallbackAdapterFunction";
  case FunctionBehaviorClass::FieldCallbackAdapterFunction:
    return "FieldCallbackAdapterFunction";
  case FunctionBehaviorClass::GuardedFieldCallbackDispatcherFunction:
    return "GuardedFieldCallbackDispatcherFunction";
  case FunctionBehaviorClass::ReinterpretingWriteFunction:
    return "ReinterpretingWriteFunction";
  case FunctionBehaviorClass::TaggedUnionLikeFunction:
    return "TaggedUnionLikeFunction";
  case FunctionBehaviorClass::MultiTargetReinterpretationFunction:
    return "MultiTargetReinterpretationFunction";
  case FunctionBehaviorClass::UnsafeReinterpretationFunction:
    return "UnsafeReinterpretationFunction";
  case FunctionBehaviorClass::MixedPointerBehaviorFunction:
    return "MixedPointerBehaviorFunction";
  case FunctionBehaviorClass::Unknown:
    return "Unknown";
  }
  return "Unknown";
}

struct FunctionBehaviorEvidence {
  bool hasInterestingPointerParam = false;
  bool hasOpaqueOnly = false;
  bool hasTypeErasure = false;
  bool hasBytewise = false;
  bool hasConcreteSpecializer = false;
  bool hasReinterpretingWrite = false;
  bool hasByteWrite = false;
  bool hasConcreteWrite = false;
  bool hasCallbackProvider = false;
  bool hasCallbackDependent = false;
  bool hasFieldCallbackProvider = false;
  bool hasGuardedFieldCallbackSelection = false;
  bool hasResolvedCallbackMediation = false;
  bool hasTaggedUnionLike = false;
  bool hasMultiTargetReinterpretation = false;
  bool hasUnsafeReinterpretation = false;
  bool hasMixedMildPatterns = false;
};

static inline void
accumulateFunctionBehaviorEvidence(FunctionBehaviorEvidence &e,
                                   WildPointerBehaviorClass behavior,
                                   FunctionParameterRoleClass role) {
  if (role == FunctionParameterRoleClass::CallbackBehaviorProvider) {
    e.hasInterestingPointerParam = true;
    e.hasCallbackProvider = true;
  }
  if (role == FunctionParameterRoleClass::CallbackDependentDataPointer) {
    e.hasInterestingPointerParam = true;
    e.hasCallbackDependent = true;
  }
  if (role == FunctionParameterRoleClass::FieldCallbackDependentDataPointer) {
    e.hasInterestingPointerParam = true;
    e.hasCallbackDependent = true;
    e.hasFieldCallbackProvider = true;
  }
  if (role == FunctionParameterRoleClass::ResolvedCallbackMediatedDataPointer ||
      role == FunctionParameterRoleClass::ResolvedFieldCallbackMediatedDataPointer ||
      role == FunctionParameterRoleClass::GuardedResolvedFieldCallbackMediatedDataPointer) {
    e.hasInterestingPointerParam = true;
    e.hasResolvedCallbackMediation = true;
    if (role != FunctionParameterRoleClass::ResolvedCallbackMediatedDataPointer)
      e.hasFieldCallbackProvider = true;
    if (role == FunctionParameterRoleClass::GuardedResolvedFieldCallbackMediatedDataPointer)
      e.hasGuardedFieldCallbackSelection = true;
  }

  switch (behavior) {
  case WildPointerBehaviorClass::NotWildLike:
    return;

  case WildPointerBehaviorClass::OpaqueForwarding:
    e.hasInterestingPointerParam = true;
    e.hasOpaqueOnly = true;
    return;

  case WildPointerBehaviorClass::TypeErasedToVoidNoDeref:
    e.hasInterestingPointerParam = true;
    e.hasTypeErasure = true;
    return;

  case WildPointerBehaviorClass::ByteViewNoDeref:
  case WildPointerBehaviorClass::ByteOffsetRoundTripNoDeref:
    e.hasInterestingPointerParam = true;
    e.hasBytewise = true;
    return;

  case WildPointerBehaviorClass::ConcreteSpecialized:
  case WildPointerBehaviorClass::ConcreteSpecializedRead:
    e.hasInterestingPointerParam = true;
    e.hasConcreteSpecializer = true;
    return;

  case WildPointerBehaviorClass::ConcreteSpecializedWrite:
    e.hasInterestingPointerParam = true;
    e.hasConcreteSpecializer = true;
    e.hasReinterpretingWrite = true;
    e.hasConcreteWrite = true;
    return;

  case WildPointerBehaviorClass::CallbackDependentPointer:
  case WildPointerBehaviorClass::CallbackDependentByteViewPointer:
    e.hasInterestingPointerParam = true;
    e.hasCallbackDependent = true;
    return;

  case WildPointerBehaviorClass::BranchTaggedUnionLike:
  case WildPointerBehaviorClass::BranchTypeSplitNoDeref:
    e.hasInterestingPointerParam = true;
    e.hasTaggedUnionLike = true;
    return;

  case WildPointerBehaviorClass::MultiTargetParallelNoDeref:
  case WildPointerBehaviorClass::ParallelSequentialReinterpretationNoDeref:
  case WildPointerBehaviorClass::MultiTypeNoDeref:
  case WildPointerBehaviorClass::ByteViewToConcreteNoDeref:
  case WildPointerBehaviorClass::ByteMediatedReinterpretationNoDeref:
  case WildPointerBehaviorClass::SequentialConcreteReinterpretationNoDeref:
  case WildPointerBehaviorClass::ByteOffsetToDifferentConcreteNoDeref:
    e.hasInterestingPointerParam = true;
    e.hasMultiTargetReinterpretation = true;
    return;

  case WildPointerBehaviorClass::ByteViewWrite:
    e.hasInterestingPointerParam = true;
    e.hasReinterpretingWrite = true;
    e.hasByteWrite = true;
    return;

  case WildPointerBehaviorClass::MultiTypeRead:
  case WildPointerBehaviorClass::MultiTypeWrite:
  case WildPointerBehaviorClass::DangerousProvenance:
  case WildPointerBehaviorClass::ExternalUnknown:
    e.hasInterestingPointerParam = true;
    e.hasUnsafeReinterpretation = true;
    return;

  case WildPointerBehaviorClass::Unknown:
    e.hasInterestingPointerParam = true;
    e.hasMixedMildPatterns = true;
    return;
  }
}

static inline FunctionBehaviorClass
classifyFunctionBehavior(const FunctionBehaviorEvidence &e) {
  if (!e.hasInterestingPointerParam)
    return FunctionBehaviorClass::OrdinaryFunction;

  // High-signal classes first.
  if (e.hasUnsafeReinterpretation)
    return FunctionBehaviorClass::UnsafeReinterpretationFunction;

  // A single write through a reinterpreted view is a shared high-level
  // category.  Evidence flags distinguish byte-view writes from concrete
  // specialization writes.
  if (e.hasReinterpretingWrite)
    return FunctionBehaviorClass::ReinterpretingWriteFunction;

  if (e.hasTaggedUnionLike)
    return FunctionBehaviorClass::TaggedUnionLikeFunction;

  if (e.hasGuardedFieldCallbackSelection &&
      (e.hasCallbackDependent || e.hasResolvedCallbackMediation))
    return FunctionBehaviorClass::GuardedFieldCallbackDispatcherFunction;

  if (e.hasFieldCallbackProvider &&
      (e.hasCallbackDependent || e.hasResolvedCallbackMediation))
    return FunctionBehaviorClass::FieldCallbackAdapterFunction;

  if (e.hasCallbackProvider && e.hasCallbackDependent)
    return FunctionBehaviorClass::CallbackAdapterFunction;

  if (e.hasMultiTargetReinterpretation)
    return FunctionBehaviorClass::MultiTargetReinterpretationFunction;

  // Mild patterns.
  bool mildKinds = false;
  int mildCount = 0;
  if (e.hasBytewise) { mildKinds = true; ++mildCount; }
  if (e.hasConcreteSpecializer) { mildKinds = true; ++mildCount; }
  if (e.hasTypeErasure || e.hasOpaqueOnly) { mildKinds = true; ++mildCount; }

  if (mildCount > 1)
    return FunctionBehaviorClass::MixedPointerBehaviorFunction;

  if (e.hasBytewise)
    return FunctionBehaviorClass::BytewiseGenericFunction;

  if (e.hasConcreteSpecializer)
    return FunctionBehaviorClass::ConcreteSpecializerFunction;

  if (e.hasTypeErasure || e.hasOpaqueOnly ||
      (e.hasCallbackProvider && !e.hasCallbackDependent))
    return FunctionBehaviorClass::OpaquePolymorphicFunction;

  if (mildKinds || e.hasMixedMildPatterns)
    return FunctionBehaviorClass::MixedPointerBehaviorFunction;

  return FunctionBehaviorClass::Unknown;
}


struct WildPointerBehaviorMetrics {
  bool wildLike = false;
  bool hasVoidInterface = false;
  bool hasCastLikeReinterpretation = false;
  bool hasHigherOrderForwarding = false;
  bool hasExternalUnknown = false;
  bool hasBranchTypeSplit = false;
  bool hasByteView = false;
  bool hasByteViewRead = false;
  bool hasByteViewWrite = false;
  bool hasConcreteDeref = false;
  bool hasConcreteWrite = false;
  bool hasMultiTypeDeref = false;
  bool hasMultiTypeWrite = false;
  bool hasAnyPointerUse = false;
  bool hasDerefLikeUse = false;
  bool hasWriteLikeUse = false;
  bool hasCallOrReturnUse = false;

  int numDerefTypes = 0;
  int numWriteDerefTypes = 0;

  std::set<std::string> derefTypes;
  std::set<std::string> writeDerefTypes;
};


// C4R0: evidence-source vocabulary for operation-aware behavior reports.
//
// Pointer-use propagation is intentionally still separate from cast-history
// solving.  These helpers only classify where already-existing PointerUseRecord
// evidence came from so that behavior labels such as ConcreteSpecializedWrite
// and MultiTypeWrite can be explained in the text and JSON reports.
struct PointerUseEvidenceMetrics {
  bool hasLocal = false;
  bool hasSameTUNamedCall = false;
  bool hasSameTURecursive = false;
  bool hasImportedDirectCall = false;
  bool hasImportedFunctionPointer = false;
  bool hasProjectSCC = false;
  bool hasReturnAliasContinuation = false;
  bool hasResolvedHigherOrder = false;
  bool hasIndirectCallArgument = false;
  bool hasMemoryLibraryAPI = false;
  bool hasStringLibraryAPI = false;

  std::set<std::string> sources;
  std::set<std::string> operationKinds;
  std::set<std::string> derefSources;
  std::set<std::string> writeSources;
  std::map<std::string, int> operationCountBySource;
};

static inline bool pointerUseDetailContains(const std::string &detail,
                                            const std::string &needle) {
  return detail.find(needle) != std::string::npos;
}

static inline bool pointerUseDetailHasPrefixOrSegment(
    const std::string &detail, const std::string &prefix) {
  return detail.rfind(prefix, 0) == 0 ||
         detail.find("; " + prefix) != std::string::npos;
}

static inline std::set<std::string>
pointerUseEvidenceSourceLabels(const PointerUseRecord &u) {
  std::set<std::string> labels;
  const std::string &d = u.detail;

  if (pointerUseDetailHasPrefixOrSegment(d, "project-scc via "))
    labels.insert("project-scc-pointer-use");

  // C6g4n1: C6g4n intentionally projects `recursive via f.$i` relay text out
  // of same-TU solver records.  Preserve the reporting/evidence distinction
  // through the existing provenance source field instead of depending only on
  // display detail.  Keep the legacy detail check for non-projected records
  // and imported summaries produced by older analyzer versions.
  const std::string &transferSource = u.provenance.source;
  const bool recursiveSameTUProvenance =
      u.provenance.recursiveNamedCallEvidence ||
      transferSource == "same-tu-recursive-fixpoint" ||
      transferSource == "same-tu-field-fnptr-recursive-fixpoint";
  const bool sameTUNamedCallProvenance =
      transferSource == "same-tu-named-call-fixpoint" ||
      transferSource == "same-tu-field-fnptr";
  if (recursiveSameTUProvenance ||
      pointerUseDetailHasPrefixOrSegment(d, "recursive via "))
    labels.insert("same-tu-recursive-pointer-use");
  const bool importedFnPtr =
      pointerUseDetailHasPrefixOrSegment(d, "imported via fnptr ");
  if (importedFnPtr)
    labels.insert("imported-function-pointer-pointer-use");
  if (!importedFnPtr && pointerUseDetailHasPrefixOrSegment(d, "imported via "))
    labels.insert("imported-direct-call-pointer-use");

  // Same-TU named-call propagation is rendered as "via callee.$i".  Preserve it
  // even when it appears after an imported/project-SCC prefix, e.g.
  // "project-scc via g.$0; via relay.$0; ...".
  if (sameTUNamedCallProvenance ||
      pointerUseDetailHasPrefixOrSegment(d, "via "))
    labels.insert("same-tu-named-call-pointer-use");

  if (pointerUseDetailContains(d, "return-alias"))
    labels.insert("return-alias-continuation");
  if (pointerUseDetailContains(d, " -> "))
    labels.insert("resolved-higher-order-pointer-use");
  if (u.kind == PointerUseKind::CallArgument &&
      pointerUseDetailContains(d, "<indirect>"))
    labels.insert("indirect-call-argument");
  if (u.kind == PointerUseKind::MemoryLibCallArgument)
    labels.insert("memory-library-api-use");
  if (u.kind == PointerUseKind::StringLibCallArgument)
    labels.insert("string-library-api-use");

  if (labels.empty())
    labels.insert("local-pointer-use");
  return labels;
}

static inline void addPointerUseEvidenceSource(PointerUseEvidenceMetrics &em,
                                               const std::string &label,
                                               PointerUseKind kind) {
  if (label.empty())
    return;
  em.sources.insert(label);
  ++em.operationCountBySource[label];

  if (label == "local-pointer-use") em.hasLocal = true;
  else if (label == "same-tu-named-call-pointer-use") em.hasSameTUNamedCall = true;
  else if (label == "same-tu-recursive-pointer-use") em.hasSameTURecursive = true;
  else if (label == "imported-direct-call-pointer-use") em.hasImportedDirectCall = true;
  else if (label == "imported-function-pointer-pointer-use") em.hasImportedFunctionPointer = true;
  else if (label == "project-scc-pointer-use") em.hasProjectSCC = true;
  else if (label == "return-alias-continuation") em.hasReturnAliasContinuation = true;
  else if (label == "resolved-higher-order-pointer-use") em.hasResolvedHigherOrder = true;
  else if (label == "indirect-call-argument") em.hasIndirectCallArgument = true;
  else if (label == "memory-library-api-use") em.hasMemoryLibraryAPI = true;
  else if (label == "string-library-api-use") em.hasStringLibraryAPI = true;

  if (detail_metrics::isDerefLike(kind))
    em.derefSources.insert(label);
  if (detail_metrics::isWriteLike(kind))
    em.writeSources.insert(label);
}

static inline void mergePointerUseEvidenceMetrics(PointerUseEvidenceMetrics &dst,
                                                       const PointerUseEvidenceMetrics &src) {
  dst.hasLocal = dst.hasLocal || src.hasLocal;
  dst.hasSameTUNamedCall = dst.hasSameTUNamedCall || src.hasSameTUNamedCall;
  dst.hasSameTURecursive = dst.hasSameTURecursive || src.hasSameTURecursive;
  dst.hasImportedDirectCall = dst.hasImportedDirectCall || src.hasImportedDirectCall;
  dst.hasImportedFunctionPointer =
      dst.hasImportedFunctionPointer || src.hasImportedFunctionPointer;
  dst.hasProjectSCC = dst.hasProjectSCC || src.hasProjectSCC;
  dst.hasReturnAliasContinuation =
      dst.hasReturnAliasContinuation || src.hasReturnAliasContinuation;
  dst.hasResolvedHigherOrder =
      dst.hasResolvedHigherOrder || src.hasResolvedHigherOrder;
  dst.hasIndirectCallArgument =
      dst.hasIndirectCallArgument || src.hasIndirectCallArgument;
  dst.hasMemoryLibraryAPI = dst.hasMemoryLibraryAPI || src.hasMemoryLibraryAPI;
  dst.hasStringLibraryAPI = dst.hasStringLibraryAPI || src.hasStringLibraryAPI;

  dst.sources.insert(src.sources.begin(), src.sources.end());
  dst.operationKinds.insert(src.operationKinds.begin(), src.operationKinds.end());
  dst.derefSources.insert(src.derefSources.begin(), src.derefSources.end());
  dst.writeSources.insert(src.writeSources.begin(), src.writeSources.end());
  for (const auto &entry : src.operationCountBySource)
    dst.operationCountBySource[entry.first] += entry.second;
}

static inline bool pointerUseEvidenceHasNonLocalSource(
    const PointerUseEvidenceMetrics &em) {
  return em.hasSameTUNamedCall || em.hasSameTURecursive ||
         em.hasImportedDirectCall || em.hasImportedFunctionPointer ||
         em.hasProjectSCC || em.hasReturnAliasContinuation ||
         em.hasResolvedHigherOrder;
}

static inline bool pointerUseEvidenceHasImportedOrSolvedSource(
    const PointerUseEvidenceMetrics &em) {
  return em.hasImportedDirectCall || em.hasImportedFunctionPointer ||
         em.hasProjectSCC;
}

static inline std::string pointerUseEvidenceSourceKind(
    const PointerUseEvidenceMetrics &em) {
  // C4R1: stable, paper-facing source buckets for function-level behavior.
  // This intentionally summarizes evidence provenance only; it does not change
  // propagation or classification semantics.  The priority order chooses the
  // most non-local explanation when several sources contribute to the same
  // behavior class.
  if (em.sources.empty())
    return "no-pointer-use-evidence";
  if (em.hasProjectSCC)
    return "project-scc-solved";
  if (em.hasImportedFunctionPointer)
    return "imported-function-pointer";
  if (em.hasImportedDirectCall)
    return "imported-direct-call";
  if (em.hasSameTURecursive)
    return "same-tu-recursive";
  if (em.hasSameTUNamedCall)
    return "same-tu-named-call";
  if (em.hasReturnAliasContinuation)
    return "return-alias-continuation";
  if (em.hasResolvedHigherOrder)
    return "resolved-higher-order";
  if (em.hasMemoryLibraryAPI)
    return "memory-library-api";
  if (em.hasStringLibraryAPI)
    return "string-library-api";
  if (em.hasIndirectCallArgument)
    return "indirect-call-argument";
  if (em.hasLocal)
    return "local-only";
  return "derived";
}

static inline PointerUseEvidenceMetrics
computePointerUseEvidenceMetrics(const std::vector<PointerUseRecord> *uses) {
  PointerUseEvidenceMetrics em;
  if (!uses)
    return em;

  for (const auto &u : *uses) {
    em.operationKinds.insert(pointerUseKindName(u.kind));
    for (const auto &label : pointerUseEvidenceSourceLabels(u))
      addPointerUseEvidenceSource(em, label, u.kind);
  }
  return em;
}

static inline bool callbackProvenanceLooksFieldCarrier(
    const PointerUseRecord &u) {
  const PointerUseProvenance &p = u.provenance;
  return p.edgeKind.find("field-function-pointer") != std::string::npos ||
         p.source.find("field-fnptr") != std::string::npos ||
         p.transferKind.find("field-fnptr") != std::string::npos ||
         u.detail.find("field-function-pointer") != std::string::npos ||
         u.detail.find("field-fnptr") != std::string::npos;
}

static inline bool callbackProvenanceHasKnownTarget(
    const PointerUseRecord &u) {
  const PointerUseProvenance &p = u.provenance;
  auto known = [](const std::string &name) {
    return !name.empty() && name.find("<indirect>") == std::string::npos;
  };
  return known(p.targetFunction) || known(p.targetQualifiedName) ||
         known(p.targetLookupKey);
}

static inline bool callbackGuardLooksNontrivial(const PointerUseRecord &u) {
  const std::optional<Guard> &g =
      u.operationLocalGuardKnown ? u.operationLocalGuard : u.guard;
  if (!g)
    return false;
  const std::string text = detail_metrics::trim(g->text);
  return !text.empty() && text != "always" && text != "<always>";
}

// The state distinguishes a symbolic callback reference in a generic helper
// from a concrete callback already resolved at a caller.  It is classification
// metadata only; it does not alter history solving or pointer-use propagation.
static inline CallbackMediationState computeCallbackMediationState(
    const HistoryMetrics &m,
    const std::vector<PointerUseRecord> *uses = nullptr) {
  bool callback = m.numHORefs > 0 || m.hasIndirectCallUse;
  bool fieldCarrier = false;
  bool resolvedTarget = false;
  bool guardedSelection = false;

  if (uses) {
    for (const PointerUseRecord &u : *uses) {
      const std::set<std::string> labels = pointerUseEvidenceSourceLabels(u);
      const PointerUseProvenance &p = u.provenance;
      const bool callbackUse =
          labels.count("indirect-call-argument") ||
          labels.count("resolved-higher-order-pointer-use") ||
          labels.count("imported-function-pointer-pointer-use") ||
          u.detail.find("<indirect>") != std::string::npos ||
          u.detail.find("higher-order") != std::string::npos ||
          p.edgeKind.find("function-pointer") != std::string::npos ||
          p.edgeKind.find("higher-order") != std::string::npos ||
          p.source.find("fnptr") != std::string::npos ||
          p.transferKind.find("fnptr") != std::string::npos;
      if (!callbackUse)
        continue;
      callback = true;
      fieldCarrier = fieldCarrier || callbackProvenanceLooksFieldCarrier(u);
      resolvedTarget = resolvedTarget ||
                       labels.count("resolved-higher-order-pointer-use") ||
                       labels.count("imported-function-pointer-pointer-use") ||
                       callbackProvenanceHasKnownTarget(u);
      if (callbackGuardLooksNontrivial(u))
        guardedSelection = true;
    }
  }

  // A summary that still contains $fn.$arg remains genuinely dependent, even
  // if a downstream reporting path also retains a resolved-use witness.
  if (m.numHORefs > 0)
    return CallbackMediationState::FormalCallbackParameter;
  if (!callback)
    return CallbackMediationState::None;
  if (resolvedTarget) {
    if (fieldCarrier)
      return guardedSelection
                 ? CallbackMediationState::GuardedResolvedFieldCallback
                 : CallbackMediationState::ResolvedFieldCallback;
    return CallbackMediationState::ResolvedCallback;
  }
  if (fieldCarrier)
    return CallbackMediationState::FieldCallback;
  return CallbackMediationState::UnresolvedCallback;
}

static inline WildPointerBehaviorMetrics
computeWildPointerBehaviorMetrics(const HistoryMetrics &m,
                                  const std::vector<PointerUseRecord> *uses = nullptr) {
  WildPointerBehaviorMetrics wm;
  wm.hasVoidInterface = m.hasVoid;
  wm.hasCastLikeReinterpretation = m.numNonVoidPointerTypes > 1 ||
                                   m.numParallelNodes > 0 ||
                                   m.numBranchNodes > 0 ||
                                   m.numCallWraps > 0;
  wm.hasHigherOrderForwarding = m.numHORefs > 0 || m.hasIndirectCallUse;
  wm.hasExternalUnknown = m.hasExternalSink;
  wm.hasBranchTypeSplit = m.numBranchNodes > 0 && m.numNonVoidPointerTypes > 1;
  wm.hasByteView = m.hasCharPtr;
  wm.wildLike = wm.hasVoidInterface || wm.hasCastLikeReinterpretation ||
                wm.hasHigherOrderForwarding || wm.hasExternalUnknown ||
                m.hasPointerArithmetic || m.hasIntToPtr || m.hasPtrToInt;

  if (uses) {
    for (const auto &u : *uses) {
      wm.hasAnyPointerUse = true;
      if (u.kind == PointerUseKind::CallArgument ||
          u.kind == PointerUseKind::Returned)
        wm.hasCallOrReturnUse = true;
      if (detail_metrics::isWriteLike(u.kind))
        wm.hasWriteLikeUse = true;

      if (!detail_metrics::isDerefLike(u.kind))
        continue;

      wm.hasDerefLikeUse = true;

      std::string ty = detail_metrics::trim(u.pointerType);
      if (!detail_metrics::isPointerTypeText(ty) ||
          detail_metrics::isVoidPointerTypeText(ty))
        continue;

      wm.derefTypes.insert(ty);

      bool isByte = detail_metrics::isCharPointerTypeText(ty);
      bool writeLike = detail_metrics::isWriteLike(u.kind);

      if (isByte && writeLike)
        wm.hasByteViewWrite = true;
      else if (isByte)
        wm.hasByteViewRead = true;

      if (!isByte)
        wm.hasConcreteDeref = true;

      if (writeLike) {
        wm.writeDerefTypes.insert(ty);
        if (!isByte)
          wm.hasConcreteWrite = true;
      }
    }
  }

  wm.numDerefTypes = static_cast<int>(wm.derefTypes.size());
  wm.numWriteDerefTypes = static_cast<int>(wm.writeDerefTypes.size());
  wm.hasMultiTypeDeref = wm.numDerefTypes > 1;
  wm.hasMultiTypeWrite = wm.numWriteDerefTypes > 1;
  return wm;
}

static inline WildPointerBehaviorClass
classifyWildPointerBehavior(
    const HistoryMetrics &m,
    const std::vector<PointerUseRecord> *uses = nullptr,
    CallbackMediationState callbackState = CallbackMediationState::Unknown) {
  WildPointerBehaviorMetrics wm = computeWildPointerBehaviorMetrics(m, uses);
  if (callbackState == CallbackMediationState::Unknown)
    callbackState = computeCallbackMediationState(m, uses);

  if (m.hasIntToPtr || m.hasPtrToInt)
    return WildPointerBehaviorClass::DangerousProvenance;
  if (wm.hasExternalUnknown)
    return WildPointerBehaviorClass::ExternalUnknown;
  if (!wm.wildLike)
    return WildPointerBehaviorClass::NotWildLike;

  if (wm.hasBranchTypeSplit && wm.hasMultiTypeWrite)
    return WildPointerBehaviorClass::BranchTaggedUnionLike;

  if (wm.hasMultiTypeWrite)
    return WildPointerBehaviorClass::MultiTypeWrite;
  if (wm.hasMultiTypeDeref)
    return WildPointerBehaviorClass::MultiTypeRead;

  if (wm.hasBranchTypeSplit && wm.numDerefTypes == 0)
    return WildPointerBehaviorClass::BranchTypeSplitNoDeref;

  if (m.numParallelNodes > 0 &&
      m.numNonVoidPointerTypes > 1 &&
      wm.numDerefTypes == 0 &&
      m.hasParallelSequentialArm)
    return WildPointerBehaviorClass::ParallelSequentialReinterpretationNoDeref;

  if (m.numParallelNodes > 0 &&
      m.numNonVoidPointerTypes > 1 &&
      wm.numDerefTypes == 0)
    return WildPointerBehaviorClass::MultiTargetParallelNoDeref;

  if (wm.hasByteViewWrite)
    return WildPointerBehaviorClass::ByteViewWrite;

  // Operation evidence should dominate a mixed/parallel non-dereferenced view.
  //
  // C3a imported summaries commonly create histories such as:
  //   void* => ⟨| char*, g⌜int*⌝g |⟩
  // with pointer-use evidence:
  //   attach=int*, as=int*, access=int : deref-readwrite
  //
  // The history has two non-void pointer views (char* and int*), but only one
  // concrete dereferenced/write type.  This is not Unknown: operationally it is
  // a concrete specialization/write reached through a wider imported view.
  //
  // Keep true multi-type deref/write classifications above this rule.
  if (wm.hasConcreteWrite && !wm.hasMultiTypeWrite)
    return WildPointerBehaviorClass::ConcreteSpecializedWrite;
  if (wm.hasConcreteDeref && !wm.hasMultiTypeDeref)
    return WildPointerBehaviorClass::ConcreteSpecializedRead;

  if (wm.numDerefTypes == 0) {
    if (m.hasByteOffsetToDifferentConcrete)
      return WildPointerBehaviorClass::ByteOffsetToDifferentConcreteNoDeref;
    if (m.hasByteOffsetRoundTrip)
      return WildPointerBehaviorClass::ByteOffsetRoundTripNoDeref;
    if (m.hasByteMediatedReinterpretation)
      return WildPointerBehaviorClass::ByteMediatedReinterpretationNoDeref;
    if (m.hasSequentialConcreteReinterpretation)
      return WildPointerBehaviorClass::SequentialConcreteReinterpretationNoDeref;
    if (m.hasByteViewToConcrete)
      return WildPointerBehaviorClass::ByteViewToConcreteNoDeref;
  }

  if (wm.hasHigherOrderForwarding && !wm.hasConcreteDeref &&
      !callbackMediationStateIsResolved(callbackState)) {
    // More specific callback-dependent case:
    //   void *pvho;
    //   char *pcho = pvho;
    //   pf(pcho);
    //
    // The data pointer is callback-dependent, but the callback receives it
    // after a byte-oriented char* view.
    if (wm.hasByteView)
      return WildPointerBehaviorClass::CallbackDependentByteViewPointer;
    return WildPointerBehaviorClass::CallbackDependentPointer;
  }

  if (m.numNonVoidPointerTypes > 1 && !wm.hasConcreteDeref) {
    // A char*/byte view with no dereference is a representation/view change,
    // not an actual read.  Keep the name operation-neutral.
    if (wm.hasByteView)
      return WildPointerBehaviorClass::ByteViewNoDeref;
    return WildPointerBehaviorClass::MultiTypeNoDeref;
  }

  if (m.numNonVoidPointerTypes == 1) {
    // A single char*/byte view with no dereference is not concrete
    // specialization. It is only a byte-oriented view/conversion.
    //
    // Example:
    //   void *v = &arr[i];
    //   char *pc = v;
    //
    // should be ByteViewNoDeref, not ConcreteSpecialized.
    if (wm.hasByteView && !wm.hasConcreteDeref)
      return WildPointerBehaviorClass::ByteViewNoDeref;

    // Direction matters:
    //   int*  => void*  is type erasure into an opaque interface.
    //   void* => int*   is concrete specialization from an opaque interface.
    if (wm.numDerefTypes == 0 &&
        m.hasTypeErasureToVoid &&
        !m.hasConcreteSpecializationFromVoid)
      return WildPointerBehaviorClass::TypeErasedToVoidNoDeref;

    if (m.hasConcreteSpecializationFromVoid) {
      if (wm.hasConcreteWrite)
        return WildPointerBehaviorClass::ConcreteSpecializedWrite;
      if (wm.hasConcreteDeref)
        return WildPointerBehaviorClass::ConcreteSpecializedRead;
      return WildPointerBehaviorClass::ConcreteSpecialized;
    }

    if (wm.hasConcreteWrite)
      return WildPointerBehaviorClass::ConcreteSpecializedWrite;
    if (wm.hasConcreteDeref)
      return WildPointerBehaviorClass::ConcreteSpecializedRead;

    return WildPointerBehaviorClass::TypeErasedToVoidNoDeref;
  }

  if (wm.hasByteViewRead || (wm.hasByteView && !wm.hasConcreteDeref))
    return WildPointerBehaviorClass::ByteViewNoDeref;

  if (wm.hasVoidInterface && !wm.hasConcreteDeref)
    return WildPointerBehaviorClass::OpaqueForwarding;

  return WildPointerBehaviorClass::Unknown;
}


// C4R2: paper-facing wildness terminology.
//
// These fields intentionally do not replace or reinterpret the existing
// classifiers.  They are a compatibility-preserving vocabulary layer derived
// from the already-computed structural history risk and operation-aware behavior
// class:
//   * historyShape/historyRisk remain the structural cast-history view.
//   * behavior remains the precise operation-aware behavior class.
//   * wildLike remains the legacy trigger flag used by existing reports/tests.
//   * PaperWildnessSummary provides a compact paper-facing interpretation that
//     separates surface form from semantic behavior/risk.
struct PaperWildnessSummary {
  std::string surfaceKind = "unknown";
  std::set<std::string> surfaceReasons;
  std::string semanticFamily = "Unknown";
  std::string semanticRiskTier = "Unknown";
  std::string interpretation = "unknown";
};

static inline std::string paperSemanticFamilyName(WildPointerBehaviorClass c) {
  switch (c) {
  case WildPointerBehaviorClass::NotWildLike:
    return "NotWildLike";

  case WildPointerBehaviorClass::OpaqueForwarding:
  case WildPointerBehaviorClass::TypeErasedToVoidNoDeref:
    return "OpaqueTransport";

  case WildPointerBehaviorClass::ConcreteSpecialized:
  case WildPointerBehaviorClass::ConcreteSpecializedRead:
  case WildPointerBehaviorClass::ConcreteSpecializedWrite:
    return "ConcreteSpecialization";

  case WildPointerBehaviorClass::ByteViewNoDeref:
  case WildPointerBehaviorClass::ByteViewWrite:
    return "ByteView";

  case WildPointerBehaviorClass::ByteViewToConcreteNoDeref:
  case WildPointerBehaviorClass::ByteMediatedReinterpretationNoDeref:
  case WildPointerBehaviorClass::SequentialConcreteReinterpretationNoDeref:
  case WildPointerBehaviorClass::ByteOffsetRoundTripNoDeref:
  case WildPointerBehaviorClass::ByteOffsetToDifferentConcreteNoDeref:
    return "ByteMediatedReinterpretation";

  case WildPointerBehaviorClass::MultiTypeNoDeref:
  case WildPointerBehaviorClass::MultiTargetParallelNoDeref:
  case WildPointerBehaviorClass::ParallelSequentialReinterpretationNoDeref:
    return "MultiTargetReinterpretation";

  case WildPointerBehaviorClass::MultiTypeRead:
  case WildPointerBehaviorClass::MultiTypeWrite:
    return "MultiTypeAccess";

  case WildPointerBehaviorClass::BranchTypeSplitNoDeref:
  case WildPointerBehaviorClass::BranchTaggedUnionLike:
    return "BranchTaggedUnionLike";

  case WildPointerBehaviorClass::CallbackDependentPointer:
  case WildPointerBehaviorClass::CallbackDependentByteViewPointer:
    return "CallbackDependent";

  case WildPointerBehaviorClass::ExternalUnknown:
    return "ExternalUnknown";
  case WildPointerBehaviorClass::DangerousProvenance:
    return "DangerousProvenance";
  case WildPointerBehaviorClass::Unknown:
    return "Unknown";
  }
  return "Unknown";
}

static inline bool isHighHistoryRiskForPaper(HistoryRiskClass risk) {
  return risk == HistoryRiskClass::DangerousReinterpretation ||
         risk == HistoryRiskClass::DangerousProvenance ||
         risk == HistoryRiskClass::ExternalSink ||
         risk == HistoryRiskClass::BranchPolymorphicWrite ||
         risk == HistoryRiskClass::HigherOrderUnresolved;
}

static inline bool isModerateHistoryRiskForPaper(HistoryRiskClass risk) {
  return risk == HistoryRiskClass::Moderate ||
         risk == HistoryRiskClass::ByteViewRead ||
         risk == HistoryRiskClass::ByteViewWrite ||
         risk == HistoryRiskClass::SuspiciousReinterpretation ||
         risk == HistoryRiskClass::FunctionParameterDependent;
}

static inline bool isHighBehaviorForPaper(WildPointerBehaviorClass behavior) {
  return behavior == WildPointerBehaviorClass::DangerousProvenance ||
         behavior == WildPointerBehaviorClass::ExternalUnknown ||
         behavior == WildPointerBehaviorClass::MultiTypeRead ||
         behavior == WildPointerBehaviorClass::MultiTypeWrite;
}

static inline bool isModerateBehaviorForPaper(WildPointerBehaviorClass behavior) {
  switch (behavior) {
  case WildPointerBehaviorClass::ByteViewWrite:
  case WildPointerBehaviorClass::ByteViewToConcreteNoDeref:
  case WildPointerBehaviorClass::ByteMediatedReinterpretationNoDeref:
  case WildPointerBehaviorClass::SequentialConcreteReinterpretationNoDeref:
  case WildPointerBehaviorClass::ByteOffsetRoundTripNoDeref:
  case WildPointerBehaviorClass::ByteOffsetToDifferentConcreteNoDeref:
  case WildPointerBehaviorClass::MultiTypeNoDeref:
  case WildPointerBehaviorClass::MultiTargetParallelNoDeref:
  case WildPointerBehaviorClass::ParallelSequentialReinterpretationNoDeref:
  case WildPointerBehaviorClass::BranchTypeSplitNoDeref:
  case WildPointerBehaviorClass::BranchTaggedUnionLike:
  case WildPointerBehaviorClass::CallbackDependentPointer:
  case WildPointerBehaviorClass::CallbackDependentByteViewPointer:
    return true;
  default:
    return false;
  }
}

static inline std::string paperSemanticRiskTier(
    HistoryRiskClass risk, WildPointerBehaviorClass behavior) {
  if (isHighHistoryRiskForPaper(risk) || isHighBehaviorForPaper(behavior))
    return "High";
  if (isModerateHistoryRiskForPaper(risk) || isModerateBehaviorForPaper(behavior))
    return "Moderate";
  if (risk == HistoryRiskClass::Low ||
      behavior == WildPointerBehaviorClass::NotWildLike ||
      behavior == WildPointerBehaviorClass::OpaqueForwarding ||
      behavior == WildPointerBehaviorClass::TypeErasedToVoidNoDeref ||
      behavior == WildPointerBehaviorClass::ConcreteSpecialized ||
      behavior == WildPointerBehaviorClass::ConcreteSpecializedRead ||
      behavior == WildPointerBehaviorClass::ConcreteSpecializedWrite ||
      behavior == WildPointerBehaviorClass::ByteViewNoDeref)
    return "Low";
  return "Unknown";
}

static inline std::string paperSurfaceKind(
    const HistoryMetrics &m, const WildPointerBehaviorMetrics &wm) {
  if (m.hasIntToPtr || m.hasPtrToInt)
    return "integer-pointer-provenance";
  if (wm.hasExternalUnknown)
    return "external-unknown";
  if (wm.hasHigherOrderForwarding)
    return "higher-order-dependent";
  if (m.hasPointerArithmetic)
    return "pointer-arithmetic-view";
  if (wm.hasVoidInterface)
    return "void-interface";
  if (wm.hasCastLikeReinterpretation)
    return "cast-like-pointer-view";
  if (wm.hasByteView)
    return "byte-view";
  if (!wm.wildLike)
    return "not-wild-like";
  return "wild-like-other";
}

static inline std::set<std::string> paperSurfaceReasons(
    const HistoryMetrics &m, const WildPointerBehaviorMetrics &wm) {
  std::set<std::string> reasons;
  if (wm.hasVoidInterface) reasons.insert("void-interface");
  if (wm.hasCastLikeReinterpretation) reasons.insert("cast-like-reinterpretation");
  if (wm.hasByteView) reasons.insert("byte-view");
  if (m.hasPointerArithmetic) reasons.insert("pointer-arithmetic");
  if (wm.hasHigherOrderForwarding) reasons.insert("higher-order");
  if (wm.hasExternalUnknown) reasons.insert("external-unknown");
  if (m.hasIntToPtr || m.hasPtrToInt)
    reasons.insert("integer-pointer-provenance");
  if (reasons.empty())
    reasons.insert(wm.wildLike ? "wild-like-other" : "not-wild-like");
  return reasons;
}

static inline std::string paperWildnessInterpretation(
    const PaperWildnessSummary &summary,
    WildPointerBehaviorClass behavior) {
  if (summary.semanticFamily == "NotWildLike")
    return "ordinary/non-wild pointer behavior";
  if (summary.semanticFamily == "ConcreteSpecialization") {
    if (summary.semanticRiskTier == "Low")
      return "surface-wild interface with stable concrete specialization";
    return "surface-wild interface with concrete operation evidence and structurally mixed history";
  }
  if (summary.semanticFamily == "OpaqueTransport")
    return "surface-wild pointer transported or type-erased without concrete dereference";
  if (summary.semanticFamily == "ByteView")
    return behavior == WildPointerBehaviorClass::ByteViewWrite
               ? "byte-oriented view with write evidence"
               : "byte-oriented view without concrete dereference";
  if (summary.semanticFamily == "ByteMediatedReinterpretation")
    return "byte-mediated or pointer-arithmetic reinterpretation without direct concrete access evidence";
  if (summary.semanticFamily == "MultiTargetReinterpretation")
    return "multiple possible concrete target views without multi-type access evidence";
  if (summary.semanticFamily == "MultiTypeAccess")
    return "multiple concrete access types observed";
  if (summary.semanticFamily == "BranchTaggedUnionLike")
    return "branch/tag-structured pointer interpretation";
  if (summary.semanticFamily == "CallbackDependent")
    return "pointer behavior depends on callback or higher-order target";
  if (summary.semanticFamily == "DangerousProvenance")
    return "integer/pointer provenance conversion observed";
  if (summary.semanticFamily == "ExternalUnknown")
    return "pointer behavior escapes to external or unresolved sink";
  return "unknown pointer interpretation";
}

static inline PaperWildnessSummary computePaperWildnessSummary(
    const HistoryMetrics &m, const WildPointerBehaviorMetrics &wm,
    HistoryRiskClass risk, WildPointerBehaviorClass behavior) {
  PaperWildnessSummary summary;
  summary.surfaceKind = paperSurfaceKind(m, wm);
  summary.surfaceReasons = paperSurfaceReasons(m, wm);
  summary.semanticFamily = paperSemanticFamilyName(behavior);
  summary.semanticRiskTier = paperSemanticRiskTier(risk, behavior);
  summary.interpretation = paperWildnessInterpretation(summary, behavior);
  return summary;
}



static inline ViewCompletenessClass
classifyViewCompleteness(const HistoryMetrics &m,
                         const WildPointerBehaviorMetrics &wm,
                         bool isFunctionParameter = false,
                         bool isStorageLocation = false) {
  // Function parameters and storage locations are intended public/reportable
  // views because downstream summaries and field/storage histories are projected
  // onto them.
  if (isFunctionParameter)
    return ViewCompletenessClass::FullView;

  // Root-like/reportable view: the entity receives concrete operation evidence
  // such as dereference, write, call, return, higher-order, or external use.
  if (wm.hasDerefLikeUse || wm.hasCallOrReturnUse ||
      wm.hasHigherOrderForwarding || wm.hasExternalUnknown)
    return ViewCompletenessClass::FullView;

  // Storage locations with nontrivial branch/cast histories but no operations
  // are still useful report items, but their operational behavior is partial.
  if (isStorageLocation && (wm.hasBranchTypeSplit ||
                            m.numNonVoidPointerTypes > 1 ||
                            m.hasPointerArithmetic))
    return ViewCompletenessClass::PartialView;

  // Intermediate alias/view variables often carry cast-history structure but
  // not all downstream operations. Keep them in debug output, but label them as
  // partial so their behavior class is not overinterpreted.
  if (wm.hasBranchTypeSplit || m.numNonVoidPointerTypes > 1 ||
      m.hasPointerArithmetic || m.numCallWraps > 0)
    return ViewCompletenessClass::PartialView;

  if (wm.hasAnyPointerUse)
    return ViewCompletenessClass::LocalView;

  if (wm.wildLike)
    return ViewCompletenessClass::DebugOnlyView;

  return ViewCompletenessClass::Unknown;
}


static inline std::string joinStringSet(const std::set<std::string> &xs) {
  std::string out;
  bool first = true;
  for (const auto &x : xs) {
    if (!first)
      out += ", ";
    first = false;
    out += x;
  }
  return out.empty() ? std::string("<none>") : out;
}

} // namespace cast_history
