# Priority scheduling, with aging

Run the most important thing first. Then deal with the consequence.

**Preemptive:** no.
**Source:** [`core/policies/PriorityPolicy.cpp`](../../core/policies/PriorityPolicy.cpp)

## The convention

**A lower priority number means more important.** Priority 1 outranks priority
5. Negative numbers are allowed and mean *very* important.

This is written down in [`core/Process.hpp`](../../core/Process.hpp) because it
is the sort of thing that quietly goes wrong. The project this one was modelled
on contains the comment *"Assuming higher number = higher priority? Or lower?"*
in its source, and then implements the opposite of what its own README claims.

## How it works

1. When the CPU frees up, pick the waiting process with the best priority.
2. Let it run to completion.
3. Repeat.

## Why non-preemptive

Preemptive priority is also a legitimate design, and it was considered. It was
rejected because combining it with aging raises a question with no clean answer:
**does a process keep the priority boost it earned while waiting, once it starts
running?**

- If it does, the policy needs to remember when it was scheduled - state a
  policy is deliberately not allowed to hold in this design.
- If it does not, aging becomes useless: the moment an aged process finally
  gets the CPU, the next important arrival preempts it straight back out.

Non-preemptive sidesteps this entirely, and aging's effect stays clean and
demonstrable. The trade-off is recorded here rather than being invisible.

## Aging

Without aging, an unimportant process can wait forever - every time the CPU
frees up, something more urgent is waiting.

Aging fixes it by making waiting *earn* importance. Every `agingRate` ticks a
process spends waiting, its effective priority improves by one step.

```cpp
int PriorityPolicy::effectivePriority(const ReadyProcess& process, int currentTime) const {
    if (agingRate_ == 0) return process.process->priority;  // aging disabled
    const int ticksWaited = currentTime - process.readySince;
    return process.process->priority - (ticksWaited / agingRate_);
}
```

Note what this does **not** do: it never writes to the process. The boost is
*derived* from how long the process has been queued, recalculated whenever it is
needed. There is no `currentPriority` field to corrupt, no reset to forget, and
the policy stays `const` like every other.

The reference implementation mutates a `currentPriority` field on every process
on every tick. Deriving the value instead removes that entire class of bug.

One consequence worth knowing: because the boost is measured from
`readySince` - the moment the process last joined the queue - a process that
runs and is later re-queued **starts aging afresh**. That matches the usual
description of aging, where the boost is spent when the process gets scheduled.

## Worked example: starvation, then the cure

The workload in [`examples/starvation.txt`](../../examples/starvation.txt): one
unimportant process against a stream of urgent ones.

```
# ID   ARRIVAL  BURST  PRIORITY
LOW    0        3      5
H1     0        2      1
H2     1        2      1
H3     3        2      1
H4     5        2      1
```

The urgent processes are timed so that whenever the CPU frees up, another one is
already waiting.

### Without aging

```bash
./build/bin/scheduler.exe -a Priority -i examples/starvation.txt
```

```
LOW  completed=11  turnaround=11  waiting=8  response=8
```

`LOW` does not run at all until tick 8. Every time the CPU came free - ticks 2,
4 and 6 - a priority-1 process was waiting and won.

### With aging

```bash
./build/bin/scheduler.exe -a Priority -g 1 -i examples/starvation.txt
```

```
LOW  completed=9  turnaround=9  waiting=6  response=6
```

At tick 6, `LOW` has waited 6 ticks, so its effective priority is `5 - 6 = -1`.
`H4` arrived at tick 5 and sits at `1 - 1 = 0`. `-1` beats `0`, and `LOW`
finally runs.

### What it cost

| | without aging | with aging |
|---|---|---|
| LOW first runs at | 8 | **6** |
| LOW waiting time | 8 | **6** |
| H4 waiting time | 1 | **4** |
| Total runtime | 11 | **11** |

`LOW` waits two ticks less; `H4` waits three more; the run takes exactly as long
either way. **Aging redistributes waiting - it does not remove it.**
`testAgingRescuesStarvedWork` asserts the total runtime is unchanged, so that
cannot be misread as a free improvement.

## Choosing an aging rate

- **0** - off. Only safe if you can prove low-priority work will get gaps.
- **small (1-2)** - aggressive; priority becomes a hint rather than a rule.
- **large (10+)** - priority is mostly respected, with starvation bounded rather
  than eliminated quickly.

The right value depends on how long you are willing to let unimportant work
wait. Aging never breaks correctness, only the ordering you asked for.

## Properties

| | |
|---|---|
| Average waiting time | depends entirely on the priorities given |
| Starvation | guaranteed possible without aging; bounded with it |
| Context switches | one per process |
| Requires | someone to assign meaningful priorities |
| Selection cost | O(n) per selection |

## Tests

In [`tests/PriorityTests.cpp`](../../tests/PriorityTests.cpp):

- `testPriorityRunsMostImportantFirst`
- `testPriorityDoesNotInterruptARunningProcess`
- `testPriorityStarvesLowPriorityWork` - starvation, as numbers
- `testAgingRescuesStarvedWork` - the cure, plus the cost
- `testAgingIsOffByDefault`

and in [`tests/RobustnessTests.cpp`](../../tests/RobustnessTests.cpp),
`testNegativePrioritiesWork`.
