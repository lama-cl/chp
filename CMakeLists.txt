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
