# The JSON contract

What the C++ prints with `--format json`, and what the browser expects. Defined
in [`cli/JsonReport.cpp`](../../cli/JsonReport.cpp) and mirrored as TypeScript in
[`frontend/src/types.ts`](../../frontend/src/types.ts).

## One simulation

```bash
./build/bin/scheduler.exe -a SJF -f json -i examples/sample.txt
```

```json
{
    "algorithm": "SJF",
    "totalTime": 13,
    "busyTime": 13,
    "timeline": [
      {"start": 0, "end": 5, "processId": "P1"},
      {"start": 5, "end": 6, "processId": "P3"},
      {"start": 6, "end": 9, "processId": "P2"},
      {"start": 9, "end": 13, "processId": "P4"}
    ],
    "processes": [
      {"id": "P1", "arrivalTime": 0, "burstTime": 5, "completionTime": 5,
       "turnaroundTime": 5, "waitingTime": 0, "responseTime": 0}
    ],
    "averages": {"waitingTime": 3.2500, "turnaroundTime": 6.5000,
                 "responseTime": 3.2500, "cpuUtilization": 100.0000,
                 "throughput": 0.3077}
}
```

## Several simulations

With `--compare`, the same objects are wrapped:

```json
{
  "runs": [ { ... }, { ... } ]
}
```

## Fields

### Top level

| Field | Type | Meaning |
|-------|------|---------|
| `algorithm` | string | display name, e.g. `"RR(q=2)"` or `"Priority(aging=3)"` |
| `totalTime` | int | tick at which everything had finished |
| `busyTime` | int | ticks the CPU was running something |
| `timeline` | array | what the CPU did, in order |
| `processes` | array | per process results, **in input order** |
| `averages` | object | summary figures |

`algorithm` includes the settings that were used, so a comparison of two Round
Robins with different quanta is readable without extra context.

### `timeline[]`

| Field | Type | Meaning |
|-------|------|---------|
| `start` | int | first tick of this block |
| `end` | int | one past the last tick |
| `processId` | string or **null** | which process, or `null` while idle |

Ranges are **half-open**: `{"start": 0, "end": 3}` covers ticks 0, 1 and 2. So
one block's `end` is the next block's `start`, and duration is `end - start`
with no off-by-one.

Blocks are merged: a process running ticks 0-2 is one entry, not three. Idle
stretches are `processId: null` rather than an empty string or a magic name -
`null` cannot collide with a real process id.

### `processes[]`

| Field | Meaning |
|-------|---------|
| `id` | the process |
| `arrivalTime`, `burstTime` | echoed from the input |
| `completionTime` | tick it finished |
| `turnaroundTime` | `completionTime - arrivalTime` |
| `waitingTime` | `turnaroundTime - burstTime` |
| `responseTime` | first run tick minus arrival |

Order matches the input file, not completion order, so a table rendered from
this stays in the order the user typed.

### `averages`

All doubles. `cpuUtilization` is a **percentage** (0-100); `throughput` is
processes per tick.

## Two details that matter

### Strings are escaped

Process ids come from user input, so a quote or backslash would otherwise
produce broken JSON. Quotes, backslashes and control characters are escaped,
with `\uXXXX` for anything below `0x20`:

```
id "a\"b"  ->  "a\\\"b"
```

The reference implementation builds its JSON with raw `cout <<` concatenation
and no escaping at all, so an id containing a quote corrupts its output
silently. There is a test here for exactly that.

### Numbers are never in scientific notation

Doubles are printed with fixed notation to four decimal places. A long-running
simulation gives a throughput like `0.00005`, which a naive writer emits as
`5e-05` - valid JSON, but awkward to read and handled badly by some parsers.

`testJsonAvoidsScientificNotation` runs a 20,000-tick workload and asserts no
`e-` appears anywhere in the output.

## Verifying it

The tests check bracket balance, but that is not proof it parses. Pipe it
through a real parser:

```bash
./build/bin/scheduler.exe --compare all -f json -i examples/sample.txt > out.json
node -e "const j=require('./out.json'); console.log(j.runs.length, 'runs')"
```

## Changing the shape

If you add a field, change both sides:

1. `cli/JsonReport.cpp` - write it
2. `frontend/src/types.ts` - declare it
3. `tests/CliTests.cpp` - assert it is present

TypeScript will not catch a mismatch, because the JSON is cast to the interface
at the boundary rather than validated against it. `tsc --noEmit` passing does
not mean the server is sending what the browser expects.
