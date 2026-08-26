# Shortest Remaining Time First (SRTF)

[SJF](sjf.md) with the ability to interrupt. If something arrives that would
finish sooner than whatever is running, it takes over immediately.

**Preemptive:** yes.
**Source:** [`core/policies/SrtfPolicy.cpp`](../../core/policies/SrtfPolicy.cpp)

## How it works

Every tick:

1. Admit anything that has arrived.
2. If a waiting process has **less remaining time** than the running one, take
   the CPU away and give it to the shorter one.
3. Otherwise carry on.

The comparison is on *remaining* time, not total burst. A ten-tick job with one
tick left beats a fresh two-tick arrival.

## In this codebase

Two hooks instead of one:

```cpp
std::size_t SrtfPolicy::choose(const std::vector<ReadyProcess>& ready, int) const {
    return selectBest(ready, [](const ReadyProcess& a, const ReadyProcess& b) {
        return a.remainingTime < b.remainingTime;
    });
}

bool SrtfPolicy::shouldPreempt(const ReadyProcess& running,
                               const std::vector<ReadyProcess>& ready, int) const {
    // Step aside only for a process that would genuinely finish sooner.
    // Using a strict "<" matters: on a tie the running process keeps the CPU,
    // which avoids pointless context switches between equal processes.
    for (const ReadyProcess& candidate : ready) {
        if (candidate.remainingTime < running.remainingTime) return true;
    }
    return false;
}
```

The strict `<` is deliberate. With `<=`, a process arriving with exactly the
same remaining time would trigger a switch that changes nothing, cluttering the
Gantt chart and - in a real system - wasting real time.
`testSrtfDoesNotSwitchOnATie` pins that behaviour.

## Worked example

`P1` arrives at 0 needing 5, `P2` at 1 needing 3, `P3` at 2 needing 1.

```
tick  0   1   2   3   4   5   6   7   8
      P1  P2  P3  P2  P2  P1  P1  P1  P1
```

- **tick 0** - only `P1`, it starts.
- **tick 1** - `P2` arrives needing 3; `P1` has 4 left. `3 < 4`, so `P2` takes over.
- **tick 2** - `P3` arrives needing 1; `P2` has 2 left. `1 < 2`, so `P3` takes over.
- **tick 3** - `P3` is done. `P2` (2 left) beats `P1` (4 left), so `P2` resumes.
- **tick 5** - `P2` is done, `P1` runs out the rest.

| Process | Arrival | Burst | Completion | Turnaround | Waiting | Response |
|---------|---------|-------|-----------|-----------|---------|----------|
| P1 | 0 | 5 | 9 | 9 | 4 | 0 |
| P2 | 1 | 3 | 5 | 4 | 1 | 0 |
| P3 | 2 | 1 | 3 | 1 | 0 | 0 |

Average waiting time **1.67** - the best of the five on this workload. Average
response time is **0.00**: every process ran on the tick it arrived, because
each was the shortest thing present at that moment.

## Properties

| | |
|---|---|
| Average waiting time | best of the five algorithms here |
| Starvation | worse than SJF - a long job can be preempted repeatedly |
| Context switches | most of the five; potentially one per tick |
| Requires | knowing burst times in advance |
| Selection cost | O(n) per tick |

## The bug this algorithm is famous for

SRTF is where naive implementations break, and the project this one was modelled
on breaks exactly here.

The pattern: on each tick, put the running process back into the ready queue,
then pick the minimum remaining time. If the running process **finished** on the
previous tick and is re-queued anyway, its remaining time is `0` - which is
smaller than everything. It gets selected forever, its counter goes negative,
the completion count never reaches the process count, and the loop never ends.

Traced on the reference implementation with `P1(0,5)` and `P2(1,2)`:

```
t=3 run P2 rem=0
t=4 run P2 rem=-1
t=5 run P2 rem=-2      <- P1 never runs again
```

The server hangs and the request never returns.

This project makes that structurally impossible. Retirement happens in exactly
one place, inside the engine, and a retired process is never returned to the
ready list - so no policy can reintroduce it, however badly written:

```cpp
if (running.remainingTime <= 0) {
    result.metrics[running.index] = makeMetrics(...);
    cpuBusy = false;       // retired here, never re-queued
    ++completed;
}
```

`testSrtfTerminatesWhenAProcessFinishesEarly` runs that exact workload. See
[the engine](../architecture/the-engine.md) for the full invariant.

## Tests

In [`tests/AlgorithmTests.cpp`](../../tests/AlgorithmTests.cpp):

- `testSrtfPreemptsForAShorterJob` - the schedule above
- `testSrtfTerminatesWhenAProcessFinishesEarly` - the hang, as a regression test
- `testSrtfDoesNotSwitchOnATie` - no pointless context switch
- `testSrtfBeatsSjfOnWaitingTime` - preemption measurably pays off
