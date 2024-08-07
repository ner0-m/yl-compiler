cmake_minimum_required(VERSION 3.1...3.29)

project(Compiler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(cxx-comp src/main.cpp src/lexer.cpp src/parser.cpp src/utils.cpp src/ast.cpp)
target_include_directories(cxx-comp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include/cxx-comp>  # <prefix>/include/mylib
)
