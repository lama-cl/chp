#include "CastHistory/CastHistoryVisitor.h"
#include "CastHistory/Options.h"
#include "CastHistory/SummaryIndex.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

#include <chrono>
#include <cctype>
#include <memory>
#include <string>

namespace cast_history {

llvm::cl::OptionCategory CastHistoryCategory("cast-history-tool options");
llvm::cl::opt<bool> EmitJson(
    "json",
    llvm::cl::desc("Emit JSON instead of plain text"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> IncludeImplicit(
    "include-implicit",
    llvm::cl::desc("Include meaningful implicit cast expressions in addition to explicit casts"),
    llvm::cl::init(true),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> DumpDebug(
    "debug-dump",
    llvm::cl::desc("Emit extra debug output to stderr"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> AnalysisProgress(
    "analysis-progress",
    llvm::cl::desc("Emit periodic analyzer phase/counter progress records to stderr"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<unsigned> AnalysisProgressIntervalSeconds(
    "analysis-progress-interval",
    llvm::cl::desc("Seconds between analyzer progress records; 0 prints phase transitions only"),
    llvm::cl::init(15),
    llvm::cl::value_desc("seconds"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> AnalysisProgressVerbose(
    "analysis-progress-verbose",
    llvm::cl::desc("Emit expensive per-candidate pointer-use diagnostics; requires --analysis-progress"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> NamedCallPointerUseMembershipCrossCheck(
    "named-call-pointer-use-membership-cross-check",
    llvm::cl::desc("Cross-check same-TU named-call pointer-use membership index against legacy vector scans (slow diagnostic)"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> NamedCallWatchdogDiagnostics(
    "named-call-watchdog-diagnostics",
    llvm::cl::desc("Emit bounded per-edge diagnostics for expensive same-TU named-call transfers"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> NamedCallWatchdogFilter(
    "named-call-watchdog-filter",
    llvm::cl::desc("Restrict named-call watchdog diagnostics to caller/callee names containing this substring"),
    llvm::cl::init(""),
    llvm::cl::value_desc("substring"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<unsigned> NamedCallWatchdogThresholdMilliseconds(
    "named-call-watchdog-threshold-ms",
    llvm::cl::desc("Emit a named-call watchdog event after this many milliseconds; 0 emits one completion event per watched edge"),
    llvm::cl::init(1000),
    llvm::cl::value_desc("milliseconds"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<unsigned> NamedCallWatchdogMaxEvents(
    "named-call-watchdog-max-events",
    llvm::cl::desc("Maximum named-call watchdog stderr events per translation unit; 0 means unlimited"),
    llvm::cl::init(64),
    llvm::cl::value_desc("count"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> SummaryJsonPath(
    "summary-json",
    llvm::cl::desc("Write the compact per-TU cast-history link summary JSON to this file; valid only with one input source"),
    llvm::cl::init(""),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> SummaryDir(
    "summary-dir",
    llvm::cl::desc("Write one compact per-TU cast-history link summary JSON file under this directory"),
    llvm::cl::init(""),
    llvm::cl::value_desc("dir"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> SummaryReportJsonPath(
    "summary-report-json",
    llvm::cl::desc("Write the optional verbose per-TU cast-history report JSON to this file; valid only with one input source"),
    llvm::cl::init(""),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> SummaryReportDir(
    "summary-report-dir",
    llvm::cl::desc("Write one optional verbose per-TU cast-history report JSON file under this directory"),
    llvm::cl::init(""),
    llvm::cl::value_desc("dir"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> PaperFactsJsonPath(
    "paper-facts-json",
    llvm::cl::desc("Write compact per-TU paper subject/callback facts JSON to this file; valid only with one input source"),
    llvm::cl::init(""),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> PaperFactsDir(
    "paper-facts-dir",
    llvm::cl::desc("Write one compact per-TU paper subject/callback facts JSON file under this directory"),
    llvm::cl::init(""),
    llvm::cl::value_desc("dir"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> PaperFactsProjectRoot(
    "paper-facts-project-root",
    llvm::cl::desc("Absolute project-root prefix used to inventory project-owned local/parameter pointer declarations in paper facts"),
    llvm::cl::init(""),
    llvm::cl::value_desc("dir"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> RuntimeTimingJsonPath(
    "runtime-timing-json",
    llvm::cl::desc("Write compact per-TU runtime timing JSON to this file; valid only with one input source"),
    llvm::cl::init(""),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> RuntimeTimingDir(
    "runtime-timing-dir",
    llvm::cl::desc("Write one compact per-TU runtime timing JSON file under this directory"),
    llvm::cl::init(""),
    llvm::cl::value_desc("dir"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> RuntimeTimingProjectId(
    "runtime-timing-project-id",
    llvm::cl::desc("Optional project identifier stored in runtime timing JSON"),
    llvm::cl::init(""),
    llvm::cl::value_desc("id"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> RuntimeTimingStage(
    "runtime-timing-stage",
    llvm::cl::desc("Optional pipeline stage stored in runtime timing JSON (for example c1 or import-round-01)"),
    llvm::cl::init(""),
    llvm::cl::value_desc("stage"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> SummaryConfigId(
    "summary-config-id",
    llvm::cl::desc("Optional configuration identifier stored in emitted TU summary JSON"),
    llvm::cl::init(""),
    llvm::cl::value_desc("id"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> SummaryIndexPath(
    "summary-index",
    llvm::cl::desc("Path to C2 project-summary-index.json for C3a summary import"),
    llvm::cl::init(""),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> EnableSummaryImport(
    "enable-summary-import",
    llvm::cl::desc("Enable C3a cross-TU summary import. C3a-1/C3a-2 only load the index; semantic composition is added later."),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> SummaryImportDebug(
    "summary-import-debug",
    llvm::cl::desc("Print C3a summary-index loading/debug information to stderr"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> HigherOrderSummaryDiagnostics(
    "higher-order-summary-diagnostics",
    llvm::cl::desc("Emit bounded higher-order callback-specialization diagnostics to stderr"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> HigherOrderSummaryDiagnosticsFilter(
    "higher-order-summary-diagnostics-filter",
    llvm::cl::desc("Restrict higher-order callback-specialization diagnostics to caller/callee/entity names containing this text"),
    llvm::cl::init(""),
    llvm::cl::value_desc("substring"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<unsigned> HigherOrderSummaryDiagnosticsMaxEvents(
    "higher-order-summary-diagnostics-max-events",
    llvm::cl::desc("Maximum higher-order callback-specialization diagnostic records; 0 means unlimited"),
    llvm::cl::init(256),
    llvm::cl::value_desc("count"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> PrevalenceMetricsJsonPath(
    "prevalence-metrics-json",
    llvm::cl::desc("Write source-level pointer/cast prevalence metrics JSON to this file; valid only with one input source"),
    llvm::cl::init(""),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> PrevalenceMetricsDir(
    "prevalence-metrics-dir",
    llvm::cl::desc("Write one source-level pointer/cast prevalence metrics JSON file per TU under this directory"),
    llvm::cl::init(""),
    llvm::cl::value_desc("dir"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> PrevalenceMetricsProjectRoot(
    "prevalence-metrics-project-root",
    llvm::cl::desc("Absolute project-root prefix used to count project-owned declarations and casts in prevalence metrics"),
    llvm::cl::init(""),
    llvm::cl::value_desc("dir"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<std::string> PrevalenceMetricsProjectId(
    "prevalence-metrics-project-id",
    llvm::cl::desc("Optional project identifier stored in prevalence metrics JSON"),
    llvm::cl::init(""),
    llvm::cl::value_desc("id"),
    llvm::cl::cat(CastHistoryCategory));

llvm::cl::opt<bool> PrevalenceIncludeReturnTypes(
    "prevalence-include-return-types",
    llvm::cl::desc("Include pointer-typed function definitions as return declarations in prevalence metrics"),
    llvm::cl::init(false),
    llvm::cl::cat(CastHistoryCategory));

static bool SidecarWriteFailed = false;
static std::unique_ptr<SummaryIndex> LoadedSummaryIndex;

static bool loadSummaryIndexIfRequested() {
  if (EnableSummaryImport && SummaryIndexPath.empty()) {
    llvm::errs() << "error: --enable-summary-import requires --summary-index <file>\n";
    return false;
  }

  if (SummaryIndexPath.empty())
    return true;

  auto index = std::make_unique<SummaryIndex>();
  std::string error;
  if (!index->loadFromFile(SummaryIndexPath, &error)) {
    llvm::errs() << "error: failed to load summary index: " << error << "\n";
    return false;
  }

  if (SummaryImportDebug) {
    llvm::errs() << "C3a summary import "
                 << (EnableSummaryImport ? "enabled" : "index loaded, import disabled")
                 << "\n";
    index->printDebugSummary(llvm::errs());
  }

  LoadedSummaryIndex = std::move(index);
  return true;
}

static std::string sourcePathForOutput(clang::ASTContext &context) {
  clang::SourceManager &sm = context.getSourceManager();
  clang::FileID fid = sm.getMainFileID();
  clang::SourceLocation loc = sm.getLocForStartOfFile(fid);

  llvm::StringRef name = sm.getFilename(loc);
  if (!name.empty())
    return name.str();

  clang::PresumedLoc presumed = sm.getPresumedLoc(loc);
  if (presumed.isValid())
    return std::string(presumed.getFilename());

  return "translation-unit";
}

static std::string sanitizePathComponent(llvm::StringRef input) {
  std::string out;
  out.reserve(input.size());

  for (char ch : input) {
    unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) || ch == '.' || ch == '_' || ch == '-')
      out.push_back(ch);
    else
      out.push_back('_');
  }

  while (out.find("__") != std::string::npos)
    out.replace(out.find("__"), 2, "_");

  while (!out.empty() && out.front() == '_')
    out.erase(out.begin());
  while (!out.empty() && out.back() == '_')
    out.pop_back();

  if (out.empty())
    out = "translation-unit";

  return out;
}

static bool ensureParentDirectory(llvm::StringRef outputPath) {
  llvm::SmallString<256> parent(outputPath);
  llvm::sys::path::remove_filename(parent);

  if (parent.empty())
    return true;

  std::error_code ec = llvm::sys::fs::create_directories(parent);
  if (ec) {
    llvm::errs() << "error: could not create summary output directory '"
                 << parent << "': " << ec.message() << "\n";
    return false;
  }
  return true;
}

// C6g5f4c: Link/report JSON is written to a sibling staging file and renamed
// only after the streamed emitter has completed successfully.  This preserves
// the previous good sidecar when a process is killed or a disk write fails;
// it also prevents C2/extractors from mistaking a partially-written sidecar
// for a valid completed artifact.
template <typename EmitFn>
static bool writeJsonSidecarAtomically(const std::string &outputPath,
                                       llvm::StringRef artifactLabel,
                                       EmitFn emit) {
  const std::string temporaryPath = outputPath + ".tmp";
  // A prior interrupted attempt may have left this file behind.  It is never a
  // published artifact, so removal is safe before opening a new staging file.
  llvm::sys::fs::remove(temporaryPath);

  bool writeFailed = false;
  {
    std::error_code ec;
    llvm::raw_fd_ostream os(temporaryPath, ec, llvm::sys::fs::OF_Text);
    if (ec) {
      llvm::errs() << "error: could not open staged " << artifactLabel
                   << " output file '" << temporaryPath
                   << "': " << ec.message() << "\n";
      return false;
    }

    emit(os);
    os.flush();
    if (os.has_error()) {
      llvm::errs() << "error: failed while writing staged " << artifactLabel
                   << " output file '" << temporaryPath << "'\n";
      os.clear_error();
      writeFailed = true;
    }
  } // Close the descriptor before the same-directory rename.

  if (writeFailed) {
    llvm::sys::fs::remove(temporaryPath);
    return false;
  }

  if (std::error_code ec = llvm::sys::fs::rename(temporaryPath, outputPath)) {
    llvm::errs() << "error: could not publish " << artifactLabel
                 << " output file '" << outputPath
                 << "': " << ec.message() << "\n";
    llvm::sys::fs::remove(temporaryPath);
    return false;
  }
  return true;
}

static std::string summaryPathForTU(clang::ASTContext &context) {
  if (!SummaryJsonPath.empty())
    return SummaryJsonPath;

  if (SummaryDir.empty())
    return std::string();

  llvm::SmallString<256> path(SummaryDir);
  std::string source = sourcePathForOutput(context);
  std::string fileName = sanitizePathComponent(source) + ".cast-summary.json";
  llvm::sys::path::append(path, fileName);
  return path.str().str();
}

static std::string summaryReportPathForTU(clang::ASTContext &context) {
  if (!SummaryReportJsonPath.empty())
    return SummaryReportJsonPath;

  if (SummaryReportDir.empty())
    return std::string();

  llvm::SmallString<256> path(SummaryReportDir);
  std::string source = sourcePathForOutput(context);
  std::string fileName = sanitizePathComponent(source) + ".cast-report.json";
  llvm::sys::path::append(path, fileName);
  return path.str().str();
}

static std::string paperFactsPathForTU(clang::ASTContext &context) {
  if (!PaperFactsJsonPath.empty())
    return PaperFactsJsonPath;

  if (PaperFactsDir.empty())
    return std::string();

  llvm::SmallString<256> path(PaperFactsDir);
  std::string source = sourcePathForOutput(context);
  std::string fileName = sanitizePathComponent(source) + ".cast-paper-facts.json";
  llvm::sys::path::append(path, fileName);
  return path.str().str();
}

static std::string runtimeTimingPathForTU(clang::ASTContext &context) {
  if (!RuntimeTimingJsonPath.empty())
    return RuntimeTimingJsonPath;

  if (RuntimeTimingDir.empty())
    return std::string();

  llvm::SmallString<256> path(RuntimeTimingDir);
  std::string source = sourcePathForOutput(context);
  std::string fileName =
      sanitizePathComponent(source) + ".cast-runtime.json";
  llvm::sys::path::append(path, fileName);
  return path.str().str();
}

static std::string prevalenceMetricsPathForTU(clang::ASTContext &context) {
  if (!PrevalenceMetricsJsonPath.empty())
    return PrevalenceMetricsJsonPath;

  if (PrevalenceMetricsDir.empty())
    return std::string();

  llvm::SmallString<256> path(PrevalenceMetricsDir);
  std::string source = sourcePathForOutput(context);
  std::string fileName =
      sanitizePathComponent(source) + ".cast-prevalence.json";
  llvm::sys::path::append(path, fileName);
  return path.str().str();
}

static bool writeTULinkSummaryIfRequested(clang::ASTContext &context,
                                          CastHistoryVisitor &visitor) {
  std::string outputPath = summaryPathForTU(context);
  if (outputPath.empty())
    return true;

  if (!ensureParentDirectory(outputPath))
    return false;

  return writeJsonSidecarAtomically(
      outputPath, "link summary", [&](llvm::raw_ostream &os) {
        visitor.emitTULinkSummaryJson(os, SummaryConfigId, "");
      });
}

static bool writeTUReportIfRequested(clang::ASTContext &context,
                                     CastHistoryVisitor &visitor) {
  std::string outputPath = summaryReportPathForTU(context);
  if (outputPath.empty())
    return true;

  if (!ensureParentDirectory(outputPath))
    return false;

  // Store a logical link-summary file name, rather than a staging path.
  // Runners commonly publish links under summaries/ and reports under reports/;
  // a concrete relative or absolute staging path would become stale as soon as
  // the run completes.
  std::string linkSummaryReference;
  std::string linkSummaryPath = summaryPathForTU(context);
  if (!linkSummaryPath.empty())
    linkSummaryReference = llvm::sys::path::filename(linkSummaryPath).str();

  return writeJsonSidecarAtomically(
      outputPath, "report summary", [&](llvm::raw_ostream &os) {
        visitor.emitTUReportJson(os, SummaryConfigId, "", linkSummaryReference);
      });
}

static bool writePaperFactsIfRequested(clang::ASTContext &context,
                                      CastHistoryVisitor &visitor) {
  std::string outputPath = paperFactsPathForTU(context);
  if (outputPath.empty())
    return true;

  if (!ensureParentDirectory(outputPath))
    return false;

  std::error_code ec;
  llvm::raw_fd_ostream os(outputPath, ec, llvm::sys::fs::OF_Text);
  if (ec) {
    llvm::errs() << "error: could not open paper-facts output file '"
                 << outputPath << "': " << ec.message() << "\n";
    return false;
  }

  visitor.emitTUPaperFactsJson(os, SummaryConfigId, "");
  os.flush();
  if (os.has_error()) {
    llvm::errs() << "error: failed while writing paper-facts output file '"
                 << outputPath << "'\n";
    os.clear_error();
    return false;
  }
  return true;
}

static bool writeRuntimeTimingIfRequested(clang::ASTContext &context,
                                          CastHistoryVisitor &visitor) {
  std::string outputPath = runtimeTimingPathForTU(context);
  if (outputPath.empty())
    return true;

  if (!ensureParentDirectory(outputPath))
    return false;

  std::error_code ec;
  llvm::raw_fd_ostream os(outputPath, ec, llvm::sys::fs::OF_Text);
  if (ec) {
    llvm::errs() << "error: could not open runtime timing output file '"
                 << outputPath << "': " << ec.message() << "\n";
    return false;
  }

  visitor.emitRuntimeTimingJson(os, SummaryConfigId, RuntimeTimingProjectId,
                                RuntimeTimingStage);
  os.flush();
  if (os.has_error()) {
    llvm::errs() << "error: failed while writing runtime timing output file '"
                 << outputPath << "'\n";
    os.clear_error();
    return false;
  }
  return true;
}

static bool writePrevalenceMetricsIfRequested(clang::ASTContext &context,
                                             CastHistoryVisitor &visitor) {
  std::string outputPath = prevalenceMetricsPathForTU(context);
  if (outputPath.empty())
    return true;

  if (!ensureParentDirectory(outputPath))
    return false;

  std::error_code ec;
  llvm::raw_fd_ostream os(outputPath, ec, llvm::sys::fs::OF_Text);
  if (ec) {
    llvm::errs() << "error: could not open prevalence metrics output file '"
                 << outputPath << "': " << ec.message() << "\n";
    return false;
  }

  visitor.emitPrevalenceMetricsJson(os, SummaryConfigId, "");
  os.flush();
  if (os.has_error()) {
    llvm::errs() << "error: failed while writing prevalence metrics output file '"
                 << outputPath << "'\n";
    os.clear_error();
    return false;
  }
  return true;
}

static void emitFrontendProgress(llvm::StringRef phase,
                                 llvm::StringRef inputFile = llvm::StringRef()) {
  if (!AnalysisProgress)
    return;

  llvm::errs() << "[analysis-progress] phase=" << phase;
  if (!inputFile.empty())
    llvm::errs() << "; input=" << inputFile;
  llvm::errs() << "\n";
  llvm::errs().flush();
}

class CastHistoryConsumer : public clang::ASTConsumer {
public:
  explicit CastHistoryConsumer(clang::ASTContext &context)
      : visitor(context, LoadedSummaryIndex.get(), EnableSummaryImport,
                SummaryImportDebug, AnalysisProgress,
                AnalysisProgressIntervalSeconds, AnalysisProgressVerbose,
                NamedCallPointerUseMembershipCrossCheck,
                NamedCallWatchdogDiagnostics, NamedCallWatchdogFilter,
                NamedCallWatchdogThresholdMilliseconds,
                NamedCallWatchdogMaxEvents,
                !PrevalenceMetricsJsonPath.empty() ||
                    !PrevalenceMetricsDir.empty(),
                PrevalenceMetricsProjectRoot, PrevalenceIncludeReturnTypes,
                PrevalenceMetricsProjectId,
                !PaperFactsJsonPath.empty() || !PaperFactsDir.empty(),
                PaperFactsProjectRoot) {}

  void HandleTranslationUnit(clang::ASTContext &context) override {
    // Clang calls this after parsing and constructing the translation-unit AST.
    // Keep that frontend cost separate from visitor traversal and solving.
    visitor.finishRuntimeParseAndAstConstruction();

    visitor.reportAnalysisProgress("ast-traversal", true);
    const auto traversalStartedAt = std::chrono::steady_clock::now();
    visitor.TraverseDecl(context.getTranslationUnitDecl());
    visitor.addRuntimeAstTraversalMilliseconds(
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - traversalStartedAt)
                                         .count()));
    visitor.reportAnalysisProgress("ast-traversal-complete", true);

    // Finalize summaries before timing artifact emission.  The three top-level
    // paper columns are parser/AST construction, cast-history construction, and
    // artifact write-out; SCC/import fields stay in the timing sidecar as
    // optional diagnostic subphases.
    visitor.prepareFinalSummariesForReporting();

    // Prevalence collection is intentionally AST-local. Write it before the
    // potentially expensive cast-history report path so an experimental solver
    // divergence cannot discard already-complete pointer/cast prevalence facts.
    if (!PrevalenceMetricsJsonPath.empty() || !PrevalenceMetricsDir.empty()) {
      visitor.reportAnalysisProgress("write-prevalence-metrics", true);
      const auto writeStartedAt = std::chrono::steady_clock::now();
      if (!writePrevalenceMetricsIfRequested(context, visitor))
        SidecarWriteFailed = true;
      visitor.addRuntimePrevalenceMetricsWriteMilliseconds(
          static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - writeStartedAt)
                                           .count()));
      visitor.reportAnalysisProgress("write-prevalence-metrics-complete", true);
    }

    const auto stdoutWriteStartedAt = std::chrono::steady_clock::now();
    if (EmitJson) {
      visitor.reportAnalysisProgress("emit-json", true);
      llvm::json::Object out{{"records", visitor.emitJson()}};
      llvm::outs() << llvm::formatv("{0:2}\n", llvm::json::Value(std::move(out)));
      visitor.reportAnalysisProgress("emit-json-complete", true);
    } else {
      visitor.emitText(llvm::outs());
    }

    // Keep human-readable stdout, the compact import-facing link summary,
    // and the optional verbose report sidecar separated.  This ensures that
    // C2 only ever consumes compact summaries and that report rendering cannot
    // enlarge the merge/import path.
    llvm::outs().flush();
    visitor.addRuntimeStdoutReportWriteMilliseconds(
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - stdoutWriteStartedAt)
                                         .count()));

    if (!SummaryJsonPath.empty() || !SummaryDir.empty()) {
      visitor.reportAnalysisProgress("write-tu-link-summary", true);
      const auto writeStartedAt = std::chrono::steady_clock::now();
      if (!writeTULinkSummaryIfRequested(context, visitor))
        SidecarWriteFailed = true;
      visitor.addRuntimeLinkSummaryWriteMilliseconds(
          static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - writeStartedAt)
                                           .count()));
      visitor.reportAnalysisProgress("write-tu-link-summary-complete", true);
    }

    if (!SummaryReportJsonPath.empty() || !SummaryReportDir.empty()) {
      visitor.reportAnalysisProgress("write-tu-report", true);
      const auto writeStartedAt = std::chrono::steady_clock::now();
      if (!writeTUReportIfRequested(context, visitor))
        SidecarWriteFailed = true;
      visitor.addRuntimeVerboseReportWriteMilliseconds(
          static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - writeStartedAt)
                                           .count()));
      visitor.reportAnalysisProgress("write-tu-report-complete", true);
    }

    if (!PaperFactsJsonPath.empty() || !PaperFactsDir.empty()) {
      visitor.reportAnalysisProgress("write-tu-paper-facts", true);
      const auto writeStartedAt = std::chrono::steady_clock::now();
      if (!writePaperFactsIfRequested(context, visitor))
        SidecarWriteFailed = true;
      visitor.addRuntimePaperFactsWriteMilliseconds(
          static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - writeStartedAt)
                                           .count()));
      visitor.reportAnalysisProgress("write-tu-paper-facts-complete", true);
    }

    // This compact sidecar is emitted last. Its total intentionally excludes
    // the negligible timing-sidecar write itself to avoid self-referential
    // accounting.
    if (!RuntimeTimingJsonPath.empty() || !RuntimeTimingDir.empty()) {
      visitor.reportAnalysisProgress("write-runtime-timing", true);
      if (!writeRuntimeTimingIfRequested(context, visitor))
        SidecarWriteFailed = true;
      visitor.reportAnalysisProgress("write-runtime-timing-complete", true);
    }

    visitor.reportAnalysisProgress("translation-unit-complete", true);
  }

private:
  CastHistoryVisitor visitor;
};

class CastHistoryAction : public clang::ASTFrontendAction {
public:
  bool BeginSourceFileAction(clang::CompilerInstance &ci) override {
    emitFrontendProgress("frontend-begin");
    return clang::ASTFrontendAction::BeginSourceFileAction(ci);
  }

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef inputFile) override {
    emitFrontendProgress("frontend-consumer-created", inputFile);
    return std::make_unique<CastHistoryConsumer>(ci.getASTContext());
  }
};

} // namespace cast_history

int main(int argc, const char **argv) {
  auto expectedParser = clang::tooling::CommonOptionsParser::create(
      argc, argv, cast_history::CastHistoryCategory);
  if (!expectedParser) {
    llvm::errs() << expectedParser.takeError();
    return 1;
  }

  clang::tooling::CommonOptionsParser &optionsParser = expectedParser.get();

  if (!cast_history::SummaryJsonPath.empty() &&
      !cast_history::SummaryDir.empty()) {
    llvm::errs() << "error: use only one of --summary-json or --summary-dir\n";
    return 1;
  }

  if (!cast_history::SummaryReportJsonPath.empty() &&
      !cast_history::SummaryReportDir.empty()) {
    llvm::errs() << "error: use only one of --summary-report-json or --summary-report-dir\n";
    return 1;
  }

  if (!cast_history::PaperFactsJsonPath.empty() &&
      !cast_history::PaperFactsDir.empty()) {
    llvm::errs() << "error: use only one of --paper-facts-json or --paper-facts-dir\n";
    return 1;
  }

  if (!cast_history::PaperFactsJsonPath.empty() &&
      optionsParser.getSourcePathList().size() != 1) {
    llvm::errs() << "error: --paper-facts-json is valid only with exactly one input source; "
                 << "use --paper-facts-dir for multiple translation units\n";
    return 1;
  }

  if (!cast_history::PaperFactsProjectRoot.empty() &&
      !llvm::sys::path::is_absolute(cast_history::PaperFactsProjectRoot)) {
    llvm::errs() << "error: --paper-facts-project-root must be an absolute path\n";
    return 1;
  }

  if (!cast_history::RuntimeTimingJsonPath.empty() &&
      !cast_history::RuntimeTimingDir.empty()) {
    llvm::errs() << "error: use only one of --runtime-timing-json or "
                 << "--runtime-timing-dir\n";
    return 1;
  }

  if (!cast_history::RuntimeTimingJsonPath.empty() &&
      optionsParser.getSourcePathList().size() != 1) {
    llvm::errs() << "error: --runtime-timing-json is valid only with exactly one input source; "
                 << "use --runtime-timing-dir for multiple translation units\n";
    return 1;
  }

  if (!cast_history::SummaryJsonPath.empty() &&
      optionsParser.getSourcePathList().size() != 1) {
    llvm::errs() << "error: --summary-json is valid only with exactly one input source; "
                 << "use --summary-dir for multiple translation units\n";
    return 1;
  }

  if (!cast_history::SummaryReportJsonPath.empty() &&
      optionsParser.getSourcePathList().size() != 1) {
    llvm::errs() << "error: --summary-report-json is valid only with exactly one input source; "
                 << "use --summary-report-dir for multiple translation units\n";
    return 1;
  }

  if (!cast_history::PrevalenceMetricsJsonPath.empty() &&
      !cast_history::PrevalenceMetricsDir.empty()) {
    llvm::errs() << "error: use only one of --prevalence-metrics-json or "
                 << "--prevalence-metrics-dir\n";
    return 1;
  }

  const bool prevalenceMetricsRequested =
      !cast_history::PrevalenceMetricsJsonPath.empty() ||
      !cast_history::PrevalenceMetricsDir.empty();
  if (prevalenceMetricsRequested &&
      cast_history::PrevalenceMetricsProjectRoot.empty()) {
    llvm::errs() << "error: prevalence metrics require "
                 << "--prevalence-metrics-project-root <absolute-project-root>\n";
    return 1;
  }
  if (prevalenceMetricsRequested &&
      !llvm::sys::path::is_absolute(
          cast_history::PrevalenceMetricsProjectRoot)) {
    llvm::errs() << "error: --prevalence-metrics-project-root must be an "
                 << "absolute path\n";
    return 1;
  }

  if (!cast_history::PrevalenceMetricsJsonPath.empty() &&
      optionsParser.getSourcePathList().size() != 1) {
    llvm::errs() << "error: --prevalence-metrics-json is valid only with exactly one input source; "
                 << "use --prevalence-metrics-dir for multiple translation units\n";
    return 1;
  }

  if (!cast_history::loadSummaryIndexIfRequested())
    return 1;

  clang::tooling::ClangTool tool(optionsParser.getCompilations(),
                                 optionsParser.getSourcePathList());

  int rc = tool.run(
      clang::tooling::newFrontendActionFactory<cast_history::CastHistoryAction>().get());
  if (cast_history::SidecarWriteFailed)
    return 1;
  return rc;
}
