# Problems we hit

A record of everything that actually went wrong while building this, and what
fixed it. Some are environment problems, some are bugs we wrote, and some are
design flaws found by working an answer out on paper.

Kept because the fixes are more useful than the finished code alone, and because
several of these will happen again to anyone else building on Windows.


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

### Smart App Control escalated to blocking the toolchain

**Symptom.** Having previously blocked compiled binaries now and then, Smart App
Control started blocking the compiler itself:

```
cmake   -> 0xC0E90002
ninja   -> 0xC0E90002
g++     -> exit 1, and no error message at all
```

`g++ --version` still worked, which made it look fine; actually compiling
anything failed silently, because the internal `cc1plus` process was being
killed at load time.

The web app failed at the same time with:

```
the scheduler failed
exited with code 3236495362
```

**Cause.** 3236495362 is 0xC0E90002, a Windows process-launch failure. The event
log named the exact file - `scheduler.exe` was being killed while loading
**`libgcc_s_seh-1.dll`**, an unsigned support library that ships with the MinGW
compiler. The program never reached `main()`, so there was nothing in our own
code to debug.

**Attempted fix.** Link the GCC runtime into the executable with
`-static-libgcc -static-libstdc++`, removing the dependency on that DLL. The
change is in `CMakeLists.txt` and is correct - but it cannot be applied on a
machine where the compiler itself is blocked.

**Real fix.** Build in WSL (below).

**Lesson.** Smart App Control and compiling your own code do not coexist, and it
gets stricter over time rather than settling down. On a machine where it is
enforcing, decide early: turn it off, or build in Linux. Working around it costs
more time than either.

---

### "Operation not permitted" building on the Windows drive

**Symptom.** Configuring the project from Ubuntu, against the source on `C:`,
failed repeatedly:

```
CMake Error at CMakeDetermineSystem.cmake:225 (configure_file):
  Operation not permitted
```

although the compiler itself was found and worked:

```
-- Check for working CXX compiler: /usr/bin/c++ - works
```

**Cause.** `configure_file` copies a file and then sets its permissions. The
Windows drive mounted at `/mnt/c` does not support Linux file permissions, so
every `chmod` failed.

**Fix.** Read the source from `/mnt/c`, write the build output into the Linux
filesystem:

```bash
cmake -S /mnt/c/Projects/cpu-task-scheduler -B ~/scheduler-build -G Ninja
cmake --build ~/scheduler-build
```

**Lesson.** In WSL, keep build output on the Linux side. It sidesteps the
permission problem completely and is several times faster, because file access
no longer crosses between the two systems. The source can stay on `C:` and git
keeps working normally.

---

### wsl.exe runs the wrong distribution

**Symptom.** Having built successfully in Ubuntu, running the binary from
Windows failed:

```
execvpe(/home/ayush/scheduler-build/bin/scheduler) failed: No such file or directory
```

Asking WSL where its home directory was gave an answer that was obviously not
Linux:

```
$ wsl.exe -e sh -c 'echo $HOME'
C:Usersayush
```

**Cause.** `wsl.exe` with no arguments runs the **default** distribution. With
Docker Desktop installed, that default is `docker-desktop`, not Ubuntu. The
binary genuinely did not exist there.

**Fix.** Name the distribution every time:

```bash
wsl.exe -d Ubuntu -e /home/ayush/scheduler-build/bin/scheduler --format json
```

The server does the same, with the distribution configurable through
`SCHEDULER_WSL_DISTRO`.

**Lesson.** A nonsensical answer - a Windows path reported as a Linux `$HOME` -
usually means you are talking to something other than what you think.

---

## Bugs in our own code

### The interface accepted workloads it could never draw

**Symptom.** The web app reported:

```
the simulation took too long and was stopped
```

**Cause.** Not the simulation - the size of its result. A Gantt chart has one
block per uninterrupted run, so the total burst time bounds how many blocks
there are. At the worst case, Round Robin with a quantum of 1, every single tick
becomes its own block.

Validation allowed 200 processes of 10000 ticks each. That is two million
blocks:

| Workload | Ticks | Time | JSON produced |
|---|--:|--:|--:|
| `examples/sample.txt` | 13 | 59 ms | 1 KB |
| 20 x 1000 | 20,000 | 154 ms | 1.2 MB |
| 50 x 2000 | 100,000 | 297 ms | 5.6 MB |
| 200 x 10000 | 2,000,000 | **9.0 s** | **119 MB** |

Nine seconds against a five second limit - and even if it had arrived, no
browser would draw two million elements and no human could read them.

**Fix.** Cap the *total* work in `validate.ts` and fail immediately with a
message that says what to change. The per-process limits were not enough on
their own, because twenty processes of 1000 ticks cost exactly as much as one of
20000.

The Gantt chart also now skips its entry animation above 300 blocks; thousands
of elements each running their own transition was the slowest thing on the page.

**Lesson.** Validate what the *output* will cost, not just whether the input
looks sensible. Every individual value here was within its limit; the
combination was not. And a timeout is a bad error message - it says something
took too long, not what the user should do differently.

---

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

Known failure modes for this kind of program, designed out rather than left to
be fixed later:

| Failure mode | Prevented by |
|---|---|
| Preemptive algorithms hang forever, because a finished process is re-queued and its zero remaining time makes it look optimal | Retirement happens once, in the engine; a completed process cannot re-enter the ready queue whatever a policy does |
| An unhandled `'error'` event from a spawned process crashes the server | `child.on("error")` handled; every failure resolves into a response |
| JSON built by string concatenation with no escaping, so an id containing a quote corrupts the output | Proper escaping, with a test using ids containing `"` and `\` |
| An unrecognised algorithm name silently falls back to FCFS | `makePolicy` returns `nullptr`; the caller reports the problem |
| Process ids with spaces silently rewritten to underscores | Rejected, with an explanation |
| The priority direction left implicit, so the docs and the code drift apart | Documented in `Process.hpp` and tested |

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
