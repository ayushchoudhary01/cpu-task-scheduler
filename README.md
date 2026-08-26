<div align="center">

# CPU Task Scheduler

### A CPU scheduling simulator with a real C++ engine

*Five classic algorithms. A tick-accurate engine. A Gantt chart in your terminal — and in your browser.*

<br>

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)
[![React](https://img.shields.io/badge/React-18-61DAFB?style=for-the-badge&logo=react&logoColor=black)](https://react.dev/)
[![TypeScript](https://img.shields.io/badge/TypeScript-3178C6?style=for-the-badge&logo=typescript&logoColor=white)](https://www.typescriptlang.org/)
[![Tailwind](https://img.shields.io/badge/Tailwind-4-06B6D4?style=for-the-badge&logo=tailwindcss&logoColor=white)](https://tailwindcss.com/)

[![Algorithms](https://img.shields.io/badge/algorithms-5-8b5cf6?style=flat-square)](docs/algorithms/README.md)
[![Dependencies](https://img.shields.io/badge/C%2B%2B%20dependencies-0-orange?style=flat-square)](CMakeLists.txt)
[![License](https://img.shields.io/badge/license-MIT-blue?style=flat-square)](LICENSE)

<br>

[**Quick start**](#quick-start) &nbsp;•&nbsp;
[**Algorithms**](#algorithms) &nbsp;•&nbsp;
[**How it works**](#how-it-works) &nbsp;•&nbsp;
[**Docs**](docs/README.md)

</div>

---

##  See it run

One workload, four algorithms, side by side — same work, completely different experience:

```
Comparison of 5 algorithms on 4 processes

  Algorithm   Waiting  Turnaround  Response    CPU %   Total
  FCFS           3.75        7.00      3.75   100.00      13
  SJF            3.25        6.50      3.25   100.00      13
  SRTF           2.50        5.75      1.25   100.00      13   ← best waiting
  RR(q=2)        4.75        8.00      1.50   100.00      13
  Priority       3.75        7.00      3.75   100.00      13

  Lowest average waiting time:  SRTF (2.50)
  Lowest average response time: SRTF (1.25)
```

> [!NOTE]
> The **Total** column is identical on every row — and that is the whole point.
> The same work takes the same time however you schedule it.
> **Scheduling changes _who waits_, not how much work there is.**

And a Gantt chart, drawn in the terminal, with block widths proportional to CPU time:

```
Algorithm: SJF

Gantt chart
  +-------------------------+-----+---------------+--------------------+
  |           P1            | P3  |      P2       |         P4         |
  +-------------------------+-----+---------------+--------------------+
  0                         5     6               9                    13

Averages
  Waiting time              3.25
  Turnaround time           6.50
  CPU utilization         100.00 %
  Throughput                0.31 processes/tick
```

---

<a id="quick-start"></a>

##  Quick start

<table>
<tr><td width="60"><h3 align="center">1️⃣</h3></td>
<td>

**Get a compiler** — Windows doesn't ship one *(skip if `g++ --version` works)*

```bash
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
```

</td></tr>
<tr><td><h3 align="center">2️⃣</h3></td>
<td>

**Build it**

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

</td></tr>
<tr><td><h3 align="center">3️⃣</h3></td>
<td>

**Run it**

```bash
./build/bin/scheduler.exe --compare all -i examples/sample.txt
```

</td></tr>
<tr><td><h3 align="center">4️⃣</h3></td>
<td>

**Or open the web interface** → [localhost:5173](http://localhost:5173)

```bash
cd frontend && npm install && npm run dev
```

</td></tr>
</table>

> 💡 New to C++ builds? [**Installing a compiler**](docs/getting-started/installing-a-compiler.md) explains why an editor isn't enough.

---

<a id="algorithms"></a>

##  The algorithms

<div align="center">

| | Algorithm | Preemptive | Picks |  Best at |
|:--:|:--|:--:|:--|:--|
| 🔵 | [**FCFS**](docs/algorithms/fcfs.md) | ❌ | whoever arrived first | simplicity, zero overhead |
| 🟢 | [**SJF**](docs/algorithms/sjf.md) | ❌ | the shortest waiting job | *provably* optimal waiting time |
| 🟣 | [**SRTF**](docs/algorithms/srtf.md) | ✅ | the shortest remaining time | lowest waiting time overall |
| 🟠 | [**Round Robin**](docs/algorithms/round-robin.md) | ✅ | next in the queue | responsiveness, no starvation |
| 🔴 | [**Priority + aging**](docs/algorithms/priority-and-aging.md) | ❌ | the most important | honouring importance, safely |

</div>

<details>
<summary><b>🍽️ See starvation happen — and then get fixed</b></summary>

<br>

An unimportant process against a stream of urgent ones:

```bash
./build/bin/scheduler.exe -a Priority -i examples/starvation.txt
```

```
LOW  completed=11  waiting=8  response=8      😵 never runs until tick 8
```

Now turn **aging** on — waiting earns importance:

```bash
./build/bin/scheduler.exe -a Priority -g 1 -i examples/starvation.txt
```

```
LOW  completed=9   waiting=6  response=6      😌 rescued
```

| | without aging | with aging |
|---|:--:|:--:|
| LOW waiting time | 8 | **6** ⬇ |
| H4 waiting time | 1 | **4** ⬆ |
| Total runtime | 11 | **11** ➡ |

Aging **redistributes** waiting — it never removes it. There's a test asserting
the total runtime is unchanged, so the improvement can't be misread as free.

</details>

---

<a id="how-it-works"></a>

##  How it works

```mermaid
flowchart LR
    UI[" React + TypeScript<br/>Simulator · Compare"]
    API[" Express<br/>validate · spawn"]
    CPP[" C++ engine<br/>tick loop · policies"]

    UI -- "JSON request" --> API
    API -- "stdin: workload" --> CPP
    CPP -- "stdout: JSON" --> API
    API -- "results" --> UI

    style UI fill:#3987e5,stroke:#1e40af,stroke-width:2px,color:#fff
    style API fill:#199e70,stroke:#065f46,stroke-width:2px,color:#fff
    style CPP fill:#d95926,stroke:#9a3412,stroke-width:2px,color:#fff
```

The browser doesn't reimplement anything. It sends a workload to the **same
binary** the terminal uses, so the two can never disagree — and every number in
the UI was checked against the CLI to prove it.

### The design decision that matters

Most simulators put every algorithm as a branch inside one giant loop. This one
splits it: the **engine** owns the clock, the queue and all bookkeeping; a
**policy** answers one question — *who runs next?*

```cpp
class SchedulingPolicy {
    virtual std::size_t choose(const std::vector<ReadyProcess>& ready,
                               int currentTime) const = 0;   // who runs next?
    virtual int timeSlice() const { return 0; }               // Round Robin
    virtual bool shouldPreempt(...) const { return false; }   // SRTF
};
```

Every method is `const`. **A policy physically cannot corrupt simulation state.**

Which makes each algorithm tiny — FCFS is a policy with no opinion at all, so
arrival order falls straight out of the shared tie-break:

```cpp
return selectBest(ready, [](const ReadyProcess&, const ReadyProcess&) { return false; });
```

<details>
<summary><b>🐛 And it makes the classic SRTF hang impossible</b></summary>

<br>

Naive SRTF implementations re-queue the running process each tick without
checking whether it finished. A completed process has `remainingTime == 0` —
which looks like the *best possible choice*, forever:

```
t=3 run P2 rem=0
t=4 run P2 rem=-1
t=5 run P2 rem=-2      ← P1 never runs again, loop never ends
```

Here, retirement happens in **exactly one place**, inside the engine, and a
retired process is never returned to the ready list:

```cpp
if (running.remainingTime <= 0) {
    result.metrics[running.index] = makeMetrics(...);
    cpuBusy = false;      // retired here — cannot come back
    ++completed;
}
```

No policy can cause the hang, however badly written. There's a regression test
for the exact workload, plus a deliberately absurd policy that preempts on
*every single tick* to hammer the path.

</details>

---

##  Tested properly

<div align="center">

**`258/258 checks passed`**

</div>

| | Approach | Why |
|:--:|:--|:--|
|  | **Hand-computed expectations** | Every number worked out on paper first — never captured from a run. It caught a real design flaw *and* an arithmetic error of mine. |
|  | **Property tests** | Textbook claims asserted, not commented: SJF beats FCFS; RR responds faster; RR with a huge quantum *is* FCFS. |
|  | **Invariants over random workloads** | 20 generated workloads × 5 algorithms = **100 simulations**, each checked against 6 rules that must always hold. |
|  | **Stub policies** | An `AlwaysPreempt` policy nothing sane would use, purely to stress the engine. |

```bash
./build/bin/scheduler_tests.exe        # or: ctest --test-dir build
```

---

## Features

<table>
<tr>
<td width="33%" valign="top">

### Engine
- Tick-accurate simulation
- 5 pluggable algorithms
- Deterministic tie-breaking
- Zero dependencies

</td>
<td width="33%" valign="top">

### CLI
- ASCII Gantt chart
- Text **or** JSON output
- Multi-algorithm compare
- Seeded workload generator

</td>
<td width="33%" valign="top">

### Web
- Animated Gantt chart
- Editable workload
- Side-by-side compare
- Colour-blind-safe palette

</td>
</tr>
</table>

---

## Project layout

```
cpu-task-scheduler/
├── 📂 core/        the scheduling logic — no I/O, no dependencies
│   └── policies/   one small class per algorithm
├── 📂 cli/         parsers + Gantt/table/JSON renderers
├── 📂 tests/       258 checks; a ~30-line test helper, no framework
├── 📂 frontend/    React + TypeScript UI, Express bridge to the binary
├── 📂 examples/    sample and starvation workloads
└── 📂 docs/        20 pages — algorithms, architecture, troubleshooting
```

---

## Documentation

<div align="center">

| | | |
|:--|:--|:--|
| 🚀 [**Getting started**](docs/getting-started/building.md) | 🧠 [**Algorithms**](docs/algorithms/README.md) | 🏗️ [**Architecture**](docs/architecture/overview.md) |
| Compiler setup, building, every CLI flag | How each one works, with worked examples | The engine, policies, JSON contract |
| 🧪 [**Testing**](docs/development/testing.md) | ➕ [**Add an algorithm**](docs/development/adding-an-algorithm.md) | 🔧 [**Troubleshooting**](docs/troubleshooting/build-and-toolchain.md) |
| What's checked, and how | Worked example, end to end | Build, toolchain and web issues |

</div>

> [!TIP]
> [**Problems we hit**](docs/troubleshooting/problems-we-hit.md) — every real bug
> and blocker during development, with the fix and the lesson. Includes the time
> a clean typecheck shipped a completely blank page.

---

## Command reference

```bash
scheduler [options]
```

| Flag | Does |
|:--|:--|
| `-a`, `--algorithm NAME` | which algorithm to run *(default `FCFS`)* |
| `-c`, `--compare LIST` | run several on one workload — comma separated, or `all` |
| `-f`, `--format FORMAT` | `text` or `json` |
| `-q`, `--quantum N` | Round Robin time slice |
| `-g`, `--aging N` | one priority step per N ticks waited |
| `-i`, `--input FILE` | workload file *(default: stdin)* |
| `--generate N` `--seed S` | make up a reproducible workload |
| `-l`, `--list` &nbsp; `-h`, `--help` | list algorithms · show usage |

<details>
<summary><b>📄 Workload file format</b></summary>

<br>

```
# ID  ARRIVAL  BURST  PRIORITY      ← '#' starts a comment
P1    0        5      3
P2    1        3      1
P3    2        1      2
```

Priority is optional and defaults to `0`. **Lower means more important.**

Every problem is reported at once, not one per run:

```
Problems in the workload:
  line 1: burst time 'five' is not a whole number
  line 2: arrival time cannot be negative
  line 4: duplicate process id 'P1'
```

</details>

---

<div align="center">

### Room to grow

**Multilevel feedback queues** &nbsp;·&nbsp; **configurable context-switch cost** &nbsp;·&nbsp; **WebAssembly build**

*The architecture was built so MLFQ is one new class touching nothing else.*

<br>

---

<sub>Built with C++20, React and a lot of hand-computed Gantt charts.</sub>

<sub>[MIT licensed](LICENSE)</sub>

</div>
