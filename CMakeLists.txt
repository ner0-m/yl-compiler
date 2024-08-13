cmake_minimum_required(VERSION 3.1...3.29)

project(Compiler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_executable(cxx-comp src/main.cpp src/lexer.cpp src/parser.cpp src/utils.cpp src/ast.cpp src/sema.cpp src/codegen.cpp)
target_link_libraries(cxx-comp PUBLIC LLVM)
target_include_directories(cxx-comp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include/cxx-comp>
)

add_custom_target(check
    COMMAND lit -v --path=${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test
    DEPENDS cxx-comp
    USES_TERMINAL)
