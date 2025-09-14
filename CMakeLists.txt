cmake_minimum_required(VERSION 3.20)
project(Main)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(main main.cpp)

# Header-only FSM tests (no third-party libs)
add_executable(fsm_tests test_state_machine.cpp)
