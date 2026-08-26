# Adding an algorithm

Five files touched, none of them the engine. Worked example: **Longest Job
First** - the opposite of SJF, and a genuinely bad scheduler, which makes it a
clear demonstration.

## 1. Write the policy

`core/policies/LjfPolicy.hpp`:

```cpp
#pragma once

#include "SchedulingPolicy.hpp"

namespace scheduler::policies {

// Longest Job First. Runs whichever waiting process needs the most CPU time.
// Included as a counter-example: it maximises average waiting time, which is
// exactly why SJF minimises it.
class LjfPolicy : public SchedulingPolicy {
public:
    std::string name() const override { return "LJF"; }

    std::size_t choose(const std::vector<ReadyProcess>& ready, int currentTime) const override;
};

}  // namespace scheduler::policies
```

`core/policies/LjfPolicy.cpp`:

```cpp
#include "policies/LjfPolicy.hpp"

#include "policies/Select.hpp"

namespace scheduler::policies {

std::size_t LjfPolicy::choose(const std::vector<ReadyProcess>& ready, int currentTime) const {
    (void)currentTime;

    return selectBest(ready, [](const ReadyProcess& a, const ReadyProcess& b) {
        return a.process->burstTime > b.process->burstTime;
    });
}

}  // namespace scheduler::policies
```

That is the entire algorithm - one comparison, reversed from SJF's. Not
overriding `timeSlice()` or `shouldPreempt()` makes it non-preemptive with no
time limit.

**Use `selectBest`.** It applies the shared tie-break for you, so ties resolve
in queue order and results stay reproducible. Do not write your own loop.

## 2. Register it

In [`core/PolicyRegistry.cpp`](../../core/PolicyRegistry.cpp):

```cpp
#include "policies/LjfPolicy.hpp"
...
if (key == "LJF") return std::make_unique<policies::LjfPolicy>();
```

and in `availablePolicies()`:

```cpp
return {"FCFS", "SJF", "SRTF", "RR", "Priority", "LJF"};
```

That second line is all that is needed for `--list`, `--compare all`, and the
CLI's validation to pick it up. Nothing in `main.cpp` changes.

## 3. Add it to the build

In `CMakeLists.txt`, inside `add_library(scheduler_core ...)`:

```cmake
    core/policies/LjfPolicy.cpp
```

## 4. Test it

Work the expected schedule out by hand *first*. For `P1(0,5)`, `P2(1,3)`,
`P3(2,1)`: `P1` starts alone and runs to completion; at tick 5 both `P2` (3) and
`P3` (1) wait, so LJF takes `P2`.

```
tick  0   1   2   3   4   5   6   7   8
      P1  P1  P1  P1  P1  P2  P2  P2  P3
```

In [`tests/AlgorithmTests.cpp`](../../tests/AlgorithmTests.cpp):

```cpp
void testLjfPrefersLongerJobs() {
    std::cout << "LJF picks the longest waiting job\n";

    SimulationResult result = runSimulation(kMixedWorkload, LjfPolicy());

    checkEqual(result.timeline.slices()[1].processId, std::string("P2"), "P2 before P3");
    checkEqual(result.metrics[2].waitingTime, 6, "P3 waits longest");

    // The point of the algorithm: it is worse than SJF, on purpose.
    SimulationResult sjf = runSimulation(kMixedWorkload, SjfPolicy());
    check(result.averages.waitingTime > sjf.averages.waitingTime, "LJF is worse than SJF");
}
```

Call it from `runAlgorithmTests()`.

The invariant tests in `RobustnessTests.cpp` pick it up automatically, because
they iterate `availablePolicies()` - so the new algorithm is immediately checked
against all six invariants on 20 generated workloads with no extra work.

## 5. Add it to the web interface

Two places:

- [`frontend/src/algorithms.ts`](../../frontend/src/algorithms.ts) - add `"LJF"`
  to `ALGORITHMS` and a line to `DESCRIPTIONS`
- [`frontend/server/validate.ts`](../../frontend/server/validate.ts) - add
  `"LJF"` to `KNOWN_ALGORITHMS`

Both lists exist so the browser and server reject bad input without asking the
C++ program. Forgetting the server one means the UI offers an algorithm the API
refuses.

## Then

```bash
cmake -S . -B build -G Ninja      # re-run: CMakeLists.txt changed
cmake --build build
./build/bin/scheduler_tests.exe
./build/bin/scheduler.exe --compare all -i examples/sample.txt
```

## If your algorithm needs a setting

Like Round Robin's quantum or Priority's aging rate:

1. Add a field to `PolicyOptions` in `core/PolicyRegistry.hpp`.
2. Take it as a constructor argument, and clamp it defensively - policies get
   validated input, but a backstop costs one line.
3. Pass it through in `makePolicy`.
4. Include it in `name()`, as `RoundRobinPolicy` does with `"RR(q=2)"`, so
   comparisons stay readable.
5. Add the CLI flag in `cli/CommandLine.cpp`, and surface it in the UI when that
   algorithm is selected.

## If your algorithm is preemptive

Override one or both:

- `int timeSlice() const` - a fixed number of ticks, then yield. Round Robin.
- `bool shouldPreempt(running, ready, currentTime) const` - decide per tick.
  SRTF.

Prefer a strict comparison in `shouldPreempt`. Using `<=` where `<` would do
causes a context switch that changes nothing - see
[SRTF](../algorithms/srtf.md).

## What you never touch

`core/Engine.cpp`. If an algorithm seems to need engine changes, the interface
is probably the wrong shape - that is a design discussion, not a patch.

Multilevel feedback queues are the interesting test of this. MLFQ needs several
queues with demotion between them, which the current `ReadyProcess` cannot
express, since a policy holds no state. It would need a per-process queue level
in `ReadyProcess` that the engine carries but does not interpret. That is the
one extension the design has room for but has not made.
