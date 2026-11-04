#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace cast_history {

// ============================================================================
// Shared source metadata
// ============================================================================

struct Guard {
  std::string text;
  std::string kind;
};

struct SourceSpan {
  std::string file;
  unsigned line = 0;
  unsigned column = 0;
};

enum class TypeKind {
  Unknown,
  Void,
  VoidPointer,
  CharPointer,
  OtherPointer,
  Array,
  Integer,
  FunctionPointer,
  RecordPointer,
  Floating,
  NullPointer,
  Other
};

struct TypeAtom {
  std::string text;

  // Structured classification. These fields are populated from Clang QualType
  // when available. The text field remains the human-readable rendering used by
  // printers; semantic decisions should prefer semanticKey()/compatibilityKey().
  TypeKind kind = TypeKind::Unknown;
  bool isVoid = false;
  bool isPointer = false;
  bool isVoidPointer = false;
  bool isCharPointer = false;
  bool isInteger = false;
  bool isArray = false;
  bool isFunctionPointer = false;
  bool isRecordPointer = false;
  bool isFloating = false;
  bool isNullPointer = false;

  bool isConstQualified = false;
  bool isVolatileQualified = false;
  bool pointeeIsConstQualified = false;
  bool pointeeIsVolatileQualified = false;

  std::string pointeeText;
  std::string pointeeCanonicalText;
  std::string canonicalText;

  std::string recordName;
  std::string recordTag;

  std::string functionReturnCanonicalText;
  std::vector<std::string> functionParamCanonicalTexts;
  bool functionHasPrototype = false;
  bool functionIsVariadic = false;

  bool hasStructuredInfo() const { return kind != TypeKind::Unknown; }

  static const char *kindName(TypeKind k) {
    switch (k) {
    case TypeKind::Unknown: return "unknown";
    case TypeKind::Void: return "void";
    case TypeKind::VoidPointer: return "void-pointer";
    case TypeKind::CharPointer: return "char-pointer";
    case TypeKind::OtherPointer: return "other-pointer";
    case TypeKind::Array: return "array";
    case TypeKind::Integer: return "integer";
    case TypeKind::FunctionPointer: return "function-pointer";
    case TypeKind::RecordPointer: return "record-pointer";
    case TypeKind::Floating: return "floating";
    case TypeKind::NullPointer: return "null-pointer";
    case TypeKind::Other: return "other";
    }
    return "unknown";
  }

  std::string displayText() const {
    return text.empty() ? canonicalText : text;
  }

  std::string semanticKey() const {
    if (!hasStructuredInfo())
      return "text:" + displayText();

    switch (kind) {
    case TypeKind::Void:
      return "void";
    case TypeKind::VoidPointer:
      return "ptr:void";
    case TypeKind::CharPointer:
      return "ptr:byte";
    case TypeKind::FunctionPointer:
      if (!functionReturnCanonicalText.empty() || !functionParamCanonicalTexts.empty()) {
        std::string out = "fnptr:(";
        for (size_t i = 0; i < functionParamCanonicalTexts.size(); ++i) {
          if (i)
            out += ",";
          out += functionParamCanonicalTexts[i];
        }
        if (functionIsVariadic)
          out += ",...";
        out += ")->";
        out += functionReturnCanonicalText.empty() ? pointeeCanonicalText
                                                   : functionReturnCanonicalText;
        return out;
      }
      return "fnptr:" + (!canonicalText.empty() ? canonicalText : displayText());
    case TypeKind::RecordPointer:
      if (!recordName.empty())
        return "ptr:record:" + recordTag + ":" + recordName;
      return "ptr:record:" + (!pointeeCanonicalText.empty()
                                  ? pointeeCanonicalText
                                  : (!canonicalText.empty() ? canonicalText
                                                            : displayText()));
    case TypeKind::OtherPointer:
      return "ptr:" + (!pointeeCanonicalText.empty()
                           ? pointeeCanonicalText
                           : (!canonicalText.empty() ? canonicalText
                                                     : displayText()));
    case TypeKind::Array:
      return "array:" + (!canonicalText.empty() ? canonicalText : displayText());
    case TypeKind::Integer:
      return "int:" + (!canonicalText.empty() ? canonicalText : displayText());
    case TypeKind::Floating:
      return "float:" + (!canonicalText.empty() ? canonicalText : displayText());
    case TypeKind::NullPointer:
      return "nullptr";
    case TypeKind::Other:
      return "other:" + (!canonicalText.empty() ? canonicalText : displayText());
    case TypeKind::Unknown:
      return "text:" + displayText();
    }
    return "text:" + displayText();
  }

  // For now, compatibility is semantic equality modulo display spelling.  This
  // gives us a single hook to relax qualifier/typedef distinctions later.
  std::string compatibilityKey() const {
    return semanticKey();
  }

  bool semanticallySame(const TypeAtom &other) const {
    return semanticKey() == other.semanticKey();
  }

  bool compatibleForCastHistory(const TypeAtom &other) const {
    return compatibilityKey() == other.compatibilityKey();
  }

  static std::string jsonEscape(const std::string &s) {
    std::ostringstream os;
    for (char ch : s) {
      switch (ch) {
      case '\\': os << "\\\\"; break;
      case '"': os << "\\\""; break;
      case '\n': os << "\\n"; break;
      case '\r': os << "\\r"; break;
      case '\t': os << "\\t"; break;
      default:
        os << ch;
        break;
      }
    }
    return os.str();
  }

  static std::string jsonString(const std::string &s) {
    return "\"" + jsonEscape(s) + "\"";
  }

  std::string toJsonString() const {
    std::ostringstream os;
    os << "{";
    os << "\"text\":" << jsonString(text);
    os << ",\"kind\":" << jsonString(kindName(kind));
    os << ",\"canonical\":" << jsonString(canonicalText);
    os << ",\"semanticKey\":" << jsonString(semanticKey());
    os << ",\"compatibilityKey\":" << jsonString(compatibilityKey());
    os << ",\"isVoid\":" << (isVoid ? "true" : "false");
    os << ",\"isPointer\":" << (isPointer ? "true" : "false");
    os << ",\"isVoidPointer\":" << (isVoidPointer ? "true" : "false");
    os << ",\"isCharPointer\":" << (isCharPointer ? "true" : "false");
    os << ",\"isFunctionPointer\":" << (isFunctionPointer ? "true" : "false");
    os << ",\"isRecordPointer\":" << (isRecordPointer ? "true" : "false");
    os << ",\"isInteger\":" << (isInteger ? "true" : "false");
    os << ",\"isArray\":" << (isArray ? "true" : "false");
    os << ",\"isFloating\":" << (isFloating ? "true" : "false");
    os << ",\"isNullPointer\":" << (isNullPointer ? "true" : "false");
    os << ",\"isConstQualified\":" << (isConstQualified ? "true" : "false");
    os << ",\"isVolatileQualified\":" << (isVolatileQualified ? "true" : "false");
    os << ",\"pointeeText\":" << jsonString(pointeeText);
    os << ",\"pointeeCanonical\":" << jsonString(pointeeCanonicalText);
    os << ",\"pointeeConst\":" << (pointeeIsConstQualified ? "true" : "false");
    os << ",\"pointeeVolatile\":" << (pointeeIsVolatileQualified ? "true" : "false");
    os << ",\"recordName\":" << jsonString(recordName);
    os << ",\"recordTag\":" << jsonString(recordTag);
    os << ",\"functionReturnCanonical\":" << jsonString(functionReturnCanonicalText);
    os << ",\"functionHasPrototype\":" << (functionHasPrototype ? "true" : "false");
    os << ",\"functionIsVariadic\":" << (functionIsVariadic ? "true" : "false");
    os << ",\"functionParamCanonical\":[";
    for (size_t i = 0; i < functionParamCanonicalTexts.size(); ++i) {
      if (i)
        os << ",";
      os << jsonString(functionParamCanonicalTexts[i]);
    }
    os << "]";
    os << "}";
    return os.str();
  }

  bool operator==(const TypeAtom &other) const { return semanticallySame(other); }
  bool operator!=(const TypeAtom &other) const { return !(*this == other); }
  bool operator<(const TypeAtom &other) const {
    return semanticKey() < other.semanticKey();
  }
};

// ============================================================================
// STEP 1 CORE: FlowHist
//
// Design goal linkage:
//   - Local sequential propagation of interpretations.
//   - Good for "what does this current variable/expression carry right now?"
//   - Supports meaningful-cast filtering and local branch joins.
//   - Kept separate from ObsHist so later enum / higher-order analyses do not
//     get polluted by purely local operational flow.
// ============================================================================

struct FlowHist;
using FlowHistPtr = std::shared_ptr<FlowHist>;

struct FlowBranchAlternative {
  FlowHistPtr history;
  std::optional<Guard> guard;
};

enum class FlowKind {
  Base,
  Step,
  Join,
  CallWrap,
  Unknown
};

struct FlowHist {
  FlowKind kind = FlowKind::Unknown;

  // Base / Step
  TypeAtom ty;
  FlowHistPtr prev;

  // Join
  std::vector<FlowBranchAlternative> alts;

  // CallWrap
  std::string callee;
  FlowHistPtr inner;

  // Unknown
  std::string note;
};

struct FlowRecord {
  std::string subject;
  FlowHistPtr history;
  std::string scope;
  SourceSpan span;
  std::string note;
};

static FlowHistPtr mkBase(const TypeAtom &ty) {
  auto h = std::make_shared<FlowHist>();
  h->kind = FlowKind::Base;
  h->ty = ty;
  return h;
}

static FlowHistPtr mkBase(const std::string &ty) {
  return mkBase(TypeAtom{ty});
}

static FlowHistPtr mkStep(FlowHistPtr prev, const TypeAtom &ty) {
  auto h = std::make_shared<FlowHist>();
  h->kind = FlowKind::Step;
  h->prev = std::move(prev);
  h->ty = ty;
  return h;
}

static FlowHistPtr mkStep(FlowHistPtr prev, const std::string &ty) {
  return mkStep(std::move(prev), TypeAtom{ty});
}

static FlowHistPtr mkJoin(std::vector<FlowBranchAlternative> alts) {
  auto h = std::make_shared<FlowHist>();
  h->kind = FlowKind::Join;
  h->alts = std::move(alts);
  return h;
}

static FlowHistPtr mkCallWrap(const std::string &callee, FlowHistPtr inner) {
  auto h = std::make_shared<FlowHist>();
  h->kind = FlowKind::CallWrap;
  h->callee = callee;
  h->inner = std::move(inner);
  return h;
}

static FlowHistPtr mkUnknown(const std::string &note) {
  auto h = std::make_shared<FlowHist>();
  h->kind = FlowKind::Unknown;
  h->note = note;
  return h;
}

class FlowFormatter {
public:
  static std::string render(const FlowHistPtr &h) {
    if (!h)
      return "<null>";

    switch (h->kind) {
    case FlowKind::Base:
      return h->ty.text;
    case FlowKind::Step: {
      std::string lhs = render(h->prev);
      if (lhs.empty())
        return h->ty.text;
      return lhs + " => " + h->ty.text;
    }
    case FlowKind::Join: {
      std::string out = "⟨";
      for (size_t i = 0; i < h->alts.size(); ++i) {
        if (i)
          out += ", ";
        const auto &alt = h->alts[i];
        if (alt.guard && !alt.guard->text.empty())
          out += "[" + alt.guard->text + "] ";
        out += render(alt.history);
      }
      out += "⟩";
      return out;
    }
    case FlowKind::CallWrap:
      return h->callee + "⌜" + render(h->inner) + "⌝" + h->callee;
    case FlowKind::Unknown:
      return "<unknown:" + h->note + ">";
    }
    return "<bad-flow>";
  }
};

// ============================================================================
// STEP 2A ADDITION: ObsHist
//
// Design goal linkage:
//   - Captures later interpretations of a source entity.
//   - Supports branch casts by attaching guarded observations to the *source*
//     entity rather than only to the new variable receiving a casted view.
//   - Supports higher-order summaries like $0.$0.
//   - Serves as the main input for future analyses:
//       * cast polymorphism
//       * enum detection
//       * ad-hoc polymorphism
//       * field-sensitive variant/tag reasoning
// ============================================================================

struct ObsHist;
using ObsHistPtr = std::shared_ptr<ObsHist>;

struct ObsAlt {
  Guard guard;
  ObsHistPtr hist;
};

enum class ObsKind {
  AsType,
  Join,
  ParamRef,
  ReturnRef,
  HOFParamRef,
  FieldRef,
  PtrArith, // pointer arithmetic / offset-derived pointer step
  Seq,
  Parallel, // parallel later interpretations that all occur as observations
  CallWrap, // observation caused by a direct callee, rendered f⌜...⌝f
  Unknown
};

struct ObsHist {
  ObsKind kind = ObsKind::Unknown;

  // AsType / PtrArith
  TypeAtom ty;

  // Join
  std::vector<ObsAlt> alts;

  // ParamRef
  unsigned paramIndex = 0;

  // HOFParamRef
  unsigned functionParamIndex = 0;
  unsigned argumentIndex = 0;

  // FieldRef
  ObsHistPtr base;
  std::vector<std::string> fieldPath;

  // Seq
  ObsHistPtr first;
  ObsHistPtr second;

  // Parallel
  std::vector<ObsHistPtr> arms;

  // CallWrap
  std::string callee;
  ObsHistPtr inner;

  // Unknown
  std::string note;
};

static ObsHistPtr mkObsAsType(const TypeAtom &ty) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::AsType;
  h->ty = ty;
  return h;
}

static ObsHistPtr mkObsAsType(const std::string &ty) {
  return mkObsAsType(TypeAtom{ty});
}

static ObsHistPtr mkObsPtrArith(const TypeAtom &pointerType) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::PtrArith;
  h->ty = pointerType;
  return h;
}

static ObsHistPtr mkObsPtrArith(const std::string &pointerType) {
  return mkObsPtrArith(TypeAtom{pointerType});
}

static ObsHistPtr mkObsJoin(std::vector<ObsAlt> alts) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::Join;
  h->alts = std::move(alts);
  return h;
}

static ObsHistPtr mkObsParamRef(unsigned i) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::ParamRef;
  h->paramIndex = i;
  return h;
}

static ObsHistPtr mkObsReturnRef() {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::ReturnRef;
  return h;
}

static ObsHistPtr mkObsHOFParamRef(unsigned fnParam, unsigned argIdx) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::HOFParamRef;
  h->functionParamIndex = fnParam;
  h->argumentIndex = argIdx;
  return h;
}

static ObsHistPtr mkObsFieldRef(ObsHistPtr base, std::vector<std::string> path) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::FieldRef;
  h->base = std::move(base);
  h->fieldPath = std::move(path);
  return h;
}

static ObsHistPtr mkObsSeq(ObsHistPtr first, ObsHistPtr second) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::Seq;
  h->first = std::move(first);
  h->second = std::move(second);
  return h;
}

static ObsHistPtr mkObsParallel(std::vector<ObsHistPtr> arms) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::Parallel;
  h->arms = std::move(arms);
  return h;
}

static ObsHistPtr mkObsCallWrap(const std::string &callee, ObsHistPtr inner) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::CallWrap;
  h->callee = callee;
  h->inner = std::move(inner);
  return h;
}

static ObsHistPtr mkObsUnknown(const std::string &note) {
  auto h = std::make_shared<ObsHist>();
  h->kind = ObsKind::Unknown;
  h->note = note;
  return h;
}

// C6g5d1: terminal widening evidence for one named-call edge captured while
// traversing an if condition inside a same-TU recursive SCC.  This remains an
// opaque Unknown leaf for all existing consumers; the prefix only lets the
// same-TU solver keep it local to the defining function and render it clearly.
inline constexpr const char *ObsRecursiveConditionTokenPrefix = "rec-edge:";

static ObsHistPtr mkObsRecursiveConditionToken(const std::string &token) {
  return mkObsUnknown(std::string(ObsRecursiveConditionTokenPrefix) + token);
}

static bool isObsRecursiveConditionToken(const ObsHistPtr &h,
                                         const std::string &token) {
  return h && h->kind == ObsKind::Unknown &&
         h->note == std::string(ObsRecursiveConditionTokenPrefix) + token;
}

static bool isAnyObsRecursiveConditionToken(const ObsHistPtr &h) {
  return h && h->kind == ObsKind::Unknown &&
         h->note.rfind(ObsRecursiveConditionTokenPrefix, 0) == 0;
}

// C6g5c: lightweight, thread-local render counters used only by the
// opt-in named-call watchdog.  The pointer is normally null, so normal output
// and summary construction pay only one null check per formatter recursion.
struct ObsRenderInstrumentation {
  uint64_t calls = 0;
  uint64_t depth = 0;
  uint64_t maxDepth = 0;
};

inline thread_local ObsRenderInstrumentation *ActiveObsRenderInstrumentation =
    nullptr;
// Optional callback owned by the visitor-level watchdog.  This keeps HistoryIR
// independent of Clang/visitor headers while allowing a deeply recursive
// formatter call to trigger a bounded cooperative checkpoint.
inline thread_local void *ActiveObsRenderCheckpointContext = nullptr;
inline thread_local void (*ActiveObsRenderCheckpoint)(void *) = nullptr;
inline thread_local uint64_t ActiveObsRenderCheckpointStride = 0;

class ObsFormatter {
public:
  static std::string render(const ObsHistPtr &h) {
    struct RenderScope {
      ObsRenderInstrumentation *state = ActiveObsRenderInstrumentation;
      RenderScope() {
        if (!state)
          return;
        ++state->calls;
        ++state->depth;
        state->maxDepth = std::max(state->maxDepth, state->depth);
        if (ActiveObsRenderCheckpoint &&
            ActiveObsRenderCheckpointStride != 0 &&
            (state->calls % ActiveObsRenderCheckpointStride) == 0)
          ActiveObsRenderCheckpoint(ActiveObsRenderCheckpointContext);
      }
      ~RenderScope() {
        if (state && state->depth != 0)
          --state->depth;
      }
    } renderScope;
    if (!h)
      return "<null>";

    switch (h->kind) {
    case ObsKind::AsType:
      return h->ty.text;

    case ObsKind::PtrArith:
      return "ptrarith(" + h->ty.text + ")";

    case ObsKind::Join: {
      std::string out = "⟨";
      for (size_t i = 0; i < h->alts.size(); ++i) {
        if (i)
          out += ", ";
        out += "[" + h->alts[i].guard.text + "] ";
        out += render(h->alts[i].hist);
      }
      out += "⟩";
      return out;
    }

    case ObsKind::ParamRef:
      return "$" + std::to_string(h->paramIndex);

    case ObsKind::ReturnRef:
      return "ret";

    case ObsKind::HOFParamRef:
      return "$" + std::to_string(h->functionParamIndex) + ".$" +
             std::to_string(h->argumentIndex);

    case ObsKind::FieldRef: {
      std::string out = render(h->base);
      for (const auto &f : h->fieldPath)
        out += "." + f;
      return out;
    }

    case ObsKind::Seq:
      return render(h->first) + " => " + render(h->second);

    case ObsKind::Parallel: {
      std::string out = "⟨| ";
      for (size_t i = 0; i < h->arms.size(); ++i) {
        if (i) out += ", ";
        out += render(h->arms[i]);
      }
      out += " |⟩";
      return out;
    }

    case ObsKind::CallWrap:
      return h->callee + "⌜" + render(h->inner) + "⌝" + h->callee;

    case ObsKind::Unknown:
      if (h->note.rfind(ObsRecursiveConditionTokenPrefix, 0) == 0) {
        return "rec{" + h->note.substr(
            std::char_traits<char>::length(ObsRecursiveConditionTokenPrefix)) +
               "}";
      }
      return "<unknown:" + h->note + ">";
    }
    return "<bad-obs>";
  }
};

} // namespace cast_history
