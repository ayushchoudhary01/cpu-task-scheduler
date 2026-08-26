# The engine

**Source:** [`core/Engine.cpp`](../../core/Engine.cpp)

One function, `runSimulation`, containing one loop. It owns the clock, the ready
queue, and all the bookkeeping. Algorithms own none of that - they answer
questions.

```cpp
SimulationResult runSimulation(const std::vector<Process>& processes,
                               const SchedulingPolicy& policy);
```

The input is `const`. The same workload can be run through five policies and
compared, with no resetting and no copying by the caller.

## The tick loop

The simulation advances one tick at a time. Each tick does four things in a
fixed order, and the order is part of the design.

```
while (completed < processes.size()) {
    1. admit everything that has arrived by now
    2. decide whether the running process must give up the CPU
    3. hand the CPU to whoever the policy picks
    4. run for exactly one tick
}
```

### 1. Admission

```cpp
while (nextArrival < order.size() &&
       processes[order[nextArrival]].arrivalTime <= currentTime) {
```

`<=`, not `==`. Using `==` works only if the loop visits every tick, and breaks
silently the moment anything changes. The reference implementation uses `==`.

Processes are admitted in arrival order regardless of the order they appear in
the input file - `arrivalOrder()` sorts indices with `stable_sort`, so equal
arrival times keep their listed order.

### 2. Preemption

```cpp
const bool sliceUsedUp = policy.timeSlice() > 0 && ticksOnCpu >= policy.timeSlice();

if (!ready.empty() &&
    (sliceUsedUp || policy.shouldPreempt(running, ready, currentTime))) {
    running.readySince = currentTime;
    running.queueOrder = nextQueueOrder++;
    ready.push_back(running);
    cpuBusy = false;
} else if (sliceUsedUp) {
    // Nothing else wants the CPU, so it simply gets another slice.
    ticksOnCpu = 0;
}
```

Two ways to lose the CPU: a time slice running out (Round Robin) or the policy
asking for it (SRTF). Neither happens if nothing is waiting - switching to
nobody would just waste a tick.

**This step happens after admission**, which is what makes a newly arrived
process queue *ahead* of one whose quantum just expired. That is the
conventional Round Robin behaviour, and it falls out of ordering rather than
being special-cased.

### 3. Selection

```cpp
const std::size_t chosen = policy.choose(ready, currentTime);
running = ready[chosen];
ready.erase(ready.begin() + chosen);
if (running.firstRunTime < 0) running.firstRunTime = currentTime;
```

The policy returns an index. The engine does the removal, and records first-run
time - so response time cannot be got wrong by an algorithm.

### 4. Execution

```cpp
if (cpuBusy) {
    result.timeline.runProcess(currentTime, running.process->id);
    running.remainingTime -= 1;
    ticksOnCpu += 1;

    if (running.remainingTime <= 0) {
        result.metrics[running.index] =
            makeMetrics(*running.process, running.firstRunTime, currentTime + 1);
        cpuBusy = false;
        ++completed;
    }
} else {
    result.timeline.runIdle(currentTime);
}
```

Exactly one tick. Never two, never a whole burst.

## The invariant that matters

**A finished process is retired in one place, and can never re-enter the ready
queue.**

Look at the completion branch: `cpuBusy = false` and nothing is pushed back into
`ready`. There is no code path anywhere by which a completed process returns to
being schedulable, whatever a policy does.

This is the difference between fixing a bug and removing the possibility of one.
The project this was modelled on re-queues the running process on every
preemptive tick without checking whether it finished - so a completed process,
with zero remaining time, looks like the best possible choice forever. The loop
never terminates and the server hangs. See
[SRTF](../algorithms/srtf.md#the-bug-this-algorithm-is-famous-for).

Here, a policy *cannot* cause that. The worst a broken policy can do is pick a
bad process, not resurrect a dead one - and there is a test using a deliberately
absurd policy (`AlwaysPreempt`, which interrupts on every single tick) that
confirms the engine still terminates with no wasted ticks.

## Queue ordering

Each process gets a strictly increasing `queueOrder` stamp as it joins the ready
queue. Policies tie-break on it.

The obvious alternative - "which tick did you join" - is not enough, because
several processes can join on the same tick, and in Round Robin that happens
constantly. `queueOrder` gives a **total order**, so a simulation's result never
depends on vector ordering.

## What the engine guarantees

Whatever policy is used:

- busy time equals the total burst time - the CPU does exactly the work asked,
  no more
- the timeline starts at 0, with no gaps or overlaps between slices
- nothing finishes before it arrives
- `turnaround == waiting + burst`
- `response <= waiting`
- metrics come back in input order, not completion order

These are not aspirations. `tests/RobustnessTests.cpp` runs 20 generated
workloads through all 5 algorithms - 100 simulations - and checks every one of
them after each. See [testing](../development/testing.md).

## Termination

The loop runs while `completed < processes.size()`. Two things guarantee it ends:

1. Every tick with a running process reduces its remaining time by one.
2. A process reaching zero is retired permanently and increments `completed`.

Zero-burst processes cannot reach the engine - the parser rejects them - but the
completion check is `<= 0` rather than `== 0` anyway, so even a burst of 0 would
terminate rather than counting down forever.
