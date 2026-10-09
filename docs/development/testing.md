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

## The web layer

A second suite, run from `frontend/`:

```bash
cd frontend && npm test
```

```
tests 26
pass 26
fail 0
```

It uses **Node's built-in test runner** - `node:test` and `node:assert` - so it
costs no new dependency. `tsx` is already present and loads the TypeScript.

Two files, doing different jobs:

- [`tests/validate.test.ts`](../../frontend/tests/validate.test.ts) - unit tests
  for request validation and for turning a request into command line
  arguments. No server, no child process.
- [`tests/api.test.ts`](../../frontend/tests/api.test.ts) - **integration**
  tests. They start the real server on a throwaway port (`listen(0)`, so they
  never collide with a development server) and let it run the real C++ program.

The integration tests assert the same hand-computed numbers the C++ tests use -
`SRTF` averaging 2.50 waiting and 1.25 response on the sample workload. That is
the point: if the browser and the terminal ever disagree, these fail.

They **skip rather than fail** when the C++ has not been built, because a
missing binary is a setup problem rather than a broken API.

This required one change to the server: `createApp()` lives in
[`server/app.ts`](../../frontend/server/app.ts) and `index.ts` only starts it.
Previously importing the server started it listening, which made it untestable.

## Checking the threading

Comparison mode runs each algorithm on its own thread
([`core/BatchRunner.cpp`](../../core/BatchRunner.cpp)) and takes no locks, on
the grounds that the shared data is read-only and each thread writes to its own
slot. That is a claim worth proving rather than asserting:

```bash
g++ -std=c++20 -g -O1 -fsanitize=thread -I core -I cli -I tests \
    core/*.cpp core/policies/*.cpp cli/*.cpp tests/*.cpp -o tsan_tests -pthread
./tsan_tests
```

ThreadSanitizer reports **no data races** across the whole suite. Run against a
deliberately racy control program, the same build flags report two - so the
detector is working, and the zero means something.

There are also ordinary tests for it: results identical to running the
algorithms one at a time, results in the requested order rather than the order
they finished, and twenty repeats producing identical output.

## What is still not covered

- **No browser tests.** The React components are checked by hand. Component
  tests would need a test renderer and a DOM, which is a real dependency.
- **Performance is only sanity-checked** - a 500 process workload completes -
  not benchmarked.
- **The JSON contract is not validated at the TypeScript boundary.** JSON is
  cast to an interface, so a mismatch between server and browser would compile
  fine. The integration tests catch it in practice by asserting on real
  responses, but nothing checks the types themselves. See
  [the JSON contract](../architecture/json-contract.md#changing-the-shape).

## Adding a test

1. Write the function in the relevant file, inside the anonymous namespace.
2. Call it from that file's `run...Tests()`.
3. Work out expected values **by hand** before running anything.

For a whole new file, add it to `add_executable(scheduler_tests ...)` in
`CMakeLists.txt` and declare its entry point in `tests/Tests.hpp`.
