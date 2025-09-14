cmake_minimum_required(VERSION 3.20)

# Basic tests (no external framework)
add_executable(fsm_tests test_state_machine.cpp)
target_link_libraries(fsm_tests PRIVATE state_machine)

add_executable(test_mdspan test_mdspan.cpp)
target_link_libraries(test_mdspan PRIVATE state_machine)
