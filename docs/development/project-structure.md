# Project structure

```
cpu-task-scheduler/
├── CMakeLists.txt              the whole C++ build, one file
├── core/                       scheduling logic - no I/O, no dependencies
│   ├── Process.hpp             a process as the user describes it
│   ├── TimeSlice.hpp           one block of the Gantt chart
│   ├── Timeline.{hpp,cpp}      records what the CPU did, merges runs
│   ├── Metrics.{hpp,cpp}       waiting / turnaround / response / averages
│   ├── SchedulingPolicy.hpp    the interface every algorithm implements
│   ├── Engine.{hpp,cpp}        the tick loop
│   ├── PolicyRegistry.{hpp,cpp}  name -> policy
│   ├── WorkloadGenerator.{hpp,cpp}  reproducible random workloads
│   └── policies/
│       ├── Select.hpp          the shared tie-break, implemented once
│       ├── FcfsPolicy.{hpp,cpp}
│       ├── SjfPolicy.{hpp,cpp}
│       ├── SrtfPolicy.{hpp,cpp}
│       ├── RoundRobinPolicy.{hpp,cpp}
│       └── PriorityPolicy.{hpp,cpp}
├── cli/                        the command line program
│   ├── WorkloadParser.{hpp,cpp}  reads workload files
│   ├── CommandLine.{hpp,cpp}     reads arguments
│   ├── TextReport.{hpp,cpp}      Gantt chart and tables
│   ├── JsonReport.{hpp,cpp}      machine readable output
│   └── main.cpp                  wiring only
├── tests/
│   ├── Check.hpp               the entire test framework, ~30 lines
│   ├── main.cpp                runs each group, reports the tally
│   ├── CoreTests.cpp           Timeline, Metrics, Engine
│   ├── AlgorithmTests.cpp      FCFS, SJF, SRTF, Round Robin
│   ├── PriorityTests.cpp       Priority, aging, the registry
│   ├── CliTests.cpp            parsers and output rendering
│   └── RobustnessTests.cpp     invariants, generator, edge cases
├── examples/
│   ├── sample.txt              a small mixed workload
│   └── starvation.txt          demonstrates starvation and aging
├── frontend/
│   ├── server/                 Express: validates, spawns, returns
│   │   ├── index.ts            routes
│   │   ├── scheduler.ts        finds and runs the binary safely
│   │   └── validate.ts         checks requests, collects every problem
│   ├── src/
│   │   ├── types.ts            the JSON contract, in TypeScript
│   │   ├── api.ts              fetch wrappers
│   │   ├── theme.ts            the validated chart palette
│   │   ├── algorithms.ts       names and descriptions
│   │   ├── App.tsx             layout, tabs, shared workload state
│   │   ├── views/              SimulatorView, CompareView
│   │   └── components/         GanttChart, ProcessEditor, ResultView, ...
│   └── vite.config.ts
└── docs/                       you are here
```

## Why libraries rather than one executable

`CMakeLists.txt` builds two libraries and two programs:

```
scheduler_core  ──┬──> scheduler_cli ──┬──> scheduler.exe
                  │                    └──> scheduler_tests.exe
```

If everything were compiled straight into `scheduler.exe`, the tests could only
poke at it from outside - running the program and checking its printed output.
Making the logic a library means tests link against **exactly the code that
ships** and can call it directly.

That is why `cli/` is a library too: `parseWorkload` and `parseCommandLine` are
tested by calling them with strings, not by running the program and reading
stderr.

## Header and source conventions

- `.hpp` for headers, `.cpp` for sources, `#pragma once`
- Types with no behaviour (`Process`, `TimeSlice`) are header-only structs
- Everything is in `namespace scheduler`; policies in `scheduler::policies`;
  the CLI in `namespace cli`
- Helpers local to a file go in an anonymous `namespace { }`
- Headers live in `core/` and `cli/`, both on the include path, so includes read
  as `#include "Engine.hpp"` and `#include "policies/Select.hpp"`

## Where to put a change

| Change | Where |
|--------|-------|
| A new algorithm | `core/policies/`, plus the registry - see [adding an algorithm](adding-an-algorithm.md) |
| A new metric | `core/Metrics.hpp`, then both renderers and `types.ts` |
| A new CLI option | `cli/CommandLine.{hpp,cpp}`, then `main.cpp` |
| Changing the terminal output | `cli/TextReport.cpp` |
| Changing the JSON | `cli/JsonReport.cpp` **and** `frontend/src/types.ts` |
| Anything visual | `frontend/src/` |

## Generated directories

`build/`, `build-dbg/`, `frontend/node_modules/` and `frontend/dist/` are all
generated and gitignored. Deleting any of them is safe.
