cmake_minimum_required(VERSION 3.20)
project(cast_history_clang_tool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include(HandleLLVMOptions)

add_executable(cast-history-tool
  src/main.cpp
)

target_include_directories(cast-history-tool PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
  ${CLANG_INCLUDE_DIRS}
)

target_compile_definitions(cast-history-tool PRIVATE ${LLVM_DEFINITIONS})

target_link_libraries(cast-history-tool PRIVATE
  clangAST
  clangBasic
  clangFrontend
  clangLex
  clangRewrite
  clangSerialization
  clangTooling
  clangToolingCore
)

if (MSVC)
  target_compile_options(cast-history-tool PRIVATE /W4)
else()
  target_compile_options(cast-history-tool PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Pointer-coverage gates (docs/pointer-coverage-plan.md).
#   ctest -R pointer-coverage-ratchet   nothing that passed in baseline.json may fail
#   ctest -R pointer-coverage-phase     exit gates of the phases sealed below
# Sealing a phase = appending it to POINTER_COVERAGE_SEALED_PHASES in the same
# change that turns its gates green.  Sealed phases can never go red again.
set(POINTER_COVERAGE_SEALED_PHASES "" CACHE STRING
    "Phases (P0;P1;...) whose pointer-coverage gates are required to be green")
enable_testing()
find_package(Python3 COMPONENTS Interpreter)
if (Python3_FOUND)
  set(POINTER_COVERAGE_GATES
      ${CMAKE_CURRENT_SOURCE_DIR}/gates/pointer-coverage/run_gates.py)
  add_test(NAME pointer-coverage-ratchet
    COMMAND ${Python3_EXECUTABLE} ${POINTER_COVERAGE_GATES}
            --tool $<TARGET_FILE:cast-history-tool>)
  foreach(phase IN LISTS POINTER_COVERAGE_SEALED_PHASES)
    add_test(NAME pointer-coverage-phase-${phase}
      COMMAND ${Python3_EXECUTABLE} ${POINTER_COVERAGE_GATES}
              --tool $<TARGET_FILE:cast-history-tool>
              --phase ${phase} --require-phase ${phase})
  endforeach()
endif()
