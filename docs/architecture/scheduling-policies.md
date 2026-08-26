# Scheduling policies

**Source:** [`core/SchedulingPolicy.hpp`](../../core/SchedulingPolicy.hpp)

An algorithm's job is to answer one question: **who runs next?** Everything
else - the clock, the queue, timing, metrics - belongs to
[the engine](the-engine.md).

## The interface

```cpp
class SchedulingPolicy {
public:
    virtual ~SchedulingPolicy() = default;

    virtual std::string name() const = 0;

    // Pick which ready process runs next. `ready` is never empty.
    virtual std::size_t choose(const std::vector<ReadyProcess>& ready,
                               int currentTime) const = 0;

    // How many ticks a process may hold the CPU before giving it up.
    // 0 means no limit. Only Round Robin overrides this.
    virtual int timeSlice() const { return 0; }

    // Should the running process be interrupted right now?
    // Non-preemptive algorithms use the default.
    virtual bool shouldPreempt(const ReadyProcess& running,
                               const std::vector<ReadyProcess>& ready,
                               int currentTime) const { return false; }
};
```

Three hooks, two with defaults. All five algorithms are expressible in them, and
each ends up 20-40 lines.

| Algorithm | `choose` | `timeSlice` | `shouldPreempt` |
|-----------|----------|-------------|-----------------|
| FCFS | queue order | - | - |
| SJF | smallest burst | - | - |
| SRTF | smallest remaining | - | anything shorter is waiting |
| Round Robin | queue order | quantum | - |
| Priority | best effective priority | - | - |

## Everything is const

Every method is `const`, and `ReadyProcess` is passed as `const&`. A policy has
**no way to write to simulation state**. It cannot decrement a counter, mark
something finished, or reorder a queue.

That is not a style preference. It is what makes the engine's invariants hold
regardless of how badly a policy is written. The worst a broken policy can do is
return a poor index.

Compare with the reference implementation, where each algorithm is a branch
inside the main loop, mutating the ready queue, the running process and the
clock directly - which is how two of its eight algorithms end up hanging.

## What a policy sees

```cpp
struct ReadyProcess {
    const Process* process;    // the original, unmodified input
    int remainingTime;         // CPU time still needed
    int readySince;            // tick it last joined the ready queue
    long long queueOrder;      // strictly increasing; the tie-break
    int firstRunTime;          // tick it first got the CPU, -1 if never
    std::size_t index;         // position in the original input list
};
```

`process` points at the untouched input, so `process->burstTime` is always the
*total* burst while `remainingTime` is what is left. SJF uses the former, SRTF
the latter, and the difference between those two lines is the difference between
the algorithms.

## The tie-breaking rule

> When two processes look equally good, prefer the smaller `queueOrder` - that
> is, whoever joined the ready queue first.

Written once in the interface, implemented once in
[`core/policies/Select.hpp`](../../core/policies/Select.hpp), used by every
policy:

```cpp
template <typename IsBetter>
std::size_t selectBest(const std::vector<ReadyProcess>& ready, IsBetter isBetter) {
    std::size_t best = 0;
    for (std::size_t i = 1; i < ready.size(); ++i) {
        const ReadyProcess& candidate = ready[i];
        const ReadyProcess& incumbent = ready[best];

        if (isBetter(candidate, incumbent)) {
            best = i;
        } else if (!isBetter(incumbent, candidate) && joinedQueueFirst(candidate, incumbent)) {
            best = i;
        }
    }
    return best;
}
```

`isBetter` expresses only what makes an algorithm *different*; anything it
considers equal falls through to the shared rule. Which is why FCFS is a policy
with no opinion at all:

```cpp
return selectBest(ready, [](const ReadyProcess&, const ReadyProcess&) { return false; });
```

Without a single shared rule, each algorithm would tie-break slightly
differently - or not at all, leaving results dependent on vector ordering and
impossible to reproduce.

## The registry

**Source:** [`core/PolicyRegistry.cpp`](../../core/PolicyRegistry.cpp)

```cpp
std::unique_ptr<SchedulingPolicy> makePolicy(const std::string& name,
                                             const PolicyOptions& options = {});
std::vector<std::string> availablePolicies();
```

Name to policy, case-insensitive. `PolicyOptions` carries `quantum` and
`agingRate`, and each policy takes what it needs and ignores the rest - so
callers never need to know which algorithm wants which setting.

An unknown name returns `nullptr`. It does **not** fall back to a default: the
reference implementation silently returns FCFS for an unrecognised algorithm, so
a typo gives you confidently wrong results with no warning.

`availablePolicies()` is what the CLI's `--list` and `--compare all` iterate, so
adding an algorithm requires no changes to either.

## Next

[Adding an algorithm](../development/adding-an-algorithm.md), with a worked
example.
