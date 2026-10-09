# Future improvements

Things deliberately left out, with enough detail to pick up later. Each entry
says what it is, why it was skipped, and what would actually have to change.

---

## Multilevel Feedback Queue (MLFQ)

**The most worthwhile thing left to add.** It is what real operating systems
use - Windows, macOS and Linux's older schedulers are all variations of it.

### The idea

Every other algorithm here needs something it cannot really know.
[SJF](../algorithms/sjf.md) and [SRTF](../algorithms/srtf.md) need to know how
long each job will take; [Priority](../algorithms/priority-and-aging.md) needs
somebody to assign priorities sensibly.

MLFQ needs neither. It **watches how processes behave and sorts them out
itself**:

```
Queue 0   quantum 2    <- everything starts here
Queue 1   quantum 4
Queue 2   quantum 8, FCFS   <- long-running work sinks to here
```

Two rules:

1. **Used your whole quantum?** You are CPU-hungry, so you are demoted to the
   next queue down - which gives you a longer turn, but a lower priority.
2. **Gave the CPU up early?** You are interactive, so you stay where you are.

The CPU always runs the highest non-empty queue.

The effect is that a text editor stays near the top and feels instant, while a
video export sinks to the bottom and quietly uses what is left - with nobody
assigning a single priority. SJF needs to know the future; MLFQ infers it from
the past.

Real implementations add a third rule - periodically push everything back to
the top queue - which is [aging](../algorithms/priority-and-aging.md) by another
name, and stops long jobs starving at the bottom forever.

### What would have to change

This is the one extension the current design cannot absorb without a change to
`ReadyProcess`, because **a policy holds no state**. Every other algorithm
computes its decision from what it is handed; MLFQ has to remember which queue
each process ended up in.

The smallest honest change:

1. Add `int queueLevel = 0;` to `ReadyProcess` in
   [`core/SchedulingPolicy.hpp`](../../core/SchedulingPolicy.hpp). The engine
   carries it and never interprets it.
2. Let a policy write it back - for example by having `choose()` return a small
   struct, or by adding an `onQuantumExpired(ReadyProcess&)` hook the engine
   calls when a time slice runs out.
3. `timeSlice()` becomes per-process rather than per-policy, since the quantum
   depends on which queue the process is in.

Step 2 is the design decision, and it is worth thinking about rather than
rushing: the reason policies are stateless is that it makes
[the engine's invariants](../architecture/the-engine.md) impossible for a policy
to break. Any hook that lets a policy write to engine state gives some of that
back, so it should be as narrow as possible.

Then it is one new class, a registry entry, and two lines in the web interface -
see [adding an algorithm](adding-an-algorithm.md).

### Why it was skipped

Scope. Five algorithms already cover preemptive and non-preemptive, both
selection strategies, starvation and its cure. MLFQ is the sixth that needs an
architectural change, and doing it badly - bolting mutable state onto the policy
interface - would cost more than it adds.

---

## WebAssembly build

Compile the C++ to WebAssembly with Emscripten so the scheduler runs **inside
the browser**, with no server at all.

**Why it is appealing:** the whole project becomes a static site. It hosts free
on GitHub Pages forever, there is no Node process, no spawning, no `wsl.exe`
bridge, and nothing to go wrong at runtime. It would also have sidestepped the
entire Smart App Control problem documented in
[problems we hit](../troubleshooting/problems-we-hit.md).

**What it costs:** Emscripten is a large toolchain, the build gets a second
compiler path to maintain, and the CLI still needs a native build. The JSON
boundary would stay exactly as it is, so the browser code barely changes.

---

## Input/output bursts

Right now a process needs CPU and nothing else. Real processes alternate between
computing and waiting for disk or network, which is precisely what MLFQ's
"gave the CPU up early" rule detects.

Adding it would mean a process described as a sequence - `CPU 3, IO 5, CPU 2` -
and a notion of being blocked rather than ready. That is a real change to the
engine: a third state alongside ready and running, and arrivals back into the
ready queue when I/O finishes.

It would make the simulator meaningfully more realistic, and it is a
prerequisite for MLFQ being interesting rather than just correct.

---

## Smaller ideas

- **Per-process colour choice** in the web interface, for screenshots.
- **Export the Gantt chart** as SVG or PNG from the browser.
- **A workload library** - classic textbook examples, each with the point it
  illustrates, loadable from a dropdown.
- **Starvation detection**: flag any process whose waiting time exceeds some
  multiple of its burst, so the problem is called out rather than left to be
  noticed.
- **Variance as well as averages.** Average waiting time hides how unfair a
  schedule is; two algorithms can share an average while one treats everyone
  equally and the other is dreadful to a few processes.

---

## Things deliberately not planned

- **More algorithms for their own sake.** HRRN is a different scoring formula
  plugged into the same selection as SJF - it adds arithmetic, not a concept.
- **A database.** A simulator has nothing to persist. Every run is derived
  entirely from the workload you typed, so storage would add operational weight
  and buy nothing.
- **Accounts or sharing.** A URL carrying the workload would do the same job
  with none of the machinery.
