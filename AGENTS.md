# AGENTS.md

Instructions for AI coding agents (Claude Code, Codex, Cursor, etc.) working in this repository.

## Project purpose

`fx-montecarlo` is a C++20 library for Monte Carlo simulation of exchange rates: stochastic models
(Garman–Kohlhagen GBM, exponential Ornstein–Uhlenbeck), a reproducible path simulator with
antithetic variates, risk measures (VaR, Expected Shortfall) and Monte Carlo pricing of FX options
checked against the Garman–Kohlhagen closed form. It is a course project for quantitative analysis
and algorithmic trading, so numerical correctness matters more than features.

## Layout

| Path                 | Contents                                                         |
|----------------------|------------------------------------------------------------------|
| `include/fxmc/`      | Public headers. Every public symbol has a Doxygen comment.       |
| `src/`               | Library implementation (`fxmc` static library).                  |
| `app/`               | `fxmc_demo` executable.                                          |
| `tests/`             | GoogleTest unit tests (`fxmc_tests`), one file per header.       |
| `bench/`             | Google Benchmark benchmarks (`fxmc_bench`).                      |
| `.github/workflows/` | CI (`ci.yml`) and documentation deploy (`docs.yml`).             |
| `Doxyfile`           | Doxygen config; warnings are treated as errors.                  |

## Commands

```bash
# configure + build (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# unit tests
ctest --test-dir build --output-on-failure

# benchmark
./build/bench/fxmc_bench

# demo
./build/fxmc_demo

# documentation (must produce no warnings)
doxygen Doxyfile

# formatting
clang-format -i include/fxmc/*.hpp src/*.cpp app/*.cpp tests/*.cpp bench/*.cpp
```

## Code style

- C++20, no compiler extensions. The library compiles with `-Wall -Wextra -Wpedantic -Wconversion`
  and must stay warning-free.
- Formatting follows `.clang-format` (4-space indent, 100 columns).
- Naming: `snake_case` for functions, variables and files; `PascalCase` for types;
  trailing underscore for private members (`params_`).
- Everything lives in `namespace fxmc`. Headers use `#pragma once`.
- Rates are continuously compounded and annualised; time is in years; an exchange rate is domestic
  currency per one unit of foreign currency. Keep these conventions in new code and docs.
- New models must satisfy the `fxmc::FxModel` concept and use an exact (or clearly documented)
  discretisation.
- Document every public class, function and type with Doxygen (`@brief`, `@param`, `@return`,
  `@throws`). Missing docs fail the documentation build.
- Report invalid input with exceptions from `<stdexcept>`; do not print from library code.

## Rules for the agent

- Before finishing any task, build the project and run the tests; both must pass. If you changed
  public headers, also run `doxygen Doxyfile` and make sure it reports no warnings.
- Every behavior change or bug fix comes with a unit test in `tests/`.
- Statistical tests must use a fixed seed and a tolerance expressed in standard errors (4 SE);
  never loosen a tolerance just to make a test pass, and never compare against values produced by
  the code under test.
- If you touch the simulation loop or the models, run the benchmark before and after and report the
  numbers.
- Do not change CI/CD workflows (`.github/workflows/`), `Doxyfile` or dependency versions in
  `CMakeLists.txt` unless explicitly asked.
- Do not change model conventions (quote direction, compounding, the meaning of seeds) without
  being asked.
- Do not add new third-party dependencies without asking.
- Do not commit build output (`build/`) or generated documentation (`apidocs/`).
- Keep changes small and focused; explain what you changed and why.
