# state_machine — Header‑only C++23 FSM

A small, header‑only finite state machine (FSM) library and enum helpers for C++23. 
It uses modern C++ features like `std::mdspan` for the transition table, 
`std::expected` for error reporting, and `std::print` for examples/tests.

- Library target: `state_machine` (INTERFACE)
- Public headers: `include/state_machine/*.hpp`
- Example executable: `state_machine_example`
- Tests: self‑contained executables (no third‑party framework)

## Requirements
- CMake ≥ 3.20
- C++23 compiler and standard library with support for:
  - `<mdspan>`
  - `<expected>`
  - `<print>`
- Modern Clang/GCC with matching libc++/libstdc++ that provide the above headers

## Build
- Configure (Debug):
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build all:
  - `cmake --build build` or `./build.sh`
- Options:
  - `STATE_MACHINE_BUILD_EXAMPLES=ON|OFF` (default ON)
  - `STATE_MACHINE_BUILD_TESTS=ON|OFF` (default ON)
- Reconfigure Release:
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`

## Run
- Example:
  - `./build/example/state_machine_example` or `./run.sh`
- Tests:
  - `./build/tests/fsm_tests`
  - `./build/tests/test_mdspan`

## Project Layout
- `include/state_machine/EnumUtils.hpp` — enum helpers for `MAX_VALUE`‑sentinel enums
- `include/state_machine/StateMachine.hpp` — FSM implementation
- `example/` — minimal, runnable example
- `tests/` — simple executables using only the standard library
- `CMakeLists.txt` — declares `state_machine` INTERFACE target and optional subdirs

## Quick Start
Minimal example showing states, events, transitions, guard, and callbacks.

```cpp
#include <state_machine/StateMachine.hpp>
#include <print>

namespace sm = state_machine;

enum class State { Idle, Active, Stopped, Canceled, MAX_VALUE };
enum class Event { Start, Timeout, Cancel, Restart, MAX_VALUE };

int main() {
  sm::FSM<State, Event> fsm(State::Idle);
  fsm.init();

  // Transitions
  fsm.enableTransition(State::Idle,   State::Active,  Event::Start);
  fsm.enableTransition(State::Active, State::Stopped, Event::Timeout);

  // Optional guard on a state
  fsm.attachTransitionGuard(State::Idle, [](State cur, State next, Event ev) {
    (void)cur; (void)next; (void)ev;
    return true; // allow transition
  });

  // Optional callbacks
  fsm.attachOnExitStateCallback(State::Idle, [](sm::TransitionType, State cur, State nxt, Event ev) {
    std::println("Exit: {} -> {} on {}", std::to_underlying(cur), std::to_underlying(nxt), std::to_underlying(ev));
  });
  fsm.attachOnEnterStateCallback(State::Active, [](sm::TransitionType, State prev, State nxt, Event ev) {
    std::println("Enter: {} -> {} on {}", std::to_underlying(prev), std::to_underlying(nxt), std::to_underlying(ev));
  });

  // Drive the FSM
  if (auto r = fsm.processEvent(Event::Start)) {
    std::println("Now in state {}", std::to_underlying(*r));
  }
}
```

## API Overview
- `template <StateID S, EventID E> class FSM` (header‑only)
  - `FSM(S initial)` — construct with initial state
  - `void init()` — clear transitions/guards/callbacks (sets all transitions to `S::MAX_VALUE`)
  - `void enableTransition(S from, S to, E onEvent)`
  - `void disableTransition(S from, S to, E onEvent)`
  - `std::expected<S, ProcessEventErr> processEvent(E event)`
    - `ProcessEventErr::NoNextStateFound` if no transition
    - `ProcessEventErr::TransitionForbidden` if a guard blocks it
  - `void attachOnEnterStateCallback(S state, TransitionCallbackFn<S,E>)`
  - `void attachOnExitStateCallback(S state, TransitionCallbackFn<S,E>)`
  - `void attachTransitionGuard(S state, TransitionGuard<S,E>)`
  - `S getCurrentState()`
- Helpers in `EnumUtils.hpp` assume enums are `0..MAX_VALUE-1` with `MAX_VALUE` sentinel

## Notes & Tips
- Define states/events as `enum class` with a `MAX_VALUE` sentinel; values must be contiguous from 0.
- Prefer small, fast callbacks; `std::function` is used at the boundary for convenience.
- The transition table is an `std::mdspan` over a flat storage array for cache‑friendly access.

