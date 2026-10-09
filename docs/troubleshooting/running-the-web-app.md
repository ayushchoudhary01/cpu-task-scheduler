# Troubleshooting: the web app

Start with the two health checks. Almost everything shows up in one of them.

```bash
curl http://localhost:5174/api/health
```

```json
{"ready":true,"binary":"C:\\...\\build\\bin\\scheduler.exe","message":"scheduler binary found"}
```

```bash
netstat -ano | grep LISTENING | grep -E ":517[34]"
```

You want **exactly two** lines - one for 5173 (UI) and one for 5174 (API).

## Which binary is the server using?

`/api/health` says, and so does the server's startup line:

```
Using scheduler: /home/ayush/scheduler-build/bin/scheduler (WSL: Ubuntu)
```

The server looks in this order:

1. `SCHEDULER_BIN`, if set - used exactly as given
2. a **Linux build inside WSL**, on Windows only
3. a native binary in `build/bin/` then `build-dbg/bin/`

A WSL build is preferred over a Windows one, which looks backwards until you
have met Smart App Control - it blocks the unsigned GCC runtime DLLs that a
MinGW build needs, killing the program before it reaches `main()`. See
[build and toolchain](build-and-toolchain.md#building-under-wsl).

Two environment variables adjust the WSL lookup:

| Variable | Default | Meaning |
|---|---|---|
| `SCHEDULER_WSL_DISTRO` | `Ubuntu` | which distribution to use |
| `SCHEDULER_WSL_PATH` | `scheduler-build/bin/scheduler` | path relative to the Linux `$HOME` |

To force the Windows binary instead:

```bash
SCHEDULER_BIN=build/bin/scheduler.exe npm run dev
```

## The page says "scheduler binary not found"

The C++ program has not been built. See [building](../getting-started/building.md):

```bash
cmake -S . -B build -G Ninja && cmake --build build
```

The server looks in `build/bin/` first, then `build-dbg/bin/`. You can point it
somewhere else:

```bash
SCHEDULER_BIN=/path/to/scheduler.exe npm run start
```

## "unknown option '--format'" or similar nonsense from the scheduler

**A stale binary.** The server found an older `scheduler.exe` that predates the
flag being used.

This happens when you have two build directories and only rebuild one. The
server prefers `build/`, so a fresh `build-dbg/` does not help.

```bash
cmake --build build
cmake --build build-dbg    # if you use it
```

Check what it is actually running:

```bash
./build/bin/scheduler.exe --list
```

## Requests hang forever, no error anywhere

Almost always a **port collision**, and it is sneaky.

Vite used to move to another port when 5173 was busy - straight onto 5174, where
the API lives. Both then appear to start fine, but `/api` requests proxy into
the wrong process and hang with nothing logged.

`vite.config.ts` now sets `strictPort: true`, so Vite fails loudly instead. If
you see the failure, something else is on 5173 - most likely an orphaned dev
server (see below).

## Orphaned dev servers

`npm run dev` starts two processes with `concurrently`. If one crashes, **the
other keeps running**. A failed start can leave Vite serving stale code from
5173 while its API half is dead - so you edit files and nothing changes, or the
page loads but every request fails.

Find and clear them:

```bash
netstat -ano | grep LISTENING | grep -E ":517[34]"
```

```bash
taskkill //PID <pid> //F
```

Then start again. If behaviour is inexplicable, do this **first** - you may not
be talking to the server you think you are.

## Blank page, no error in the UI

Check the browser console and the Vite output. A dependency failing to resolve
takes the whole app down while leaving the HTML shell in place:

```
X [ERROR] Could not resolve "react-is"
```

Fix the missing package and clear Vite's cache, which holds the failed
optimisation:

```bash
rm -rf node_modules/.vite
npm run dev
```

**`tsc --noEmit` passing does not mean the app runs.** Typechecking cannot see
missing runtime dependencies, dependency resolution failures, or anything that
happens in a browser. Always load the page.

## `504 (Outdated Optimize Dep)` in the console

Vite's dependency cache is stale, usually after installing or upgrading a
package while the dev server was running.

```bash
rm -rf node_modules/.vite
npm run dev
```

## Charts render axes but no bars

Recharts animates bars with `requestAnimationFrame`. If the tab is not painting
when the data arrives - backgrounded, or a headless-ish browser - the animation
never starts and the bars never draw. The `<g>` elements are in the DOM, empty.

Fixed here by disabling bar animation in
[`MetricComparison.tsx`](../../frontend/src/components/MetricComparison.tsx):

```tsx
isAnimationActive={false}
```

Worth remembering for any recharts component added later.

## The API rejects something the UI offered

The list of algorithms exists in two places:
[`frontend/src/algorithms.ts`](../../frontend/src/algorithms.ts) for the UI and
[`frontend/server/validate.ts`](../../frontend/server/validate.ts) for the
server. Add to one and not the other and the UI offers something the API
refuses.

## "id must be 1-16 letters, digits, dash or underscore"

Deliberate. Process ids become whitespace-separated fields on a line the C++
program parses, so an id containing a space would silently become two fields.

Rename the process rather than working around it.

## "this workload is too large to chart"

A deliberate limit, not a failure. The web interface accepts workloads totalling
at most **20,000 ticks of CPU time** across all processes.

The constraint is the size of the *result*, not the speed of the simulation. A
Gantt chart has one block per uninterrupted run, so the total burst time bounds
how many blocks there are - and at the worst case, Round Robin with a quantum of
1, every tick becomes its own block:

| Workload | Ticks | Time | JSON produced |
|---|--:|--:|--:|
| `examples/sample.txt` | 13 | 59 ms | 1 KB |
| 20 x 1000 *(the limit)* | 20,000 | 154 ms | 1.2 MB |
| 50 x 2000 | 100,000 | 297 ms | 5.6 MB |
| 200 x 10000 | 2,000,000 | **9.0 s** | **119 MB** |

That last row is two million blocks: too slow to produce, too large to send, and
unreadable even if it arrived.

Reduce the burst times, or use the command line, which has no such limit:

```bash
./build/bin/scheduler.exe -a RR -q 1 -i big-workload.txt
```

The quantum is the multiplier to watch - quantum 1 is the worst case, and
anything larger roughly halves the block count or better.

## Everything is slow, or a request times out

The server kills a simulation after 5 seconds and returns:

```json
{"errors":["the simulation took too long and was stopped"]}
```

With the size limit above in place this should no longer be reachable through
the UI. If you do see it, the likely cause is a very large `arrivalTime` - the
simulation ticks through idle time one tick at a time, so an arrival at tick
100,000 means 100,000 iterations before anything runs. There is a separate
8 MB cap on output, reported as *"the scheduler produced too much output"*.

## Checking against the CLI

The web interface and the terminal run the same binary, so they must agree. When
a number looks wrong, check it directly:

```bash
./build/bin/scheduler.exe -a SRTF -i examples/sample.txt
```

If the CLI agrees with the browser, the C++ is doing what it was asked and the
question is about the workload. If they disagree, the bug is in the web layer.
