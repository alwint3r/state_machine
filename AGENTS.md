# Repository Guidelines

## Project Structure & Module Organization
- `include/state_machine/*.hpp`: Header-only C++23 FSM and enum helpers.
- `example/`: Concrete example project (`state_machine_example`).
- `tests/`: Self-contained tests (no framework).
- `CMakeLists.txt`: Declares `state_machine` INTERFACE target with optional subdirs.
- `build/`: CMake build artifacts (generated).
- `build.sh`, `run.sh`: Convenience scripts for local development.

## Build, Test, and Development Commands
- First configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build all: `cmake --build build` or `./build.sh`
- Run example: `./build/example/state_machine_example` or `./run.sh` (if `STATE_MACHINE_BUILD_EXAMPLES=ON`)
- Run tests: `./build/tests/fsm_tests` and `./build/tests/test_mdspan` (if `STATE_MACHINE_BUILD_TESTS=ON`)
- Reconfigure release: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`

Toolchain: C++23 with `std::mdspan`, `std::expected`, and `std::print` support (CMake ≥ 3.20; modern Clang/GCC/libc++/libstdc++).

## Coding Style & Naming Conventions
- Language: C++23; header-only library style; public headers under `include/state_machine`.
- Indentation: 2 spaces; braces on the same line.
- Naming: types/enums `PascalCase` (e.g., `TransitionType`), functions `lowerCamelCase` (e.g., `processEvent`), private members `camelCase_` suffix (e.g., `currentState_`).
- Headers: prefer `.hpp`; keep headers self-contained and include what you use.
- Prefer `constexpr`, spans/`mdspan`, and STL algorithms over raw loops where sensible.

## Testing Guidelines
- Tests are simple executables using the standard library (no third-party framework).
- File naming: `test_<topic>.cpp` at repo root.
- Add tests to CMake by creating an executable, for example:
  `add_executable(my_tests test_my_feature.cpp)`
- Run with `./build/<target>` and return non-zero on failure.
- Aim for meaningful console output: `[PASS]/[FAIL]` and a final summary.

## Commit & Pull Request Guidelines
- Commits: short, imperative subjects (e.g., "Add enum utilities", "Fix transition guard"); keep ≤ 72 chars; include a rationale body when needed.
- Scope changes narrowly; separate refactors from behavior changes.
- PRs: include a concise description, linked issues, before/after output for behavior, and instructions to reproduce (commands used).
- Ensure `cmake --build build` succeeds and `./build/fsm_tests` passes before requesting review.

## Security & Configuration Tips
- Avoid undefined behavior; validate enum bounds and sentinel usage (`MAX_VALUE`).
- Keep public APIs header-only and minimal; prefer `std::function` only on boundaries.
