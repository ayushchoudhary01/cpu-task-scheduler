# Round Robin

Everyone takes turns. Each process gets at most `quantum` ticks, then goes to
the back of the queue.

**Preemptive:** yes, but on a timer rather than on a comparison.
**Source:** [`core/policies/RoundRobinPolicy.cpp`](../../core/policies/RoundRobinPolicy.cpp)

## How it works

1. Take the process at the front of the queue.
2. Run it for up to `quantum` ticks.
3. If it is not finished, put it at the back and take the next one.

No process is ever considered more deserving than another. That is the entire
idea, and it is why nothing starves.

## In this codebase

Round Robin's `choose` is **identical to FCFS** - front of the queue:

```cpp
std::size_t RoundRobinPolicy::choose(const std::vector<ReadyProcess>& ready, int) const {
    // Like FCFS, Round Robin just takes the front of the queue. What makes it
    // different is timeSlice(), which sends the running process to the back.
    return selectBest(ready, [](const ReadyProcess&, const ReadyProcess&) { return false; });
}

int timeSlice() const override { return quantum_; }
```

The whole difference is one number. The engine counts how long the current
process has held the CPU and re-queues it when the slice runs out.

## Queue order matters

When a process arrives on the *same tick* that another's quantum expires, which
goes first?

The convention is that the **new arrival queues ahead** of the one being
preempted, and the engine gets this right by ordering its steps: arrivals are
admitted before preemption is handled, and each process is stamped with an
increasing `queueOrder` as it joins.

This is subtler than it looks. An earlier version of this project tie-broke on
"which tick did you join the queue", which cannot separate two processes that
joined on the *same* tick - and Round Robin hits that case constantly. It was
found by working out the expected output by hand and noticing the code
disagreed. `testRoundRobinQueuesArrivalsAheadOfPreemptedProcess` pins it.

## Worked example

`P1` arrives at 0 needing 5, `P2` at 1 needing 3, `P3` at 2 needing 1, quantum 2.

```
tick  0   1   2   3   4   5   6   7   8
      P1  P1  P2  P2  P3  P1  P1  P2  P1
```

| Process | Arrival | Burst | Completion | Turnaround | Waiting | Response |
|---------|---------|-------|-----------|-----------|---------|----------|
| P1 | 0 | 5 | 9 | 9 | 4 | **0** |
| P2 | 1 | 3 | 8 | 7 | 4 | **1** |
| P3 | 2 | 1 | 5 | 3 | 2 | **2** |

Average waiting **3.33**, average response **1.00**.

Now compare against FCFS on the same workload: average waiting is *also* 3.33,
but FCFS's average response is 3.33 against Round Robin's 1.00.

That is the trade in one line. **Round Robin does not reduce waiting - it
redistributes it.** Everybody starts sooner; everybody finishes later. On an
interactive system that is exactly the right trade, because a user notices the
delay before the first response far more than the total.

## The quantum

The single tuning knob, and it interpolates between two other algorithms:

| Quantum | Behaviour |
|---------|-----------|
| 1 | maximum fairness, maximum switching |
| moderate | the useful middle |
| larger than any burst | **identical to FCFS** |

That last row is not an approximation. With a quantum nobody ever exhausts, no
process is ever re-queued, so Round Robin *is* FCFS.
`testRoundRobinWithLargeQuantumBehavesLikeFcfs` asserts the two produce the same
slices and the same averages.

In a real operating system there is a cost per switch - saving registers,
flushing pipelines, cache effects - so a tiny quantum wastes real time on
overhead. This simulator treats switching as free, which is the standard
simplification; adding a configurable switch cost would be a natural extension.

## Properties

| | |
|---|---|
| Average waiting time | usually the worst of the five |
| Average response time | usually the best |
| Starvation | impossible - the queue rotates |
| Context switches | many; controlled by the quantum |
| Requires | nothing; no knowledge of burst times |
| Selection cost | O(1) with a real queue |

That "requires nothing" row is underrated. SJF and SRTF need to know how long
jobs will take. Round Robin does not, which is why real interactive schedulers
are built on it.

## Tests

In [`tests/AlgorithmTests.cpp`](../../tests/AlgorithmTests.cpp):

- `testRoundRobinTakesTurns` - the schedule above
- `testRoundRobinQueuesArrivalsAheadOfPreemptedProcess` - the same-tick ordering rule
- `testRoundRobinWithLargeQuantumBehavesLikeFcfs` - degenerates to FCFS
- `testRoundRobinRespondsFasterThanFcfs` - 1.00 against 3.33
