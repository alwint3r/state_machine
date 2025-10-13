# Repository Guidelines

## Project Structure & Module Organization
- Library headers live in `include/state_machine`, organized by feature; keep new utilities header-only and self-contained.
- Example application resides in `example/` and builds the `state_machine_example` demo; mirror its layout for additional samples.
- Standalone tests sit under `tests/`; each test is a self-sufficient executable with its own `main`.
- Generated CMake artifacts belong in `build/`; avoid checking in contents from that directory.

## Build, Test, and Development Commands
- Configure once with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`; rerun if toolchain settings change.
- Build everything via `cmake --build build` or `./build.sh`; both ensure headers remain interface-only.
- Run the example with `./build/example/state_machine_example` when `STATE_MACHINE_BUILD_EXAMPLES=ON`.
- Execute tests individually: `./build/tests/fsm_tests` and `./build/tests/test_mdspan` (enable via `STATE_MACHINE_BUILD_TESTS=ON`).

## Coding Style & Naming Conventions
- Target C++23 with `std::mdspan`, `std::expected`, and `std::print`; prefer constexpr and STL algorithms over raw loops.
- Indent two spaces; place braces on the same line; use `.hpp` headers that include what they use.
- Name types and enums in PascalCase, functions in lowerCamelCase, and private members with a trailing `_` (e.g., `currentState_`).
- Limit comments to clarifying complex intent; ensure new headers compile standalone.

## Testing Guidelines
- Keep tests framework-free; return non-zero on failure and print `[PASS]/[FAIL]` summaries.
- Name sources `test_<topic>.cpp` at repository root and register each with `add_executable` in CMake.
- Run tests from the build tree; clear failures before submitting changes.

## Commit & Pull Request Guidelines
- Write commits with imperative subjects ≤72 chars (e.g., "Add transition guard check"); split refactors from behavior changes.
- For pull requests, describe scope, link issues, and include before/after output or commands (`cmake --build build`, `./build/tests/fsm_tests`).
- Ensure example and tests pass locally before requesting review; document any skipped steps and rationale.

## Security & Configuration Tips
- Validate enum ranges and sentinel values; avoid undefined behavior in state transitions.
- Keep public APIs minimal, header-only, and free of dynamic allocation unless isolated behind clear boundaries.
