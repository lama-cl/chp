#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/JSON.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cast_history {

// C6g4w1: source-level pointer/cast prevalence metrics.
//
// This recorder is intentionally observational. It is populated while the AST
// is traversed, but it never participates in cast-history normalization,
// pointer-use membership, call-summary solving, or progress/fixpoint state.
// The emitted JSON contains raw, source-located rows so a project-level Python
// aggregator can deduplicate shared headers across translation units.
class PrevalenceMetrics {
public:
  PrevalenceMetrics(clang::SourceManager &sourceManager, bool enabled,
                    std::string projectRoot, bool includeReturnTypes,
                    std::string projectId)
      : sourceManager(sourceManager), enabled(enabled),
        projectRoot(normalizePath(std::move(projectRoot))),
        includeReturnTypes(includeReturnTypes), projectId(std::move(projectId)) {}

  bool isEnabled() const { return enabled; }

  void recordVarDecl(const clang::VarDecl *decl) {
    if (!enabled || !decl)
      return;

    const clang::QualType type = decl->getType();
    if (!isOuterPointer(type))
      return;

    std::string declarationKind = "other";
    if (const auto *param = llvm::dyn_cast<clang::ParmVarDecl>(decl)) {
      // A definition and one or more prototypes have distinct ParmVarDecl
      // source locations, so source-location de-duplication alone counts the
      // same logical parameter repeatedly. Prefer the parameter attached to
      // the defining FunctionDecl; if no definition is present, retain the
      // canonical declaration as the one logical API parameter. This does not
      // affect FieldDecl owners, including function-pointer struct fields.
      if (!shouldRecordParameterDecl(param))
        return;
      declarationKind = "parameter";
    } else if (decl->isLocalVarDecl()) {
      declarationKind = "local";
    } else if (decl->hasGlobalStorage()) {
      declarationKind = "global";
    }

    recordPointerDeclaration(decl->getLocation(), declarationKind,
                             decl->getQualifiedNameAsString(), type,
                             /*isPointerField=*/false);
  }

  void recordFieldDecl(const clang::FieldDecl *decl) {
    if (!enabled || !decl || !isOuterPointer(decl->getType()))
      return;

    recordPointerDeclaration(decl->getLocation(), "field",
                             decl->getQualifiedNameAsString(), decl->getType(),
                             /*isPointerField=*/true);
  }

  // Return types are deliberately opt-in. When enabled, count only function
  // definitions, not every prototype/redeclaration, so a function contributes
  // at most one return declaration from its definition location per TU.
  void recordFunctionReturn(const clang::FunctionDecl *decl) {
    if (!enabled || !includeReturnTypes || !decl ||
        !decl->isThisDeclarationADefinition() ||
        !isOuterPointer(decl->getReturnType()))
      return;

    recordPointerDeclaration(decl->getLocation(), "return",
                             decl->getQualifiedNameAsString(),
                             decl->getReturnType(),
                             /*isPointerField=*/false);
  }

  void recordCast(const clang::CastExpr *cast) {
    if (!enabled || !cast)
      return;

    const auto location = projectLocation(cast->getExprLoc());
    if (!location)
      return;

    const bool implicit = llvm::isa<clang::ImplicitCastExpr>(cast);
    const clang::QualType sourceType = cast->getSubExpr()->getType();
    const clang::QualType destinationType = cast->getType();
    const bool sourcePointer = isOuterPointer(sourceType);
    const bool destinationPointer = isOuterPointer(destinationType);
    const bool sourceVoidPointer = isOuterVoidPointer(sourceType);
    const bool destinationVoidPointer = isOuterVoidPointer(destinationType);

    CastRecord record;
    record.source = *location;
    record.castKind = cast->getStmtClassName();
    record.isImplicit = implicit;
    record.sourceType = typeText(sourceType);
    record.sourceCanonicalType = canonicalTypeText(sourceType);
    record.destinationType = typeText(destinationType);
    record.destinationCanonicalType = canonicalTypeText(destinationType);
    record.sourceIsPointer = sourcePointer;
    record.destinationIsPointer = destinationPointer;

    // The void-boundary columns intentionally count only explicit casts and
    // only a pointer-to-pointer crossing. This excludes a no-op void* -> void*
    // cast and does not conflate integer conversions with a void-pointer API.
    record.toVoidPointer = !implicit && sourcePointer && destinationPointer &&
                           !sourceVoidPointer && destinationVoidPointer;
    record.fromVoidPointer = !implicit && sourcePointer && destinationPointer &&
                             sourceVoidPointer && !destinationVoidPointer;
    record.bytePointerCast = !implicit &&
                             (isBytePointer(sourceType) ||
                              isBytePointer(destinationType));

    record.id = "cast:" + record.source.file + ":" +
                std::to_string(record.source.line) + ":" +
                std::to_string(record.source.column) + ":" +
                record.castKind + ":" +
                (record.isImplicit ? "implicit" : "explicit");
    if (!seenCastIds.insert(record.id).second)
      return;

    casts.push_back(std::move(record));
  }

  llvm::json::Object emitJsonObject(llvm::StringRef sourceFile,
                                    llvm::StringRef configurationId,
                                    llvm::StringRef compileCommandHash) const {
    llvm::json::Array declarationJson;
    for (const PointerDeclarationRecord &record : pointerDeclarations) {
      declarationJson.push_back(llvm::json::Object{
          {"id", record.id},
          {"source", sourceJson(record.source)},
          {"declarationKind", record.declarationKind},
          {"name", record.name},
          {"type", record.type},
          {"canonicalType", record.canonicalType},
          {"isVoidPointer", record.isVoidPointer},
          {"isFunctionPointer", record.isFunctionPointer},
          {"isPointerField", record.isPointerField},
      });
    }

    llvm::json::Array functionTypePositionJson;
    for (const FunctionTypeVoidPointerPositionRecord &record :
         functionTypeVoidPointerPositions) {
      functionTypePositionJson.push_back(llvm::json::Object{
          {"id", record.id},
          {"ownerDeclarationId", record.ownerDeclarationId},
          {"source", sourceJson(record.source)},
          {"ownerDeclarationKind", record.ownerDeclarationKind},
          {"ownerName", record.ownerName},
          {"positionKind", record.positionKind},
          {"positionPath", record.positionPath},
          {"type", record.type},
          {"canonicalType", record.canonicalType},
      });
    }

    llvm::json::Array castJson;
    for (const CastRecord &record : casts) {
      castJson.push_back(llvm::json::Object{
          {"id", record.id},
          {"source", sourceJson(record.source)},
          {"castKind", record.castKind},
          {"isImplicit", record.isImplicit},
          {"sourceType", record.sourceType},
          {"sourceCanonicalType", record.sourceCanonicalType},
          {"destinationType", record.destinationType},
          {"destinationCanonicalType", record.destinationCanonicalType},
          {"sourceIsPointer", record.sourceIsPointer},
          {"destinationIsPointer", record.destinationIsPointer},
          {"toVoidPointer", record.toVoidPointer},
          {"fromVoidPointer", record.fromVoidPointer},
          {"bytePointerCast", record.bytePointerCast},
      });
    }

    const Counts counts = currentCounts();
    return llvm::json::Object{
        {"schemaName", "cast-history-prevalence-metrics"},
        {"schemaVersion", static_cast<int64_t>(2)},
        {"toolVersion", "cast-history-dev"},
        {"project",
         llvm::json::Object{{"id", projectId},
                            {"root", projectRoot},
                            {"includeReturnTypes", includeReturnTypes}}},
        {"tu",
         llvm::json::Object{{"sourceFile", sourceFile},
                            {"configurationId", configurationId},
                            {"compileCommandHash", compileCommandHash}}},
        {"pointerDeclarations", std::move(declarationJson)},
        {"functionTypeVoidPointerPositions",
         std::move(functionTypePositionJson)},
        {"casts", std::move(castJson)},
        {"tuCounts", countsJson(counts)},
    };
  }

private:
  struct MetricLocation {
    std::string file;
    unsigned line = 0;
    unsigned column = 0;
  };

  struct PointerDeclarationRecord {
    std::string id;
    MetricLocation source;
    std::string declarationKind;
    std::string name;
    std::string type;
    std::string canonicalType;
    bool isVoidPointer = false;
    bool isFunctionPointer = false;
    bool isPointerField = false;
  };

  struct FunctionTypeVoidPointerPositionRecord {
    std::string id;
    std::string ownerDeclarationId;
    MetricLocation source;
    std::string ownerDeclarationKind;
    std::string ownerName;
    std::string positionKind;
    std::string positionPath;
    std::string type;
    std::string canonicalType;
  };

  struct CastRecord {
    std::string id;
    MetricLocation source;
    std::string castKind;
    bool isImplicit = false;
    std::string sourceType;
    std::string sourceCanonicalType;
    std::string destinationType;
    std::string destinationCanonicalType;
    bool sourceIsPointer = false;
    bool destinationIsPointer = false;
    bool toVoidPointer = false;
    bool fromVoidPointer = false;
    bool bytePointerCast = false;
  };

  struct Counts {
    uint64_t pointerDeclarations = 0;
    uint64_t voidPointerDeclarations = 0;
    uint64_t voidPointerGlobalDeclarations = 0;
    uint64_t voidPointerFieldDeclarations = 0;
    uint64_t voidPointerLocalDeclarations = 0;
    uint64_t voidPointerParameterDeclarations = 0;
    uint64_t voidPointerReturnDeclarations = 0;
    uint64_t functionTypeVoidPointerPositions = 0;
    uint64_t functionPointerDeclarations = 0;
    uint64_t pointerFieldDeclarations = 0;
    uint64_t returnPointerDeclarations = 0;
    uint64_t explicitCasts = 0;
    uint64_t implicitCasts = 0;
    uint64_t toVoidPointerCasts = 0;
    uint64_t fromVoidPointerCasts = 0;
    uint64_t bytePointerCasts = 0;
  };

  clang::SourceManager &sourceManager;
  bool enabled = false;
  std::string projectRoot;
  bool includeReturnTypes = false;
  std::string projectId;
  std::vector<PointerDeclarationRecord> pointerDeclarations;
  std::vector<FunctionTypeVoidPointerPositionRecord>
      functionTypeVoidPointerPositions;
  std::vector<CastRecord> casts;
  std::set<std::string> seenDeclarationIds;
  std::set<std::string> seenFunctionTypeVoidPointerPositionIds;
  std::set<std::string> seenCastIds;

  static std::optional<unsigned>
  parameterIndexInOwner(const clang::ParmVarDecl *param,
                        const clang::FunctionDecl *owner) {
    if (!param || !owner)
      return std::nullopt;
    for (unsigned index = 0; index < owner->getNumParams(); ++index) {
      if (owner->getParamDecl(index) == param)
        return index;
    }
    return std::nullopt;
  }

  static bool shouldRecordParameterDecl(const clang::ParmVarDecl *param) {
    if (!param)
      return false;
    const auto *owner = llvm::dyn_cast<clang::FunctionDecl>(
        param->getDeclContext());
    if (!owner)
      return true;

    const clang::FunctionDecl *preferred = owner->getDefinition();
    if (!preferred)
      preferred = owner->getCanonicalDecl();
    if (!preferred)
      preferred = owner;

    const std::optional<unsigned> index = parameterIndexInOwner(param, owner);
    return index && *index < preferred->getNumParams() &&
           preferred->getParamDecl(*index) == param;
  }

  static std::string normalizePath(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    while (value.size() > 1 && value.back() == '/')
      value.pop_back();
    return value;
  }

  bool isProjectOwnedFile(llvm::StringRef file) const {
    if (projectRoot.empty())
      return false;
    const std::string normalized = normalizePath(file.str());
    if (normalized == projectRoot)
      return true;
    return normalized.size() > projectRoot.size() &&
           normalized.compare(0, projectRoot.size(), projectRoot) == 0 &&
           normalized[projectRoot.size()] == '/';
  }

  std::optional<MetricLocation>
  projectLocation(clang::SourceLocation location) const {
    if (location.isInvalid())
      return std::nullopt;
    const clang::SourceLocation spelling = sourceManager.getSpellingLoc(location);
    if (spelling.isInvalid())
      return std::nullopt;

    llvm::StringRef rawFile = sourceManager.getFilename(spelling);
    if (rawFile.empty() || !isProjectOwnedFile(rawFile))
      return std::nullopt;

    MetricLocation out;
    out.file = normalizePath(rawFile.str());
    out.line = sourceManager.getSpellingLineNumber(spelling);
    out.column = sourceManager.getSpellingColumnNumber(spelling);
    if (out.line == 0 || out.column == 0)
      return std::nullopt;
    return out;
  }

  static bool isOuterPointer(clang::QualType type) {
    if (type.isNull())
      return false;
    return type.getCanonicalType()->isPointerType();
  }

  static bool isOuterVoidPointer(clang::QualType type) {
    if (!isOuterPointer(type))
      return false;
    clang::QualType pointee = type.getCanonicalType()->getPointeeType();
    if (pointee.isNull())
      return false;
    return pointee.getCanonicalType().getUnqualifiedType()->isVoidType();
  }

  static bool isFunctionPointer(clang::QualType type) {
    if (!isOuterPointer(type))
      return false;
    clang::QualType pointee = type.getCanonicalType()->getPointeeType();
    return !pointee.isNull() && pointee->isFunctionType();
  }

  static bool isBytePointee(clang::QualType type) {
    if (type.isNull())
      return false;
    const clang::QualType canonical =
        type.getCanonicalType().getUnqualifiedType();

    if (const auto *builtin = canonical->getAs<clang::BuiltinType>()) {
      switch (builtin->getKind()) {
      case clang::BuiltinType::Char_S:
      case clang::BuiltinType::Char_U:
      case clang::BuiltinType::SChar:
      case clang::BuiltinType::UChar:
        return true;
      default:
        break;
      }
    }

    if (const auto *enumType = canonical->getAs<clang::EnumType>()) {
      const clang::EnumDecl *decl = enumType->getDecl();
      return decl && decl->getQualifiedNameAsString() == "std::byte";
    }
    return false;
  }

  static bool isBytePointer(clang::QualType type) {
    if (!isOuterPointer(type))
      return false;
    return isBytePointee(type.getCanonicalType()->getPointeeType());
  }

  static std::string typeText(clang::QualType type) {
    return type.isNull() ? std::string() : type.getAsString();
  }

  static std::string canonicalTypeText(clang::QualType type) {
    return type.isNull() ? std::string() : type.getCanonicalType().getAsString();
  }

  static llvm::json::Object sourceJson(const MetricLocation &location) {
    return llvm::json::Object{{"file", location.file},
                              {"line", static_cast<int64_t>(location.line)},
                              {"column", static_cast<int64_t>(location.column)}};
  }

  static llvm::json::Object countsJson(const Counts &counts) {
    return llvm::json::Object{
        {"pointerDeclarations", static_cast<int64_t>(counts.pointerDeclarations)},
        {"voidPointerDeclarations", static_cast<int64_t>(counts.voidPointerDeclarations)},
        {"voidPointerGlobalDeclarations",
         static_cast<int64_t>(counts.voidPointerGlobalDeclarations)},
        {"voidPointerFieldDeclarations",
         static_cast<int64_t>(counts.voidPointerFieldDeclarations)},
        {"voidPointerLocalDeclarations",
         static_cast<int64_t>(counts.voidPointerLocalDeclarations)},
        {"voidPointerParameterDeclarations",
         static_cast<int64_t>(counts.voidPointerParameterDeclarations)},
        {"voidPointerReturnDeclarations",
         static_cast<int64_t>(counts.voidPointerReturnDeclarations)},
        {"functionTypeVoidPointerPositions",
         static_cast<int64_t>(counts.functionTypeVoidPointerPositions)},
        {"functionPointerDeclarations", static_cast<int64_t>(counts.functionPointerDeclarations)},
        {"pointerFieldDeclarations", static_cast<int64_t>(counts.pointerFieldDeclarations)},
        {"returnPointerDeclarations", static_cast<int64_t>(counts.returnPointerDeclarations)},
        {"explicitCasts", static_cast<int64_t>(counts.explicitCasts)},
        {"implicitCasts", static_cast<int64_t>(counts.implicitCasts)},
        {"toVoidPointerCasts", static_cast<int64_t>(counts.toVoidPointerCasts)},
        {"fromVoidPointerCasts", static_cast<int64_t>(counts.fromVoidPointerCasts)},
        {"bytePointerCasts", static_cast<int64_t>(counts.bytePointerCasts)},
    };
  }

  Counts currentCounts() const {
    Counts counts;
    counts.pointerDeclarations = pointerDeclarations.size();
    for (const PointerDeclarationRecord &record : pointerDeclarations) {
      counts.voidPointerDeclarations += record.isVoidPointer;
      if (record.isVoidPointer) {
        if (record.declarationKind == "global")
          ++counts.voidPointerGlobalDeclarations;
        else if (record.declarationKind == "field")
          ++counts.voidPointerFieldDeclarations;
        else if (record.declarationKind == "local")
          ++counts.voidPointerLocalDeclarations;
        else if (record.declarationKind == "parameter")
          ++counts.voidPointerParameterDeclarations;
        else if (record.declarationKind == "return")
          ++counts.voidPointerReturnDeclarations;
      }
      counts.functionPointerDeclarations += record.isFunctionPointer;
      counts.pointerFieldDeclarations += record.isPointerField;
      counts.returnPointerDeclarations +=
          record.declarationKind == "return";
    }
    counts.functionTypeVoidPointerPositions =
        functionTypeVoidPointerPositions.size();
    for (const CastRecord &record : casts) {
      if (record.isImplicit)
        ++counts.implicitCasts;
      else
        ++counts.explicitCasts;
      counts.toVoidPointerCasts += record.toVoidPointer;
      counts.fromVoidPointerCasts += record.fromVoidPointer;
      counts.bytePointerCasts += record.bytePointerCast;
    }
    return counts;
  }

  void recordPointerDeclaration(clang::SourceLocation declarationLocation,
                                llvm::StringRef declarationKind,
                                llvm::StringRef name, clang::QualType type,
                                bool isPointerField) {
    const auto location = projectLocation(declarationLocation);
    if (!location)
      return;

    PointerDeclarationRecord record;
    record.source = *location;
    record.declarationKind = declarationKind.str();
    record.name = name.str();
    record.type = typeText(type);
    record.canonicalType = canonicalTypeText(type);
    record.isVoidPointer = isOuterVoidPointer(type);
    record.isFunctionPointer = isFunctionPointer(type);
    record.isPointerField = isPointerField;
    record.id = "pointer-decl:" + record.source.file + ":" +
                std::to_string(record.source.line) + ":" +
                std::to_string(record.source.column) + ":" +
                record.declarationKind;
    if (!seenDeclarationIds.insert(record.id).second)
      return;

    recordFunctionTypeVoidPointerPositions(record, type);
    pointerDeclarations.push_back(std::move(record));
  }

  void recordFunctionTypeVoidPointerPosition(
      const PointerDeclarationRecord &owner, llvm::StringRef positionKind,
      llvm::StringRef positionPath, clang::QualType type) {
    if (!isOuterVoidPointer(type))
      return;

    FunctionTypeVoidPointerPositionRecord record;
    record.ownerDeclarationId = owner.id;
    record.source = owner.source;
    record.ownerDeclarationKind = owner.declarationKind;
    record.ownerName = owner.name;
    record.positionKind = positionKind.str();
    record.positionPath = positionPath.str();
    record.type = typeText(type);
    record.canonicalType = canonicalTypeText(type);
    record.id = "function-type-void-pointer:" + owner.id + ":" +
                record.positionKind + ":" + record.positionPath;
    if (!seenFunctionTypeVoidPointerPositionIds.insert(record.id).second)
      return;
    functionTypeVoidPointerPositions.push_back(std::move(record));
  }

  void recordFunctionTypeVoidPointerPositions(
      const PointerDeclarationRecord &owner, clang::QualType type,
      llvm::StringRef path = "type") {
    if (type.isNull())
      return;
    const clang::QualType canonical = type.getCanonicalType();
    if (!canonical->isPointerType())
      return;
    const clang::QualType pointee = canonical->getPointeeType();
    if (pointee.isNull())
      return;

    if (const auto *proto = pointee->getAs<clang::FunctionProtoType>()) {
      const std::string returnPath = path.str() + ".return";
      recordFunctionTypeVoidPointerPosition(
          owner, "function-return", returnPath, proto->getReturnType());
      recordFunctionTypeVoidPointerPositions(owner, proto->getReturnType(),
                                             returnPath);
      unsigned index = 0;
      for (clang::QualType param : proto->param_types()) {
        const std::string paramPath =
            path.str() + ".param[" + std::to_string(index++) + "]";
        recordFunctionTypeVoidPointerPosition(
            owner, "function-parameter", paramPath, param);
        recordFunctionTypeVoidPointerPositions(owner, param, paramPath);
      }
      return;
    }
    // K&R/no-prototype function types do not expose parameter types, but their
    // return position is still an explicit nested function-type position.
    if (const auto *function = pointee->getAs<clang::FunctionType>()) {
      const std::string returnPath = path.str() + ".return";
      recordFunctionTypeVoidPointerPosition(
          owner, "function-return", returnPath, function->getReturnType());
      recordFunctionTypeVoidPointerPositions(owner, function->getReturnType(),
                                             returnPath);
      return;
    }

    // A pointer-to-pointer-to-function is still a function-type position of
    // the owning declaration. Descend through ordinary pointer layers only;
    // direct outer void pointers are intentionally excluded from this nested
    // inventory and remain in pointerDeclarations.
    recordFunctionTypeVoidPointerPositions(owner, pointee,
                                           path.str() + ".pointee");
  }
};

} // namespace cast_history
