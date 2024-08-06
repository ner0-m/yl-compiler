cmake_minimum_required(VERSION 3.1...3.29)

project(Compiler VERSION 0.1.0 LANGUAGES CXX)

add_executable(cxx-comp src/main.cpp)
target_include_directories(cxx-comp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include/cxx-comp>  # <prefix>/include/mylib
)
