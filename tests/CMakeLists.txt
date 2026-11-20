# Regression tests: see design/regression-testing.md (§4.6).
#
#   cd build && ctest --output-on-failure         # everything
#   ctest -L regress                              # expected-output diffs
#   ctest -L contracts                            # static source contracts (seconds)
#   ctest -LE known-failing                       # skip tests that fail on main today
#
# Labels: regress, checkers, contracts, smoke, known-failing.

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CH_REPO "${PROJECT_SOURCE_DIR}")
set(CH_REGRESS "${CH_REPO}/scripts/regress.py")
set(CH_OUT "${CMAKE_BINARY_DIR}/regress-out")
set(CH_REGRESS_ARGS --tool $<TARGET_FILE:cast-history-tool> --out-dir ${CH_OUT})

# Tests that fail on the current main branch for reasons unrelated to the
# harness (outdated assertions).  They still run and still report; the label
# only lets `ctest -LE known-failing` give a clean signal.  See §11 of the
# design doc.  Remove an entry as soon as the test is fixed.
set(CH_KNOWN_FAILING
  contract-c6g5u_project_materialization
  smoke-c6g5u_direct_local_project_materialization
  verify-p91-json
)

function(ch_add_test name label)
  add_test(NAME ${name} COMMAND ${ARGN} WORKING_DIRECTORY ${CH_REPO})
  set(labels ${label})
  if(name IN_LIST CH_KNOWN_FAILING)
    list(APPEND labels known-failing)
  endif()
  set_tests_properties(${name} PROPERTIES LABELS "${labels}")
endfunction()

# --------------------------------------------------------------------- regress
# One run of the tool per fixture; every test below reads that output, so an
# expected-output diff never hides a checker result (or vice versa).
ch_add_test(regress-produce regress
  ${Python3_EXECUTABLE} ${CH_REGRESS} produce ${CH_REGRESS_ARGS})
set_tests_properties(regress-produce PROPERTIES FIXTURES_SETUP ch_regress_out)

foreach(kind examples projects)
  ch_add_test(regress-${kind} regress
    ${Python3_EXECUTABLE} ${CH_REGRESS} run --reuse --only ${kind} ${CH_REGRESS_ARGS})
  set_tests_properties(regress-${kind} PROPERTIES FIXTURES_REQUIRED ch_regress_out)
endforeach()

# -------------------------------------------------------------------- checkers
set(P126 p126_recursive_parallel_observation_dedup)
set(P91 p91_deref_fnptr_higher_order_callee_normalization)
set(P126_OUT "${CH_OUT}/projects/${P126}/${P126}")
set(P91_OUT "${CH_OUT}/projects/${P91}/${P91}")

# tests/verify_c6g5*_report.py take a text report and a fixture name.
# verify_c6g5p is not wired: it reads [recursive-scc-transfer-diagnostic]
# log lines that the tool no longer emits.
foreach(step q r s t u)
  file(GLOB script "${CH_REPO}/tests/verify_c6g5${step}_*.py")
  ch_add_test(verify-c6g5${step}-p126 checkers
    ${Python3_EXECUTABLE} ${script} --fixture p126 ${P126_OUT}/a.c.txt)
  set_tests_properties(verify-c6g5${step}-p126 PROPERTIES FIXTURES_REQUIRED ch_regress_out)
endforeach()

set(P91_EXPECTED "${CH_REPO}/test_projects/projects/${P91}/expected")
ch_add_test(verify-p91 checkers
  ${Python3_EXECUTABLE} ${P91_EXPECTED}/verify_p91.py ${P91_OUT}/a.c.txt)
ch_add_test(verify-p91-json checkers
  ${Python3_EXECUTABLE} ${P91_EXPECTED}/verify_p91_json.py ${P91_OUT}/summaries/a.c.cast-summary.json)
ch_add_test(verify-p91-index checkers
  ${Python3_EXECUTABLE} ${P91_EXPECTED}/verify_p91_index.py ${P91_OUT}/project-summary-index.json)
set_tests_properties(verify-p91 verify-p91-json verify-p91-index
  PROPERTIES FIXTURES_REQUIRED ch_regress_out)

# Summary validators, on each project's summaries/ folder (pointing them at the
# whole output tree would also feed them prevalence sidecars they don't know).
set(summary_dirs)
foreach(p p01_direct_cross_tu p02_cross_tu_return p03_cross_tu_mutual_recursion ${P91} ${P126})
  list(APPEND summary_dirs "${CH_OUT}/projects/${p}/${p}/summaries")
endforeach()
ch_add_test(validate-json-summaries checkers
  ${Python3_EXECUTABLE} ${CH_REPO}/scripts/validate_json_summaries.py --strict ${summary_dirs})
ch_add_test(validate-summary-schema checkers
  ${Python3_EXECUTABLE} ${CH_REPO}/scripts/validate_cast_summary_schema.py --strict ${summary_dirs})
set_tests_properties(validate-json-summaries validate-summary-schema
  PROPERTIES FIXTURES_REQUIRED ch_regress_out)

# ------------------------------------------------- contracts and smoke tests
# These need neither the tool nor its output.
file(GLOB contract_scripts "${CH_REPO}/tests/*_source_contract.py")
foreach(script ${contract_scripts})
  get_filename_component(stem ${script} NAME_WE)
  string(REGEX REPLACE "_source_contract$" "" stem ${stem})
  ch_add_test(contract-${stem} contracts ${Python3_EXECUTABLE} ${script})
endforeach()

file(GLOB smoke_scripts "${CH_REPO}/tests/*_smoke.py")
foreach(script ${smoke_scripts})
  get_filename_component(stem ${script} NAME_WE)
  string(REGEX REPLACE "_smoke$" "" stem ${stem})
  ch_add_test(smoke-${stem} smoke ${Python3_EXECUTABLE} ${script})
endforeach()

# C++ smoke tests: header-only model checks (HistoryIR.h uses only the STL).
file(GLOB smoke_sources "${CH_REPO}/tests/*_smoke.cpp")
foreach(src ${smoke_sources})
  get_filename_component(stem ${src} NAME_WE)
  string(REGEX REPLACE "_smoke$" "" stem ${stem})
  add_executable(smoke-${stem} ${src})
  target_include_directories(smoke-${stem} PRIVATE ${CH_REPO}/include)
  # The checks are assert()s: keep them in every build type.
  target_compile_options(smoke-${stem} PRIVATE -UNDEBUG)
  ch_add_test(smoke-${stem}-cpp smoke $<TARGET_FILE:smoke-${stem}>)
endforeach()
