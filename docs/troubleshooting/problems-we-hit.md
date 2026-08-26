# Problems we hit

A record of everything that actually went wrong while building this, and what
fixed it. Some are environment problems, some are bugs we wrote, and some are
design flaws found by working an answer out on paper.

Kept because the fixes are more useful than the finished code alone, and because
several of these will happen again to anyone else building on Windows.

---

## Environment

### No C++ compiler at all

**Symptom.** Nothing to build with. `g++`, `gcc`, `clang`, `cl`, `make` and
`cmake` all absent.

**Cause.** Windows does not ship a C++ compiler, and VS Code does not include
one - it is an editor that calls tools already installed. Other languages hide
this because installing Python or Node brings a runtime with it.

**Fix.** `winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT`, then a new
terminal.

**Lesson.** "Which compiler?" is the first question on a C++ project, not an
afterthought. It should be settled before the first line of code.

---

### MSYS2 would not start

**Symptom.**

```
C:\msys64\usr\bin\msys-2.0.dll is either not designed to run on Windows
or it contains an error.  Error status 0xc0e90002.
```

MSYS2's own shell would not open. Driving it programmatically failed differently,
with `STATUS_DLL_INIT_FAILED`.

**Cause.** `msys-2.0.dll` is MSYS2's Unix emulation layer. When it cannot
initialise, nothing in MSYS2 runs. The file was present and a normal size, so
not a truncated download - most likely antivirus interference or a clash with
another copy of that DLL (Git for Windows ships one).

**Fix.** Abandoned MSYS2 for WinLibs, which is a plain folder of native Windows
executables with no emulation layer and nothing to fail.

**Lesson.** When a tool's *own* runtime is broken, debugging it costs more than
switching. Also: MSYS2 is a package manager, so installing it gives you no
compiler until you run `pacman` yourself - a step easy to miss.

---

### Smart App Control blocks freshly compiled binaries

**Symptom.**

```
Program 'scheduler.exe' failed to run: An Application Control policy has
blocked this file
```

Intermittent and maddening: `scheduler.exe` would run, then the test binary
compiled forty seconds later would not.

**Cause.** Windows Smart App Control was enforcing
(`VerifiedAndReputablePolicyState = 1`). It blocks unsigned executables without
established cloud reputation - which is every binary you compile. Confirmed in
Event Viewer under **CodeIntegrity/Operational**, events 3077 and 3118.

**Workaround.** A second build directory (`build-dbg/`) with different compiler
settings produces different binaries, which often get a different verdict.

**Real fix.** Turn Smart App Control off - but that is **permanent**, needing a
Windows reinstall to re-enable, so it is the machine owner's decision. Building
under WSL also sidesteps it.

**Lesson.** Smart App Control and compiling your own code do not really coexist.
Worth knowing before starting a C++ project on a machine that has it on.

---

### A stale binary in a second build directory

**Symptom.** The web API returned:

```
Bad arguments:
  unknown option '--format'
```

even though the CLI had supported `--format` for two chunks.

**Cause.** The Smart App Control workaround above meant builds were going to
`build-dbg/` while `build/` sat untouched. The server looks in `build/` first,
so it was running a binary four chunks out of date.

**Fix.** Rebuilt `build/`.

**Lesson.** Two build directories is a trap - the workaround created a second
bug. If you keep both, rebuild both, and check `scheduler.exe --list` when
behaviour looks impossible.

---

### npm installed into the repo root

**Symptom.** After the `web/` directory was renamed to `frontend/`, a
`node_modules/`, `package.json` and `package-lock.json` appeared at the top of
the repository.

**Cause.** An install command still pointed at `web/`. The directory change
failed but the command continued, so npm ran in the repository root and created
a package there.

**Fix.** Deleted the three stray artifacts and re-ran the install in
`frontend/`.

**Lesson.** Chaining a directory change and a command with `;` runs the command
whether or not the change worked.

---

## Bugs in our own code

### A UTF-8 byte order mark stuck to the first process id

**Symptom.** A process appeared in the Gantt chart as `﻿P1` with an invisible
character in front of it. Worse, duplicate detection silently stopped working -
a repeated `P1` went unreported.

**Cause.** Windows editors, Notepad included, write UTF-8 files with a byte
order mark: `EF BB BF`. The parser read those three bytes as part of the first
field, producing an id of `\ufeffP1`, which does not compare equal to `P1`.

**Fix.** Strip the mark from the first line in
[`cli/WorkloadParser.cpp`](../../cli/WorkloadParser.cpp), with a test asserting
both the clean id *and* that the duplicate is still caught.

**Lesson.** Found only by piping real input through the program. Unit tests used
clean strings and would never have produced a BOM. On Windows, assume text files
have one.

---

### The tie-break could not order same-tick arrivals

**Symptom.** Round Robin put the wrong process next when one arrived on exactly
the tick another's quantum expired.

**Cause.** Ties were broken on `readySince`, a tick number. Two processes
joining the queue on the same tick are indistinguishable by it, and the fallback
was input order - which produced the opposite of the conventional behaviour,
where a new arrival queues *ahead* of the process being preempted. In Round
Robin this case is not an edge case; it happens constantly.

**Fix.** The engine now stamps each process with a strictly increasing
`queueOrder` as it joins. Since arrivals are admitted before preemption is
handled, correct ordering falls out of the loop's structure.

**Lesson.** Found by computing Round Robin's expected output on paper and
noticing the code disagreed - *before* writing the test. Hand-computing expected
values is not busywork; it is a second implementation to disagree with.

---

### Recharts rendered axes but no bars

**Symptom.** The comparison chart drew its grid, axes and legend. No bars. The
`<g class="recharts-bar-rectangle">` elements existed in the DOM and were empty.

**Cause.** Recharts animates bars in with `requestAnimationFrame`. When the tab
is not painting, that never fires, so the bars are never drawn.

**Fix.** `isAnimationActive={false}`. Bars draw immediately - which is what you
want on a chart meant to be read rather than watched.

**Lesson.** A chart library's default animation is a rendering dependency, not
decoration.

---

## Dependency problems

### Recharts 2 was end-of-life

**Symptom.** npm warned on install that recharts 1.x and 2.x are no longer
maintained.

**Fix.** Upgraded to 3.x before writing any chart code, when there was nothing
to migrate.

**Lesson.** Deal with a deprecation warning at install time. The cost only grows.

---

### Blank page from a missing peer dependency

**Symptom.** The app rendered nothing. `tsc --noEmit` passed cleanly. The
browser console showed 500s and `504 (Outdated Optimize Dep)`. The real error
was in the Vite output:

```
X [ERROR] Could not resolve "react-is"
```

**Cause.** Recharts 3 declares `react-is` as a **peer** dependency, and npm does
not install peer dependencies automatically. Vite's dependency optimiser failed,
which took the whole app down.

**Fix.** `npm install react-is@^18` - version matched to React 18 - then cleared
`node_modules/.vite`, which was caching the failed optimisation.

**Lesson.** The one that generalises furthest: **a clean typecheck does not mean
the app runs.** TypeScript cannot see missing runtime dependencies. Load the
page.

---

## Mistakes in the tests themselves

### The test was wrong, not the code

**Symptom.**

```
FAIL: P1 finishes at 8  (got 7, expected 8)
```

**Cause.** Arithmetic error while hand-computing the expected SRTF schedule. P1
runs at tick 0, then ticks 3-6 - five ticks, finishing at 7. The test said 8.

**Fix.** Corrected the test. The code was right.

**Lesson.** The argument for hand-computed expectations. Had the test recorded
whatever the code printed, it would have passed and verified nothing. A test
that can only agree with the implementation is not a test.

---

## Bugs avoided by design

Problems present in the project this one was modelled on, designed out rather
than fixed:

| Problem there | Prevented here by |
|---|---|
| SRTF and priority scheduling hang forever, because a finished process is re-queued and its zero remaining time makes it look optimal | Retirement happens once, in the engine; a completed process cannot re-enter the ready queue whatever a policy does |
| An unhandled `'error'` event from a spawned process crashes the server | `child.on("error")` handled; every failure resolves into a response |
| JSON built by string concatenation with no escaping, so an id containing a quote corrupts the output | Proper escaping, with a test using ids containing `"` and `\` |
| An unrecognised algorithm name silently falls back to FCFS | `makePolicy` returns `nullptr`; the caller reports the problem |
| Process ids with spaces silently rewritten to underscores | Rejected, with an explanation |
| Priority direction never decided - the source contains the comment *"Assuming higher number = higher priority? Or lower?"* and contradicts its own README | Documented in `Process.hpp` and tested |

See [the engine](../architecture/the-engine.md) and
[SRTF](../algorithms/srtf.md#the-bug-this-algorithm-is-famous-for) for the
details.

---

## What kept working

Worth recording alongside the failures:

- **Hand-computed test values** caught a real design flaw (the tie-break) and a
  real arithmetic error, and never once produced a false pass.
- **Invariants over generated workloads** - 100 simulations checked against six
  rules - cover shapes nobody would think to write by hand, and would catch the
  infinite-loop class of bug immediately.
- **Checking the browser against the CLI** proved the two agree. Every number in
  the web interface was verified against the same workload in the terminal.
