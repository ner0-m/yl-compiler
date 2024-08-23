cmake_minimum_required(VERSION 3.1...3.29)

project(yl-compiler VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include(FetchContent)

# Also requires one of: libbfd (gnu binutils), libdwarf, libdw (elfutils)
FetchContent_Declare(backward
    GIT_REPOSITORY https://github.com/bombela/backward-cpp
    GIT_TAG master  # or a version tag, such as v1.6
    SYSTEM          # optional, the Backward include directory will be treated as system directory
)
FetchContent_MakeAvailable(backward)

# Add Backward to your target (either Backward::Interface, Backward::Object, or Backward::Backward)

add_executable(yl src/main.cpp src/lexer.cpp src/parser.cpp src/utils.cpp
    src/ast.cpp src/sema.cpp src/codegen.cpp src/constexpr.cpp src/cfg.cpp)
target_link_libraries(yl PUBLIC LLVM Backward::Backward)
target_include_directories(yl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include/yl>
)

add_custom_target(check
    COMMAND lit -v --path=${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test
    DEPENDS yl
    USES_TERMINAL)
