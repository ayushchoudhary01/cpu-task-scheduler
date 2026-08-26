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

## Everything is slow, or a request times out

The server kills a simulation after 5 seconds and returns:

```json
{"errors":["the simulation took too long and was stopped"]}
```

Normal workloads finish in microseconds, so a timeout means something is
genuinely wrong - most likely a huge `arrivalTime`, since the simulation ticks
through idle time one tick at a time. An arrival at tick 100,000 means 100,000
iterations before anything runs.

## Checking against the CLI

The web interface and the terminal run the same binary, so they must agree. When
a number looks wrong, check it directly:

```bash
./build/bin/scheduler.exe -a SRTF -i examples/sample.txt
```

If the CLI agrees with the browser, the C++ is doing what it was asked and the
question is about the workload. If they disagree, the bug is in the web layer.
