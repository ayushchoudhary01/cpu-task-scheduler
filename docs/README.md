# Documentation

Everything about this project that does not belong in the top-level README.

## Getting started

| Page | What it covers |
|------|----------------|
| [Installing a compiler](getting-started/installing-a-compiler.md) | Windows has no C++ compiler by default. This is how to get one. |
| [Building](getting-started/building.md) | CMake, the two build commands, and running the tests. |
| [Running it](getting-started/running.md) | Every command line option, the workload file format, and the web interface. |

## The algorithms

| Page | Summary |
|------|---------|
| [Overview and comparison](algorithms/README.md) | All five side by side, and how to choose between them. |
| [FCFS](algorithms/fcfs.md) | First come, first served. The baseline. |
| [SJF](algorithms/sjf.md) | Shortest job first. Optimal average waiting time. |
| [SRTF](algorithms/srtf.md) | Preemptive shortest job first. |
| [Round Robin](algorithms/round-robin.md) | Fixed turns. Fair, responsive, slow. |
| [Priority and aging](algorithms/priority-and-aging.md) | Importance first, with a cure for starvation. |

## How it is built

| Page | What it covers |
|------|----------------|
| [Architecture overview](architecture/overview.md) | The three layers and how a request flows through them. |
| [The engine](architecture/the-engine.md) | The tick loop, and the invariants it guarantees. |
| [Scheduling policies](architecture/scheduling-policies.md) | The interface every algorithm implements. |
| [JSON contract](architecture/json-contract.md) | The exact shape passed from C++ to the browser. |

## Working on it

| Page | What it covers |
|------|----------------|
| [Project structure](development/project-structure.md) | What lives where, and why. |
| [Testing](development/testing.md) | The test harness and what is actually being checked. |
| [Adding an algorithm](development/adding-an-algorithm.md) | Step by step, with a worked example. |

## When something breaks

| Page | What it covers |
|------|----------------|
| [Build and toolchain](troubleshooting/build-and-toolchain.md) | Compiler not found, CMake failures, Smart App Control. |
| [Running the web app](troubleshooting/running-the-web-app.md) | Blank pages, hanging requests, port conflicts. |
| [Problems we hit](troubleshooting/problems-we-hit.md) | Every real bug and blocker during development, and what fixed it. |

## Quick reference

```bash
cmake -S . -B build -G Ninja      # configure, once
cmake --build build               # compile
./build/bin/scheduler_tests.exe   # run the tests
./build/bin/scheduler.exe --help  # see the options
cd frontend && npm run dev        # web interface on http://localhost:5173
```
