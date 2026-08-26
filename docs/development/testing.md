# Testing

```bash
cmake --build build && ./build/bin/scheduler_tests.exe
```

```
258/258 checks passed
```

Exits non-zero on failure. Also runnable through CTest:

```bash
ctest --test-dir build --output-on-failure
```

## The framework

There isn't one. [`tests/Check.hpp`](../../tests/Check.hpp) is about 30 lines:

```cpp
template <typename Actual, typename Expected>
void checkEqual(const Actual& actual, const Expected& expected, const std::string& what) {
    ++totalChecks;
    if (!(actual == expected)) {
        ++failedChecks;
        std::cout << "  FAIL: " << what
                  << "  (got " << actual << ", expected " << expected << ")\n";
    }
}
```

`check`, `checkEqual`, and a tally. Printing *got* against *expected* is the only
feature that really matters when something breaks.

A real framework was considered and rejected. Vendoring a 500 KB header into a
project this size is more machinery than the job needs, and this is small enough
to read top to bottom in ten seconds.

## Test files

| File | Covers |
|------|--------|
| [`CoreTests.cpp`](../../tests/CoreTests.cpp) | Timeline merging, metric arithmetic, the engine via stub policies |
| [`AlgorithmTests.cpp`](../../tests/AlgorithmTests.cpp) | FCFS, SJF, SRTF, Round Robin |
| [`PriorityTests.cpp`](../../tests/PriorityTests.cpp) | Priority, aging, the registry |
| [`CliTests.cpp`](../../tests/CliTests.cpp) | Workload parsing, argument parsing, text and JSON rendering |
| [`RobustnessTests.cpp`](../../tests/RobustnessTests.cpp) | Invariants across all algorithms, the generator, edge cases |

Each exposes one entry point; `main.cpp` calls them and reports the tally.

## Four kinds of test

### 1. Golden values, computed by hand

Every expected number is worked out on paper and written into the test:

```cpp
checkEqual(result.metrics[2].waitingTime, 6, "P3 waits 6 - the convoy effect");
```

Never captured from a run. Recording whatever the code produced would enshrine
bugs as expectations and prove nothing.

This is not theoretical. Writing the SRTF tests, one failed:

```
FAIL: P1 finishes at 8  (got 7, expected 8)
```

The code was right; the arithmetic in the test was wrong. A test that recorded
actual output would have "passed" and taught nothing.

### 2. Property tests

Claims about algorithms, asserted rather than left in comments:

```cpp
checkEqual(fcfs.averages.waitingTime, 3.75, "FCFS makes the short jobs queue up");
checkEqual(sjf.averages.waitingTime, 0.75, "SJF clears them first");
check(sjf.averages.waitingTime < fcfs.averages.waitingTime, "SJF wins on average waiting");
```

Also here: Round Robin responds faster than FCFS, SRTF beats SJF on waiting
time, and Round Robin with a huge quantum is *identical* to FCFS.

### 3. Invariants over generated workloads

The strongest test in the suite. `testInvariantsHoldForEveryAlgorithm` runs 20
generated workloads through all 5 algorithms - 100 simulations - and after each
one checks rules that must hold regardless of algorithm:

- busy time equals total burst time
- the timeline starts at 0, with no gaps or overlaps
- nothing finishes before it arrives
- `turnaround == waiting + burst`
- `response <= waiting`
- metrics are in input order

```cpp
checkEqual(runs, 100, "ran 20 workloads through 5 algorithms");
checkEqual(firstFailure, std::string(""), "no invariant was broken");
```

This is cheap fuzzing with a fixed seed, so it is reproducible. It covers
workload shapes nobody would think to write by hand, and `busyTime ==
totalBurst` alone would catch the infinite-loop bug described in
[SRTF](../algorithms/srtf.md#the-bug-this-algorithm-is-famous-for) - that bug
burns ticks on a process that has already finished.

### 4. Stub policies

The engine is tested without any real algorithm, using policies that exist only
to stress it:

```cpp
// Interrupts the running process on every single tick.
class AlwaysPreempt : public SchedulingPolicy { ... };
```

Nothing sane schedules that way. It exists to hammer the preemption path harder
than any real algorithm would, and confirm the engine still terminates cleanly.

## Testing output

Asserting exact ASCII art would be brittle - any spacing change breaks every
test. The Gantt chart tests check **properties** instead:

- borders are the same length as the label row
- processes appear in the order they ran
- the axis starts at 0 and ends at the total time
- a longer burst gets a wider block
- columns stay aligned when ids differ in length (`A` against `LONGNAME`)
- empty input gives empty output, not a crash

That catches real breakage while leaving formatting free to change.

## What is not covered

- **The web layer has no automated tests.** It was checked by driving a real
  browser and comparing every number against the CLI, but there is no test
  suite. That is the biggest gap.
- **Performance is only sanity-checked** - a 500 process workload completes -
  not benchmarked.
- **The JSON contract is not validated at the TypeScript boundary.** JSON is
  cast to an interface, so a mismatch between server and browser would compile
  fine. See [the JSON contract](../architecture/json-contract.md#changing-the-shape).

## Adding a test

1. Write the function in the relevant file, inside the anonymous namespace.
2. Call it from that file's `run...Tests()`.
3. Work out expected values **by hand** before running anything.

For a whole new file, add it to `add_executable(scheduler_tests ...)` in
`CMakeLists.txt` and declare its entry point in `tests/Tests.hpp`.
