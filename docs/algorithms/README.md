# The algorithms

Five algorithms, chosen because each one teaches something the others do not.

| Algorithm | Preemptive | Picks | Teaches |
|-----------|-----------|-------|---------|
| [FCFS](fcfs.md) | no | whoever arrived first | the baseline, and the convoy effect |
| [SJF](sjf.md) | no | the shortest waiting job | optimal average waiting, and starvation |
| [SRTF](srtf.md) | yes | the shortest remaining time | what preemption buys you |
| [Round Robin](round-robin.md) | yes (on a timer) | whoever is next in the queue | fairness and responsiveness |
| [Priority](priority-and-aging.md) | no | the most important | starvation, and aging as the cure |

## The four measurements

For a process that arrives at `A`, needs `B` ticks of CPU, first runs at `F` and
finishes at `C`:

| Measure | Formula | Means |
|---------|---------|-------|
| **Turnaround** | `C - A` | total time in the system, start to finish |
| **Waiting** | `turnaround - B` | time spent ready but not running |
| **Response** | `F - A` | how long before it first got any CPU |
| **CPU utilisation** | `busy / total` | share of ticks the CPU was not idle |

Response and waiting differ in an important way. A process can respond
immediately and still wait a long time overall - that is exactly what Round
Robin does.

## The same workload, five ways

Three processes: `P1` arrives at 0 needing 5 ticks, `P2` at 1 needing 3, `P3` at
2 needing 1.

```
FCFS    P1 P1 P1 P1 P1 P2 P2 P2 P3           avg wait 3.33   avg response 3.33
SJF     P1 P1 P1 P1 P1 P3 P2 P2 P2           avg wait 2.67   avg response 2.67
SRTF    P1 P2 P3 P2 P2 P1 P1 P1 P1           avg wait 1.67   avg response 0.00
RR(2)   P1 P1 P2 P2 P3 P1 P1 P2 P1           avg wait 3.33   avg response 1.00
```

All four take exactly 9 ticks. **They have to** - there are 9 ticks of work and
no idle time, so the finish time cannot change. Every difference above is a
difference in *who* waits.

Read that table and the trade-off is visible:

- **SRTF wins on waiting time** by letting short work jump the queue.
- **Round Robin wins on responsiveness** relative to FCFS (1.00 against 3.33)
  while having the *same* average waiting time - everyone starts sooner and
  finishes later.
- **FCFS and SJF never interrupt anyone**, so a process that starts late stays
  late.

## Choosing between them

- **Batch work where throughput is all that matters** - FCFS. It is simple,
  predictable, and has no switching overhead.
- **Minimising average waiting time** - SJF or SRTF. SJF if you cannot
  interrupt, SRTF if you can. Both need to know how long jobs will take, which
  in real systems means estimating.
- **Anything interactive** - Round Robin. Nobody starves, and everything starts
  quickly. Tune the quantum: too small and you thrash on context switches, too
  large and it degenerates into FCFS. (This project has a test asserting that
  Round Robin with a quantum larger than any burst produces results identical to
  FCFS.)
- **Work of genuinely different importance** - Priority, and turn aging on
  unless you can prove starvation is impossible.

## Deliberately not included

**HRRN** is a different scoring formula plugged into the same non-preemptive
selection as SJF - it adds arithmetic, not a concept.

**Multilevel feedback queues** are the interesting omission. MLFQ is Round Robin
plus Priority plus demotion, composed. The architecture was built so it could be
added as a single new class without touching anything else - see
[adding an algorithm](../development/adding-an-algorithm.md).
