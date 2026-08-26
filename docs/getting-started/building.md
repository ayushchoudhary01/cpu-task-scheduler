# Building

You need a C++20 compiler and CMake. If `g++ --version` prints nothing, start at
[installing a compiler](installing-a-compiler.md) - WinLibs includes CMake and
Ninja as well.

## The two commands

```bash
cmake -S . -B build -G Ninja
```

```bash
cmake --build build
```

The first **configures**: it reads `CMakeLists.txt`, works out which compiler
you have, and writes actual build instructions into `build/`. You only need to
re-run it when `CMakeLists.txt` changes.

The second **compiles**. This is the one you will use constantly.

`build/` is entirely generated and is gitignored. Delete it whenever you like
and re-run both commands to get it back; nothing you write ever lives there.

## What gets built

| Output | What it is |
|--------|------------|
| `build/bin/scheduler.exe` | the command line program |
| `build/bin/scheduler_tests.exe` | the test suite |
| `build/libscheduler_core.a` | the scheduling logic, linked into both |

## Running the tests

```bash
./build/bin/scheduler_tests.exe
```

Expect a tally like:

```
258/258 checks passed
```

It exits non-zero if anything fails, so it drops straight into CI if you ever
want that. You can also run it through CTest:

```bash
ctest --test-dir build --output-on-failure
```

## Why CMake at all

For one source file you could compile by hand:

```bash
g++ cli/main.cpp -o scheduler.exe
```

With around thirty files across `core/`, `cli/` and `tests/`, plus flags for the
language standard and warnings, that stops being practical - and everyone
building the project would need to know the incantation. `CMakeLists.txt`
records it once. It is roughly the C++ equivalent of `package.json`.

## What the build file does

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

C++20, and fail loudly rather than silently falling back to an older standard.

```cmake
add_library(scheduler_core ...)
```

The scheduling logic is a library, not part of the executable. That is what lets
the tests exercise the real code rather than a copy of it.

```cmake
target_compile_options(scheduler_core PRIVATE -Wall -Wextra)
```

Warnings on. C++ will happily compile code with real bugs in it and say nothing
by default.

## A second build directory

You may see `build-dbg/` referenced. Nothing requires it - it exists because
Smart App Control on Windows sometimes blocks a freshly linked binary, and a
differently built one often gets a different verdict:

```bash
cmake -S . -B build-dbg -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-dbg
```

**If you use two build directories, keep both current.** The web server looks in
`build/` first, and a stale binary there produces confusing errors - see
[problems we hit](../troubleshooting/problems-we-hit.md#a-stale-binary-in-a-second-build-directory).

## Next

[Running it](running.md).
