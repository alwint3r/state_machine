cmake_minimum_required(VERSION 3.20)
project(state_machine CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Header-only library
add_library(state_machine INTERFACE)
target_include_directories(state_machine INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(state_machine INTERFACE cxx_std_23)

option(STATE_MACHINE_BUILD_EXAMPLES "Build example project" ON)
option(STATE_MACHINE_BUILD_TESTS "Build tests" ON)

if(STATE_MACHINE_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()

if(STATE_MACHINE_BUILD_TESTS)
  add_subdirectory(tests)
endif()
