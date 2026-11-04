#pragma once

#include <llvm/Support/CommandLine.h>

#include <string>

namespace cast_history {

extern llvm::cl::OptionCategory CastHistoryCategory;
extern llvm::cl::opt<bool> EmitJson;
extern llvm::cl::opt<bool> IncludeImplicit;
extern llvm::cl::opt<bool> DumpDebug;

// Opt-in analyzer progress reporting. Progress records are emitted only to
// stderr so normal cast-history text/JSON output on stdout remains stable.
extern llvm::cl::opt<bool> AnalysisProgress;
extern llvm::cl::opt<unsigned> AnalysisProgressIntervalSeconds;
// C6g5f4b: opt in to expensive per-candidate/attachment diagnostics.  Normal
// --analysis-progress remains suitable for large benchmark runs.
extern llvm::cl::opt<bool> AnalysisProgressVerbose;
// C6g5f4b: slow diagnostic oracle for the same-TU named-call pointer-use
// membership index.  It compares index results to the legacy vector scan.
extern llvm::cl::opt<bool> NamedCallPointerUseMembershipCrossCheck;

// C6g5c: opt-in diagnostics for a single expensive same-TU named-call edge.
// These controls are observational only: they never participate in summary
// membership, recursive widening, or SCC scheduling.
extern llvm::cl::opt<bool> NamedCallWatchdogDiagnostics;
extern llvm::cl::opt<std::string> NamedCallWatchdogFilter;
extern llvm::cl::opt<unsigned> NamedCallWatchdogThresholdMilliseconds;
extern llvm::cl::opt<unsigned> NamedCallWatchdogMaxEvents;

// C1b summary-output options.
//   --summary-json <file> writes the current TU summary to a single file.
//   --summary-dir <dir> writes one summary file per TU, using a sanitized
//   source-path-derived filename.
// C6g5e summary split:
//   --summary-json/--summary-dir write the compact link/interchange summary
//   consumed by C2/index/import.  The optional report path/dir writes the
//   verbose human-facing JSON separately so it never enters the merge path.
extern llvm::cl::opt<std::string> SummaryJsonPath;
extern llvm::cl::opt<std::string> SummaryDir;
extern llvm::cl::opt<std::string> SummaryReportJsonPath;
extern llvm::cl::opt<std::string> SummaryReportDir;

// C6g5f compact paper-facts sidecar. This is independent of the C2 link
// summary and the verbose report: it contains stable subject/callback facts
// for table aggregation and external-tool alignment.
extern llvm::cl::opt<std::string> PaperFactsJsonPath;
extern llvm::cl::opt<std::string> PaperFactsDir;
// C6g5f4d: project-root scope for a reporting-only declaration inventory.
// When absent, the producer falls back to the main TU and marks that scope
// incomplete so corpus aggregation cannot silently overclaim coverage.
extern llvm::cl::opt<std::string> PaperFactsProjectRoot;

extern llvm::cl::opt<std::string> SummaryConfigId;

// C3a summary-import options.
// C3a-1/C3a-2 only load and query the index; they do not yet compose imported
// summaries into local cast histories.
extern llvm::cl::opt<std::string> SummaryIndexPath;
extern llvm::cl::opt<bool> EnableSummaryImport;
extern llvm::cl::opt<bool> SummaryImportDebug;

// Opt-in diagnostics for higher-order callback specialization. These records
// are emitted to stderr only and never participate in cast-history membership.
extern llvm::cl::opt<bool> HigherOrderSummaryDiagnostics;
extern llvm::cl::opt<std::string> HigherOrderSummaryDiagnosticsFilter;
extern llvm::cl::opt<unsigned> HigherOrderSummaryDiagnosticsMaxEvents;

// Opt-in source-level prevalence metrics sidecar. These metrics are separate
// from cast-history summaries and are collected during AST traversal only.
extern llvm::cl::opt<std::string> PrevalenceMetricsJsonPath;
extern llvm::cl::opt<std::string> PrevalenceMetricsDir;
extern llvm::cl::opt<std::string> PrevalenceMetricsProjectRoot;
extern llvm::cl::opt<std::string> PrevalenceMetricsProjectId;
extern llvm::cl::opt<bool> PrevalenceIncludeReturnTypes;

} // namespace cast_history
