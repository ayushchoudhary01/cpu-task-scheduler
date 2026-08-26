# First Come First Served (FCFS)

The simplest scheduler there is: whoever has been waiting longest runs, and runs
until it finishes.

**Preemptive:** no.
**Source:** [`core/policies/FcfsPolicy.cpp`](../../core/policies/FcfsPolicy.cpp)

## How it works

1. When the CPU is free, take the process at the front of the ready queue.
2. Let it run to completion.
3. Repeat.

That is the whole algorithm. There is no decision to make beyond "who was here
first".

## In this codebase

FCFS is the one algorithm with *no preference of its own*:

```cpp
std::size_t FcfsPolicy::choose(const std::vector<ReadyProcess>& ready, int) const {
    // FCFS has no preference of its own - arrival order *is* the rule, and
    // that is exactly what the shared tie-break already does.
    return selectBest(ready, [](const ReadyProcess&, const ReadyProcess&) { return false; });
}
```

`selectBest` falls back to the shared tie-break - earliest into the ready queue
wins - whenever the comparison says "these are equal". FCFS says everything is
equal, so arrival order falls straight out. It overrides neither `timeSlice()`
nor `shouldPreempt()`, so it inherits "no time limit" and "never interrupt".

## Worked example

`P1` arrives at 0 needing 5, `P2` at 1 needing 3, `P3` at 2 needing 1.

```
tick  0   1   2   3   4   5   6   7   8
      P1  P1  P1  P1  P1  P2  P2  P2  P3
```

| Process | Arrival | Burst | Completion | Turnaround | Waiting | Response |
|---------|---------|-------|-----------|-----------|---------|----------|
| P1 | 0 | 5 | 5 | 5 | 0 | 0 |
| P2 | 1 | 3 | 8 | 7 | 4 | 4 |
| P3 | 2 | 1 | 9 | 7 | **6** | 6 |

Average waiting time **3.33**.

Look at `P3`. It needs a single tick of CPU, but it arrived behind a five-tick
job and waits six ticks to get it. That is the **convoy effect**: one long
process at the front delays everything behind it, no matter how small.

## Properties

| | |
|---|---|
| Average waiting time | worst of the five on most workloads |
| Starvation | impossible - the queue always moves forward |
| Context switches | minimum possible: one per process |
| Predictability | perfect; the order is known in advance |
| Selection cost | O(n) as written, O(1) with a plain queue |

## When it is the right choice

- Batch processing where nothing is interactive and throughput is what matters.
- Systems where switching cost is high enough to dominate.
- As a baseline. Every other algorithm's benefit is measured against this one.

## When it is not

Anything interactive. A single long-running job makes the whole system feel
frozen to everything queued behind it.

## Tests

In [`tests/AlgorithmTests.cpp`](../../tests/AlgorithmTests.cpp):

- `testFcfsRunsInArrivalOrder` - the order and every metric above, hand-computed
- `testFcfsIdlesBetweenProcesses` - a gap with nothing ready produces an idle
  slice rather than skipping time
