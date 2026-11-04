#pragma once

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cast_history {

// C3a-2: project summary index reader.
//
// This class is deliberately independent of CastHistoryVisitor.  It consumes
// the C2 project-summary-index.json file and provides lookup operations that a
// later C3a patch can call from VisitCallExpr.  This patch only loads/validates
// the index and exposes query helpers; it does not change cast-history
// semantics yet.
class SummaryIndex {
public:
  struct Location {
    std::string file;
    int64_t line = 0;
    int64_t column = 0;
  };

  struct FunctionEntry {
    struct SolvedParamSummaryEntry {
      struct ForkArmProvenanceEntry {
        std::string forkId;
        std::string armId;
        std::string origin;
        std::string rendered;
      };

      unsigned index = 0;
      std::string rendered;
      std::string irJson;
      std::string source;
      // C6g5t: bounded simple-path materialization retains the root parallel
      // fork separately from the semantic IR.  These entries are report-only:
      // they never participate in index lookup or solver identity.
      std::vector<ForkArmProvenanceEntry> forkArmProvenance;
    };

    struct SolvedReturnSummaryEntry {
      std::string rendered;
      std::string irJson;
      std::string source;
      bool mayAliasParam = false;
      std::optional<unsigned> aliasedParamIndex;
    };

    struct SolvedPointerUseSummaryEntry {
      unsigned index = 0;
      std::string source;
      std::string useJson;
    };

    std::string function;
    std::string name;
    std::string qualifiedName;
    std::string signatureKey;
    std::string linkage;
    std::string sourceFile;
    Location location;
    std::string summaryFile;
    std::string tuSourceFile;
    std::string functionBehavior;
    std::string lookupKey;
    std::string publicLookupKey;
    std::string status;
    std::string resolution;
    // Import-authoritative project summaries.  Existing import machinery uses
    // this field exactly as before.
    std::vector<SolvedParamSummaryEntry> solvedParamSummaries;

    // C6g5v display-only canonical histories.  These preserve direct-local
    // fork/arm correlation for reporting but are intentionally never consumed
    // by applySolvedParamSummariesFromIndex().
    std::vector<SolvedParamSummaryEntry> canonicalProjectParamHistories;

    std::optional<SolvedReturnSummaryEntry> solvedReturnSummary;
    std::vector<SolvedPointerUseSummaryEntry> solvedPointerUseSummaries;

    bool hasSolvedParamSummaries() const {
      return !solvedParamSummaries.empty();
    }

    bool hasCanonicalProjectParamHistories() const {
      return !canonicalProjectParamHistories.empty();
    }

    bool hasSolvedReturnSummary() const {
      return solvedReturnSummary.has_value() &&
             !solvedReturnSummary->irJson.empty();
    }

    bool hasSolvedPointerUseSummaries() const {
      return !solvedPointerUseSummaries.empty();
    }

    bool isSafeForAcyclicDirectImport() const {
      // C3a is intentionally conservative.  Source-scoped/internal lookup is
      // deferred until the caller-side source context is wired into the query.
      if (resolution == "safe" || resolution == "safe-with-dedup")
        return true;
      if (resolution.empty() &&
          (status == "unambiguous" ||
           status == "repeated-same-definition-summary"))
        return true;
      return false;
    }
  };

  // C6g5w1d: transport record for source-defined branch families carried on
  // ordinary summaryParamTransferEdges.  C2 may consume recursiveClosureKey
  // during its narrow tagged recursive materialization; this C++ reader still
  // does not alter lookup, C1 SCC membership, or live import composition.
  struct BranchFamilyTransferMetadataRecord {
    std::string familyId;
    std::string familyKind;
    std::string armId;
    std::string armKind;
    std::string ownerNode;
    unsigned ownerParamIndex = 0;
    std::string origin;
    // C6g5w1c: source-family provenance remains in familyId.  This immutable
    // diagnostics-only key can instead identify a semantically repeated
    // guarded transformer on a future recursive-composition path.
    std::string recursiveClosureKey;
    bool consumedByTaggedRecursiveSolver = false;
    std::string actualPath;
    // C6g5w1b: guard intrinsic to an individual branch arm.  This is
    // separate from transferGuard, which describes reachability of the call
    // edge itself.  Both remain diagnostics-only.
    std::optional<std::string> armGuard;
    std::string originSourceEntity;
    std::string callerNode;
    std::string calleeNode;
    unsigned callerParamIndex = 0;
    unsigned calleeParamIndex = 0;
    std::string edgeKey;
    std::string summaryFile;
    std::string tuSourceFile;
    // C6g5w1a: provenance of the diagnostics record itself.  The raw transfer
    // edge channel is still preferred; the project-level companion summary is
    // an import-side rehydration fallback and cross-check only.
    std::string transportChannel;
    bool rehydratedFromProjectMetadata = false;
    std::optional<std::string> transferGuard;
    Location location;
  };

  struct CallProvenanceRecord {
    std::string kind;
    std::string callerNode;
    std::string calleeNode;
    std::string callerParam;
    std::string calleeParam;
    unsigned callerParamIndex = 0;
    unsigned calleeParamIndex = 0;
    std::string calleeName;
    std::string rendered;
    std::string callerPrefix;
    std::string formalEntry;
    std::string calleeCore;
    std::string wrapperInner;
    std::string wrapperRendered;
    std::string actualPath;
    std::string source;
    std::string note;
    std::string edgeKey;
    std::string summaryFile;
    std::string sourceFile;
    std::string tuSourceFile;
    Location location;
  };

  struct SummaryDependencySCCComponent {
    int64_t sccId = -1;
    std::vector<std::string> members;
    std::vector<std::string> translationUnits;
    int64_t size = 0;
    bool recursive = false;
    bool selfLoop = false;

    // C6g5s: C2 records whether a concrete recursive component contains an
    // actually imported edge.  `-1` preserves compatibility with an older
    // index that did not provide topology metadata; callers must then retain
    // the conservative legacy project replay rather than assuming local-only.
    int64_t concreteEdgeCount = -1;
    int64_t localEdgeCount = -1;
    int64_t importedEdgeCount = -1;

    bool hasEdgeTopology() const {
      return concreteEdgeCount >= 0 && localEdgeCount >= 0 &&
             importedEdgeCount >= 0;
    }

    bool isLocalOnly() const {
      return hasEdgeTopology() && importedEdgeCount == 0;
    }
  };

  struct SummaryDependencySCCReport {
    std::string graphKind;
    int64_t numNodes = 0;
    int64_t numUniqueEdges = 0;
    int64_t numSCCs = 0;
    int64_t numNontrivialSCCs = 0;
    std::vector<SummaryDependencySCCComponent> components;
    std::unordered_map<std::string, int64_t> nodeToScc;

    bool empty() const {
      return components.empty() && nodeToScc.empty() && graphKind.empty();
    }

    const SummaryDependencySCCComponent *componentForNode(llvm::StringRef node) const {
      auto it = nodeToScc.find(node.str());
      if (it == nodeToScc.end())
        return nullptr;
      int64_t idx = it->second;
      if (idx < 0 || static_cast<size_t>(idx) >= components.size())
        return nullptr;
      return &components[static_cast<size_t>(idx)];
    }
  };

  enum class LookupStatus {
    Found,
    Missing,
    Ambiguous,
    Unsafe,
    Disabled
  };

  struct LookupResult {
    LookupStatus status = LookupStatus::Missing;
    std::vector<size_t> candidateIndices;
    std::string message;

    bool foundUnique() const {
      return status == LookupStatus::Found && candidateIndices.size() == 1;
    }
  };

  bool loadFromFile(llvm::StringRef path, std::string *errorMessage = nullptr) {
    clear();
    indexPath = path.str();
    llvm::SmallString<256> dir(indexPath);
    llvm::sys::path::remove_filename(dir);
    indexDirectory = dir.str().str();

    auto bufferOrErr = llvm::MemoryBuffer::getFile(path);
    if (!bufferOrErr) {
      setError(errorMessage, "could not read summary index '" + path.str() +
                                 "': " + bufferOrErr.getError().message());
      return false;
    }

    auto parsed = llvm::json::parse((*bufferOrErr)->getBuffer());
    if (!parsed) {
      std::string msg;
      llvm::raw_string_ostream os(msg);
      llvm::logAllUnhandledErrors(parsed.takeError(), os,
                                  "failed to parse summary index: ");
      setError(errorMessage, os.str());
      return false;
    }

    const llvm::json::Object *root = parsed->getAsObject();
    if (!root) {
      setError(errorMessage, "summary index top-level JSON is not an object");
      return false;
    }

    schemaName = getString(*root, "schemaName");
    schemaVersion = getInteger(*root, "schemaVersion").value_or(0);
    summaryRoot = getString(*root, "summaryRoot");

    if (!schemaName.empty() &&
        schemaName != "cast-history-project-summary-index") {
      setError(errorMessage, "unsupported summary index schemaName: " +
                                 schemaName);
      return false;
    }

    const llvm::json::Array *functions = root->getArray("functions");
    if (!functions) {
      setError(errorMessage, "summary index does not contain a functions array");
      return false;
    }

    for (const llvm::json::Value &v : *functions) {
      const llvm::json::Object *obj = v.getAsObject();
      if (!obj)
        continue;

      FunctionEntry e;
      e.function = getString(*obj, "function");
      e.name = getString(*obj, "name");
      e.qualifiedName = getString(*obj, "qualifiedName");
      e.signatureKey = getString(*obj, "signatureKey");
      e.linkage = getString(*obj, "linkage");
      e.sourceFile = getString(*obj, "sourceFile");
      e.summaryFile = getString(*obj, "summaryFile");
      e.tuSourceFile = getString(*obj, "tuSourceFile");
      e.functionBehavior = getString(*obj, "functionBehavior");
      e.lookupKey = getString(*obj, "lookupKey");
      e.publicLookupKey = getString(*obj, "publicLookupKey");
      e.status = getString(*obj, "status");
      e.resolution = getString(*obj, "resolution");

      if (e.qualifiedName.empty())
        e.qualifiedName = e.function;
      if (e.name.empty())
        e.name = e.function;
      if (e.publicLookupKey.empty())
        e.publicLookupKey = makePublicLookupKey(e.qualifiedName,
                                                e.signatureKey);
      if (e.lookupKey.empty())
        e.lookupKey = e.publicLookupKey;

      if (const llvm::json::Object *loc = obj->getObject("location")) {
        e.location.file = getString(*loc, "file");
        e.location.line = getInteger(*loc, "line").value_or(0);
        e.location.column = getInteger(*loc, "column").value_or(0);
      }

      // C4G: optional project-level solved summaries for recursive SCCs.
      // These are stored directly in project-summary-index.json so import mode
      // can prefer a materialized SCC solution over the raw per-TU summaryFile
      // snapshot.  The visitor parses irJson back into ObsHist on demand.
      if (const llvm::json::Array *solved = obj->getArray("solvedParamSummaries")) {
        for (const llvm::json::Value &sv : *solved) {
          const llvm::json::Object *sobj = sv.getAsObject();
          if (!sobj)
            continue;
          FunctionEntry::SolvedParamSummaryEntry sp;
          sp.index = static_cast<unsigned>(getInteger(*sobj, "index").value_or(0));
          sp.rendered = getString(*sobj, "rendered");
          sp.source = getString(*sobj, "source");
          if (const llvm::json::Value *ir = sobj->get("ir")) {
            std::string text;
            llvm::raw_string_ostream os(text);
            os << *ir;
            os.flush();
            sp.irJson = std::move(text);
          }
          if (const llvm::json::Array *forks =
                  sobj->getArray("forkArmProvenance")) {
            for (const llvm::json::Value &fv : *forks) {
              const llvm::json::Object *fobj = fv.getAsObject();
              if (!fobj)
                continue;
              FunctionEntry::SolvedParamSummaryEntry::ForkArmProvenanceEntry
                  fork;
              fork.forkId = getString(*fobj, "forkId");
              fork.armId = getString(*fobj, "armId");
              fork.origin = getString(*fobj, "origin");
              fork.rendered = getString(*fobj, "rendered");
              if (!fork.forkId.empty() || !fork.armId.empty() ||
                  !fork.rendered.empty())
                sp.forkArmProvenance.push_back(std::move(fork));
            }
          }
          if (!sp.irJson.empty())
            e.solvedParamSummaries.push_back(std::move(sp));
        }
      }

      // C6g5v: canonical histories are a reporting companion to the
      // import-authoritative solvedParamSummaries field above.  Parse the same
      // record shape, but retain it separately so import behavior cannot be
      // changed by canonicalization.
      if (const llvm::json::Array *canonical =
              obj->getArray("canonicalProjectParamHistories")) {
        for (const llvm::json::Value &sv : *canonical) {
          const llvm::json::Object *sobj = sv.getAsObject();
          if (!sobj)
            continue;
          FunctionEntry::SolvedParamSummaryEntry sp;
          sp.index = static_cast<unsigned>(
              getInteger(*sobj, "index").value_or(0));
          sp.rendered = getString(*sobj, "rendered");
          sp.source = getString(*sobj, "source");
          if (const llvm::json::Value *ir = sobj->get("ir")) {
            std::string text;
            llvm::raw_string_ostream os(text);
            os << *ir;
            os.flush();
            sp.irJson = std::move(text);
          }
          if (const llvm::json::Array *forks =
                  sobj->getArray("forkArmProvenance")) {
            for (const llvm::json::Value &fv : *forks) {
              const llvm::json::Object *fobj = fv.getAsObject();
              if (!fobj)
                continue;
              FunctionEntry::SolvedParamSummaryEntry::ForkArmProvenanceEntry
                  fork;
              fork.forkId = getString(*fobj, "forkId");
              fork.armId = getString(*fobj, "armId");
              fork.origin = getString(*fobj, "origin");
              fork.rendered = getString(*fobj, "rendered");
              if (!fork.forkId.empty() || !fork.armId.empty() ||
                  !fork.rendered.empty())
                sp.forkArmProvenance.push_back(std::move(fork));
            }
          }
          if (!sp.irJson.empty())
            e.canonicalProjectParamHistories.push_back(std::move(sp));
        }
      }

      // C4P: optional project-level solved return summary.  This is the
      // return-side analogue of solvedParamSummaries: the project index may
      // materialize a wrapper-free return summary, including return-alias
      // metadata, so import mode does not have to rely only on raw per-TU
      // snapshots.
      if (const llvm::json::Object *sret = obj->getObject("solvedReturnSummary")) {
        FunctionEntry::SolvedReturnSummaryEntry sr;
        sr.rendered = getString(*sret, "rendered");
        sr.source = getString(*sret, "source");
        sr.mayAliasParam = sret->getBoolean("mayAliasParam").value_or(false);
        if (std::optional<int64_t> idx = getInteger(*sret, "aliasedParamIndex")) {
          if (*idx >= 0)
            sr.aliasedParamIndex = static_cast<unsigned>(*idx);
        }
        if (const llvm::json::Value *ir = sret->get("ir")) {
          std::string text;
          llvm::raw_string_ostream os(text);
          os << *ir;
          os.flush();
          sr.irJson = std::move(text);
        }
        if (!sr.irJson.empty())
          e.solvedReturnSummary = std::move(sr);
      }

      // C4Q2b: optional project-level solved pointer-use summaries.  These
      // are materialized by build_summary_index.py as a separate monotone
      // domain.  The visitor parses the stored use JSON lazily because the
      // PointerUseRecord type lives outside SummaryIndex.
      if (const llvm::json::Array *solvedUses = obj->getArray("solvedPointerUseSummaries")) {
        for (const llvm::json::Value &uv : *solvedUses) {
          const llvm::json::Object *uobj = uv.getAsObject();
          if (!uobj)
            continue;
          const llvm::json::Value *useVal = uobj->get("use");
          if (!useVal)
            continue;
          FunctionEntry::SolvedPointerUseSummaryEntry su;
          su.index = static_cast<unsigned>(getInteger(*uobj, "index").value_or(0));
          su.source = getString(*uobj, "source");
          std::string text;
          llvm::raw_string_ostream os(text);
          os << *useVal;
          os.flush();
          su.useJson = std::move(text);
          if (!su.useJson.empty())
            e.solvedPointerUseSummaries.push_back(std::move(su));
        }
      }

      size_t idx = entries.size();
      entries.push_back(std::move(e));
      byPublicLookupKey[entries.back().publicLookupKey].push_back(idx);
      byLookupKey[entries.back().lookupKey].push_back(idx);
      byQualifiedName[entries.back().qualifiedName].push_back(idx);
    }

    if (const llvm::json::Object *callProv = root->getObject("solvedParamCallProvenance")) {
      if (const llvm::json::Array *records = callProv->getArray("records")) {
        for (const llvm::json::Value &rv : *records) {
          const llvm::json::Object *robj = rv.getAsObject();
          if (!robj)
            continue;
          CallProvenanceRecord rec;
          rec.kind = getString(*robj, "kind");
          rec.callerNode = getString(*robj, "callerNode");
          rec.calleeNode = getString(*robj, "calleeNode");
          rec.callerParam = getString(*robj, "callerParam");
          rec.calleeParam = getString(*robj, "calleeParam");
          rec.callerParamIndex = static_cast<unsigned>(getInteger(*robj, "callerParamIndex").value_or(0));
          rec.calleeParamIndex = static_cast<unsigned>(getInteger(*robj, "calleeParamIndex").value_or(0));
          rec.calleeName = getString(*robj, "calleeName");
          rec.rendered = getString(*robj, "rendered");
          rec.callerPrefix = getString(*robj, "callerPrefix");
          rec.formalEntry = getString(*robj, "formalEntry");
          rec.calleeCore = getString(*robj, "calleeCore");
          rec.wrapperInner = getString(*robj, "wrapperInner");
          rec.wrapperRendered = getString(*robj, "wrapperRendered");
          rec.actualPath = getString(*robj, "actualPath");
          rec.source = getString(*robj, "source");
          rec.note = getString(*robj, "note");
          rec.edgeKey = getString(*robj, "edgeKey");
          rec.summaryFile = getString(*robj, "summaryFile");
          rec.sourceFile = getString(*robj, "sourceFile");
          rec.tuSourceFile = getString(*robj, "tuSourceFile");
          if (const llvm::json::Object *loc = robj->getObject("location")) {
            rec.location.file = getString(*loc, "file");
            rec.location.line = getInteger(*loc, "line").value_or(0);
            rec.location.column = getInteger(*loc, "column").value_or(0);
          }
          if (!rec.rendered.empty())
            callProvenanceRecords.push_back(std::move(rec));
        }
      }
    }

    // C6g5w1a: retain diagnostics-only branch-family metadata carried on the
    // normal project-level transfer edges.  A C2 index also contains a
    // compact branchFamilyTransferMetadata companion.  We read the raw
    // ordinary-edge representation first, then use the companion only to
    // rehydrate any record that an import-aware index rewrite omitted from its
    // root edge list.  Neither source participates in solver behavior.
    auto branchRecordKey = [](const BranchFamilyTransferMetadataRecord &rec) {
      return rec.familyId + "\x1f" + rec.armId + "\x1f" + rec.edgeKey;
    };
    std::unordered_map<std::string, size_t> branchRecordIndex;
    auto appendBranchRecord = [&](BranchFamilyTransferMetadataRecord rec) {
      if (rec.familyId.empty() || rec.armId.empty())
        return;
      const std::string key = branchRecordKey(rec);
      auto it = branchRecordIndex.find(key);
      if (it == branchRecordIndex.end()) {
        branchRecordIndex[key] = branchFamilyTransferMetadataRecords.size();
        branchFamilyTransferMetadataRecords.push_back(std::move(rec));
        return;
      }

      // Prefer the direct ordinary-transfer record, but fill missing transport
      // provenance from the project companion.  This proves rehydration while
      // keeping duplicate records out of text reports.
      BranchFamilyTransferMetadataRecord &existing =
          branchFamilyTransferMetadataRecords[it->second];
      if (existing.transportChannel.empty())
        existing.transportChannel = rec.transportChannel;
      if (existing.summaryFile.empty())
        existing.summaryFile = rec.summaryFile;
      if (existing.tuSourceFile.empty())
        existing.tuSourceFile = rec.tuSourceFile;
      if (existing.edgeKey.empty())
        existing.edgeKey = rec.edgeKey;
      if (existing.recursiveClosureKey.empty())
        existing.recursiveClosureKey = rec.recursiveClosureKey;
      existing.consumedByTaggedRecursiveSolver =
          existing.consumedByTaggedRecursiveSolver ||
          rec.consumedByTaggedRecursiveSolver;
      if (!existing.transferGuard && rec.transferGuard)
        existing.transferGuard = rec.transferGuard;
      if (!existing.armGuard && rec.armGuard)
        existing.armGuard = rec.armGuard;
      existing.rehydratedFromProjectMetadata =
          existing.rehydratedFromProjectMetadata ||
          rec.rehydratedFromProjectMetadata;
    };

    if (const llvm::json::Array *transfers =
            root->getArray("summaryParamTransferEdges")) {
      for (const llvm::json::Value &tv : *transfers) {
        const llvm::json::Object *tobj = tv.getAsObject();
        if (!tobj)
          continue;
        const llvm::json::Object *family = tobj->getObject("branchFamily");
        if (!family)
          continue;

        std::vector<const llvm::json::Object *> arms;
        if (const llvm::json::Object *single = tobj->getObject("branchArm"))
          arms.push_back(single);
        if (const llvm::json::Array *many = tobj->getArray("branchArms")) {
          for (const llvm::json::Value &value : *many) {
            if (const llvm::json::Object *arm = value.getAsObject())
              arms.push_back(arm);
          }
        }
        if (arms.empty())
          continue;

        for (const llvm::json::Object *arm : arms) {
          BranchFamilyTransferMetadataRecord rec;
          rec.familyId = getString(*family, "id");
          rec.familyKind = getString(*family, "kind");
          rec.armId = getString(*arm, "id");
          rec.armKind = getString(*arm, "kind");
          rec.ownerNode = getString(*family, "ownerNode");
          rec.ownerParamIndex = static_cast<unsigned>(
              getInteger(*family, "ownerParamIndex").value_or(0));
          rec.origin = getString(*family, "origin");
          rec.recursiveClosureKey =
              getString(*family, "recursiveClosureKey");
          if (rec.recursiveClosureKey.empty())
            rec.recursiveClosureKey = rec.familyId;
          rec.consumedByTaggedRecursiveSolver =
              family->getBoolean("consumedByTaggedRecursiveSolver").value_or(false);
          rec.actualPath = getString(*arm, "actualPath");
          std::string armGuard = getString(*arm, "guardText");
          if (!armGuard.empty())
            rec.armGuard = std::move(armGuard);
          rec.originSourceEntity = getString(*arm, "originSourceEntity");
          rec.callerNode = getString(*tobj, "callerNode");
          rec.calleeNode = getString(*tobj, "calleeNode");
          rec.callerParamIndex = static_cast<unsigned>(
              getInteger(*tobj, "callerParamIndex").value_or(0));
          rec.calleeParamIndex = static_cast<unsigned>(
              getInteger(*tobj, "calleeParamIndex").value_or(0));
          rec.edgeKey = getString(*tobj, "edgeKey");
          rec.summaryFile = getString(*tobj, "summaryFile");
          rec.tuSourceFile = getString(*tobj, "tuSourceFile");
          rec.transportChannel = "summaryParamTransferEdges";
          if (const llvm::json::Object *guard = tobj->getObject("guard")) {
            std::string text = getString(*guard, "text");
            if (!text.empty())
              rec.transferGuard = std::move(text);
          }
          if (const llvm::json::Object *loc = tobj->getObject("location")) {
            rec.location.file = getString(*loc, "file");
            rec.location.line = getInteger(*loc, "line").value_or(0);
            rec.location.column = getInteger(*loc, "column").value_or(0);
          }
          appendBranchRecord(std::move(rec));
        }
      }
    }

    // C6g5w1a import-side rehydration path.  The companion is built from the
    // same ordinary transfer edges by C2, but it survives project-index
    // rewrites that may omit an auxiliary branchFamily/branchArm payload from
    // a copied edge.  Reading it here gives imported-TU diagnostics a stable
    // source of truth without changing function-summary import.
    if (const llvm::json::Object *metadata =
            root->getObject("branchFamilyTransferMetadata")) {
      if (const llvm::json::Array *families = metadata->getArray("families")) {
        for (const llvm::json::Value &fv : *families) {
          const llvm::json::Object *fobj = fv.getAsObject();
          if (!fobj)
            continue;
          const std::string familyId = getString(*fobj, "id");
          const std::string familyKind = getString(*fobj, "kind");
          const std::string ownerNode = getString(*fobj, "ownerNode");
          const unsigned ownerParamIndex = static_cast<unsigned>(
              getInteger(*fobj, "ownerParamIndex").value_or(0));
          const std::string origin = getString(*fobj, "origin");
          const llvm::json::Array *arms = fobj->getArray("arms");
          if (!arms)
            continue;
          for (const llvm::json::Value &av : *arms) {
            const llvm::json::Object *aobj = av.getAsObject();
            if (!aobj)
              continue;
            BranchFamilyTransferMetadataRecord rec;
            rec.familyId = familyId;
            rec.familyKind = familyKind;
            rec.armId = getString(*aobj, "id");
            rec.armKind = getString(*aobj, "kind");
            rec.ownerNode = ownerNode;
            rec.ownerParamIndex = ownerParamIndex;
            rec.origin = origin;
            rec.recursiveClosureKey =
                getString(*fobj, "recursiveClosureKey");
            if (rec.recursiveClosureKey.empty())
              rec.recursiveClosureKey = rec.familyId;
            rec.consumedByTaggedRecursiveSolver =
                fobj->getBoolean("consumedByTaggedRecursiveSolver").value_or(false);
            rec.actualPath = getString(*aobj, "actualPath");
            std::string armGuard = getString(*aobj, "guardText");
            if (!armGuard.empty())
              rec.armGuard = std::move(armGuard);
            rec.originSourceEntity = getString(*aobj, "originSourceEntity");
            rec.callerNode = getString(*aobj, "callerNode");
            rec.calleeNode = getString(*aobj, "calleeNode");
            rec.callerParamIndex = static_cast<unsigned>(
                getInteger(*aobj, "callerParamIndex").value_or(0));
            rec.calleeParamIndex = static_cast<unsigned>(
                getInteger(*aobj, "calleeParamIndex").value_or(0));
            rec.edgeKey = getString(*aobj, "edgeKey");
            rec.summaryFile = getString(*aobj, "summaryFile");
            rec.tuSourceFile = getString(*aobj, "tuSourceFile");
            rec.transportChannel = "branchFamilyTransferMetadata";
            rec.rehydratedFromProjectMetadata = true;
            if (const llvm::json::Object *guard = aobj->getObject("transferGuard")) {
              std::string text = getString(*guard, "text");
              if (!text.empty())
                rec.transferGuard = std::move(text);
            }
            if (const llvm::json::Object *loc = aobj->getObject("callSite")) {
              rec.location.file = getString(*loc, "file");
              rec.location.line = getInteger(*loc, "line").value_or(0);
              rec.location.column = getInteger(*loc, "column").value_or(0);
            }
            appendBranchRecord(std::move(rec));
          }
        }
      }
    }

    if (const llvm::json::Object *depSccs = root->getObject("summaryDependencySCCs")) {
      if (const llvm::json::Object *concrete = depSccs->getObject("concreteFunctionGraph"))
        concreteFunctionSCCs = parseSCCReport(*concrete);
      if (const llvm::json::Object *full = depSccs->getObject("fullGraph"))
        fullSummarySCCs = parseSCCReport(*full);
    }

    return true;
  }

  void clear() {
    indexPath.clear();
    indexDirectory.clear();
    schemaName.clear();
    schemaVersion = 0;
    summaryRoot.clear();
    entries.clear();
    byPublicLookupKey.clear();
    byLookupKey.clear();
    byQualifiedName.clear();
    concreteFunctionSCCs = SummaryDependencySCCReport();
    fullSummarySCCs = SummaryDependencySCCReport();
    callProvenanceRecords.clear();
    branchFamilyTransferMetadataRecords.clear();
  }

  bool empty() const { return entries.empty(); }
  size_t functionCount() const { return entries.size(); }
  llvm::StringRef loadedPath() const { return indexPath; }
  llvm::StringRef loadedIndexDirectory() const { return indexDirectory; }
  llvm::StringRef loadedSchemaName() const { return schemaName; }
  int64_t loadedSchemaVersion() const { return schemaVersion; }
  llvm::StringRef loadedSummaryRoot() const { return summaryRoot; }

  const std::vector<FunctionEntry> &functions() const { return entries; }

  const std::vector<CallProvenanceRecord> &callProvenance() const {
    return callProvenanceRecords;
  }

  bool hasCallProvenance() const { return !callProvenanceRecords.empty(); }

  const std::vector<BranchFamilyTransferMetadataRecord> &
  branchFamilyTransferMetadata() const {
    return branchFamilyTransferMetadataRecords;
  }

  bool hasBranchFamilyTransferMetadata() const {
    return !branchFamilyTransferMetadataRecords.empty();
  }

  bool hasConcreteFunctionSCCs() const { return !concreteFunctionSCCs.empty(); }
  bool hasFullSummarySCCs() const { return !fullSummarySCCs.empty(); }

  const SummaryDependencySCCReport &concreteFunctionSCCReport() const {
    return concreteFunctionSCCs;
  }

  const SummaryDependencySCCReport &fullSummarySCCReport() const {
    return fullSummarySCCs;
  }

  const SummaryDependencySCCComponent *findConcreteFunctionSCCForNode(
      llvm::StringRef node) const {
    return concreteFunctionSCCs.componentForNode(node);
  }

  const SummaryDependencySCCComponent *findConcreteFunctionSCCForFunction(
      llvm::StringRef qualifiedName, llvm::StringRef signatureKey) const {
    return findConcreteFunctionSCCForNode(
        makePublicLookupKey(qualifiedName, signatureKey));
  }

  static std::string makePublicLookupKey(llvm::StringRef qualifiedName,
                                         llvm::StringRef signatureKey) {
    return qualifiedName.str() + "::" + signatureKey.str();
  }

  LookupResult lookupPublic(llvm::StringRef qualifiedName,
                            llvm::StringRef signatureKey) const {
    return lookupPublicKey(makePublicLookupKey(qualifiedName, signatureKey));
  }

  LookupResult lookupPublicKey(llvm::StringRef publicLookupKey) const {
    auto it = byPublicLookupKey.find(publicLookupKey.str());
    if (it == byPublicLookupKey.end()) {
      return {LookupStatus::Missing, {},
              "no function summary for public lookup key '" +
                  publicLookupKey.str() + "'"};
    }

    std::vector<size_t> safe;
    std::vector<size_t> unsafe;
    for (size_t idx : it->second) {
      if (idx >= entries.size())
        continue;
      if (entries[idx].isSafeForAcyclicDirectImport())
        safe.push_back(idx);
      else
        unsafe.push_back(idx);
    }

    if (safe.size() == 1)
      return {LookupStatus::Found, safe, "found unique safe summary"};

    if (safe.size() > 1)
      return {LookupStatus::Ambiguous, safe,
              "multiple safe summaries for public lookup key '" +
                  publicLookupKey.str() + "'"};

    return {LookupStatus::Unsafe, unsafe,
            "only unsafe/non-acyclic summaries for public lookup key '" +
                publicLookupKey.str() + "'"};
  }

  LookupResult lookupExactKey(llvm::StringRef lookupKey) const {
    auto it = byLookupKey.find(lookupKey.str());
    if (it == byLookupKey.end()) {
      return {LookupStatus::Missing, {},
              "no function summary for lookup key '" + lookupKey.str() +
                  "'"};
    }

    if (it->second.size() == 1)
      return {LookupStatus::Found, it->second, "found unique exact summary"};

    return {LookupStatus::Ambiguous, it->second,
            "multiple summaries for lookup key '" + lookupKey.str() + "'"};
  }

  const FunctionEntry *entryAt(size_t idx) const {
    if (idx >= entries.size())
      return nullptr;
    return &entries[idx];
  }

  void printDebugSummary(llvm::raw_ostream &os) const {
    size_t safe = 0;
    size_t sourceScoped = 0;
    size_t unsafe = 0;
    for (const auto &e : entries) {
      if (e.isSafeForAcyclicDirectImport())
        ++safe;
      else if (e.resolution == "source-scoped")
        ++sourceScoped;
      else
        ++unsafe;
    }

    os << "summary import index: " << indexPath << "\n";
    os << "  schema: " << (schemaName.empty() ? "<unknown>" : schemaName)
       << " v" << schemaVersion << "\n";
    os << "  summaryRoot: " << (summaryRoot.empty() ? "<none>" : summaryRoot)
       << "\n";
    os << "  functions: " << entries.size() << "\n";
    os << "  c3a-safe direct imports: " << safe << "\n";
    os << "  source-scoped/deferred: " << sourceScoped << "\n";
    os << "  unsafe/deferred: " << unsafe << "\n";
    os << "  branch-family transfer metadata: "
       << branchFamilyTransferMetadataRecords.size() << "\n";
  }

  std::string resolveSummaryFilePath(const FunctionEntry &entry) const {
    if (entry.summaryFile.empty())
      return std::string();

    if (llvm::sys::path::is_absolute(entry.summaryFile))
      return entry.summaryFile;

    llvm::SmallString<256> path;
    if (!indexDirectory.empty())
      path = indexDirectory;
    else if (!summaryRoot.empty())
      path = summaryRoot;

    if (path.empty())
      path = entry.summaryFile;
    else
      llvm::sys::path::append(path, entry.summaryFile);

    return path.str().str();
  }

  static const char *lookupStatusName(LookupStatus s) {
    switch (s) {
    case LookupStatus::Found: return "found";
    case LookupStatus::Missing: return "missing";
    case LookupStatus::Ambiguous: return "ambiguous";
    case LookupStatus::Unsafe: return "unsafe";
    case LookupStatus::Disabled: return "disabled";
    }
    return "unknown";
  }

private:
  static void setError(std::string *out, std::string msg) {
    if (out)
      *out = std::move(msg);
  }

  static std::string getString(const llvm::json::Object &obj,
                               llvm::StringRef key) {
    if (std::optional<llvm::StringRef> s = obj.getString(key))
      return s->str();
    return std::string();
  }

  static std::optional<int64_t> getInteger(const llvm::json::Object &obj,
                                           llvm::StringRef key) {
    if (std::optional<int64_t> i = obj.getInteger(key))
      return i;
    return std::nullopt;
  }

  static SummaryDependencySCCReport parseSCCReport(const llvm::json::Object &obj) {
    SummaryDependencySCCReport report;
    report.graphKind = getString(obj, "graphKind");
    report.numNodes = getInteger(obj, "numNodes").value_or(0);
    report.numUniqueEdges = getInteger(obj, "numUniqueEdges").value_or(0);
    report.numSCCs = getInteger(obj, "numSCCs").value_or(0);
    report.numNontrivialSCCs =
        getInteger(obj, "numNontrivialSCCs").value_or(0);

    if (const llvm::json::Array *components = obj.getArray("components")) {
      for (const llvm::json::Value &v : *components) {
        const llvm::json::Object *compObj = v.getAsObject();
        if (!compObj)
          continue;
        SummaryDependencySCCComponent c;
        c.sccId = getInteger(*compObj, "sccId").value_or(-1);
        c.size = getInteger(*compObj, "size").value_or(0);
        c.recursive = compObj->getBoolean("recursive").value_or(false);
        c.selfLoop = compObj->getBoolean("selfLoop").value_or(false);
        c.concreteEdgeCount = getInteger(*compObj, "concreteEdgeCount").value_or(-1);
        c.localEdgeCount = getInteger(*compObj, "localEdgeCount").value_or(-1);
        c.importedEdgeCount = getInteger(*compObj, "importedEdgeCount").value_or(-1);
        if (const llvm::json::Array *members = compObj->getArray("members")) {
          for (const llvm::json::Value &mv : *members) {
            if (std::optional<llvm::StringRef> s = mv.getAsString())
              c.members.push_back(s->str());
          }
        }
        if (const llvm::json::Array *tus = compObj->getArray("translationUnits")) {
          for (const llvm::json::Value &tv : *tus) {
            if (std::optional<llvm::StringRef> s = tv.getAsString())
              c.translationUnits.push_back(s->str());
          }
        }
        report.components.push_back(std::move(c));
      }
    }

    if (const llvm::json::Object *nodeToScc = obj.getObject("nodeToScc")) {
      for (const auto &it : *nodeToScc) {
        if (std::optional<int64_t> idx = it.second.getAsInteger())
          report.nodeToScc[it.first.str()] = *idx;
      }
    }

    return report;
  }

  std::string indexPath;
  std::string indexDirectory;
  std::string schemaName;
  int64_t schemaVersion = 0;
  std::string summaryRoot;
  std::vector<FunctionEntry> entries;
  std::unordered_map<std::string, std::vector<size_t>> byPublicLookupKey;
  std::unordered_map<std::string, std::vector<size_t>> byLookupKey;
  std::unordered_map<std::string, std::vector<size_t>> byQualifiedName;
  SummaryDependencySCCReport concreteFunctionSCCs;
  SummaryDependencySCCReport fullSummarySCCs;
  std::vector<CallProvenanceRecord> callProvenanceRecords;
  std::vector<BranchFamilyTransferMetadataRecord>
      branchFamilyTransferMetadataRecords;
};

} // namespace cast_history
