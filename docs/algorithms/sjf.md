# Shortest Job First (SJF)

When the CPU frees up, run whichever waiting process needs the least CPU time.

**Preemptive:** no.
**Source:** [`core/policies/SjfPolicy.cpp`](../../core/policies/SjfPolicy.cpp)

## How it works

1. When the CPU is free, look at everything currently waiting.
2. Pick the one with the smallest total burst time.
3. Let it run to completion.

The catch is in step 1: **only what is currently waiting**. A shorter job that
arrives a tick later has missed its chance, because this version never
interrupts. That is what [SRTF](srtf.md) changes.

## In this codebase

```cpp
std::size_t SjfPolicy::choose(const std::vector<ReadyProcess>& ready, int) const {
    // Compare total burst time, not remaining time: this policy never
    // interrupts anyone, so a process only ever gets picked untouched.
    return selectBest(ready, [](const ReadyProcess& a, const ReadyProcess& b) {
        return a.process->burstTime < b.process->burstTime;
    });
}
```

Ties fall through to the shared tie-break, so two equal-length jobs run in the
order they queued. Without that, the result would depend on vector ordering and
would not be reproducible.

## Worked example

`P1` arrives at 0 needing 5, `P2` at 1 needing 3, `P3` at 2 needing 1.

```
tick  0   1   2   3   4   5   6   7   8
      P1  P1  P1  P1  P1  P3  P2  P2  P2
```

At tick 0 only `P1` exists, so it starts - and because SJF is non-preemptive, it
finishes. At tick 5 both `P2` (3 ticks) and `P3` (1 tick) are waiting, so `P3`
goes first.

| Process | Arrival | Burst | Completion | Turnaround | Waiting |
|---------|---------|-------|-----------|-----------|---------|
| P1 | 0 | 5 | 5 | 5 | 0 |
| P2 | 1 | 3 | 9 | 8 | 5 |
| P3 | 2 | 1 | 6 | 4 | 3 |

Average waiting time **2.67**, against FCFS's 3.33 on the same workload.

## Why it is optimal

For any set of processes all available at the same time, SJF gives the **lowest
possible average waiting time** of any non-preemptive schedule.

The intuition: every process queued behind the one running waits for its whole
burst. Putting a long job first makes *everyone* behind it pay that cost; putting
a short job first charges everyone a small cost. Sorting shortest-first
minimises the total.

There is a test for this claim rather than a comment asserting it. On a workload
of one long job plus three short ones:

```
FCFS  average waiting 3.75
SJF   average waiting 0.75
```

## Starvation

The price. A long job can be indefinitely postponed if short jobs keep arriving,
because every time the CPU frees up there is something shorter waiting.

`testSjfBeatsFcfsOnWaitingTime` shows this from the short jobs' side; the long
job in that workload waits while three shorter ones jump ahead of it. In a real
system with a steady stream of short work, it might wait forever.

The fix is [aging](priority-and-aging.md), which this project implements for
Priority scheduling.

## The other practical problem

**You have to know how long a job will take.** In a real operating system you do
not. Real schedulers estimate it from recent history - typically an exponential
moving average of previous CPU bursts - which works because process behaviour is
usually consistent. This simulator is given the burst times up front, which is
the standard simplification for studying the algorithm.

## Properties

| | |
|---|---|
| Average waiting time | provably optimal among non-preemptive schedules |
| Starvation | possible, for long jobs |
| Context switches | one per process, same as FCFS |
| Requires | knowing burst times in advance |
| Selection cost | O(n) as written; O(log n) with a heap |

## Tests

In [`tests/AlgorithmTests.cpp`](../../tests/AlgorithmTests.cpp):

- `testSjfPrefersShorterJobs` - the schedule above
- `testSjfDoesNotInterruptARunningProcess` - a shorter arrival must wait
- `testSjfBreaksTiesByArrival` - equal bursts run in queue order
- `testSjfBeatsFcfsOnWaitingTime` - the optimality claim, as numbers
