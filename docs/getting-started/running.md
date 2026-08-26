# Running it

There are two ways in: a command line program, and a web interface. Both run the
same C++ code, so they always agree.

## The command line

```bash
./build/bin/scheduler.exe --algorithm SJF --input examples/sample.txt
```

```
Algorithm: SJF

Gantt chart
  +-------------------------+-----+---------------+--------------------+
  |           P1            | P3  |      P2       |         P4         |
  +-------------------------+-----+---------------+--------------------+
  0                         5     6               9                    13

Per process
  Process   Arrival   Burst  Completion  Turnaround  Waiting  Response
  P1              0       5           5           5        0         0
  P2              1       3           9           8        5         5
  P3              2       1           6           4        3         3
  P4              4       4          13           9        5         5

Averages
  Waiting time              3.25
  Turnaround time           6.50
  Response time             3.25
  CPU utilization         100.00 %
  Throughput                0.31 processes/tick
```

Block widths are proportional to how long each process held the CPU, so the
chart is readable at a glance rather than decorative.

### Options

| Option | Meaning |
|--------|---------|
| `-a`, `--algorithm NAME` | which algorithm to run (default `FCFS`) |
| `-c`, `--compare LIST` | run several on the same workload; comma separated, or `all` |
| `-f`, `--format FORMAT` | `text` or `json` (default `text`) |
| `-q`, `--quantum N` | time slice for Round Robin (default `2`) |
| `-g`, `--aging N` | one step of priority per N ticks waited (default `0`, off) |
| `-i`, `--input FILE` | workload file (default: standard input) |
| `--generate N` | make up a workload of N processes instead of reading one |
| `--seed S` | seed for `--generate`; the same seed always gives the same workload |
| `-l`, `--list` | list the available algorithms |
| `-h`, `--help` | show usage |

Algorithm names are matched without regard to case, so `srtf` and `SRTF` both
work.

### Comparing

```bash
./build/bin/scheduler.exe --compare all --input examples/sample.txt
```

```
  Algorithm   Waiting  Turnaround  Response    CPU %   Total
  FCFS           3.75        7.00      3.75   100.00      13
  SJF            3.25        6.50      3.25   100.00      13
  SRTF           2.50        5.75      1.25   100.00      13
  RR(q=2)        4.75        8.00      1.50   100.00      13
  Priority       3.75        7.00      3.75   100.00      13

  Lowest average waiting time:  SRTF (2.50)
  Lowest average response time: SRTF (1.25)
```

The `Total` column is identical on every row, and that is the point: the same
work takes the same time however you schedule it. Scheduling changes **who
waits**, not how much work there is.

### Making up a workload

```bash
./build/bin/scheduler.exe --generate 8 --seed 42 --compare all
```

The generated workload is printed above the results in the same format the
parser reads, so you can paste it into a file. The same seed always produces the
same workload, on any machine - which matters when you want to quote a result
and have someone else reproduce it.

### Reading from standard input

With no `--input`, the program reads stdin:

```bash
echo "A 0 4
B 1 2" | ./build/bin/scheduler.exe -a SRTF
```

## The workload file format

One process per line:

```
ID  ARRIVAL  BURST  [PRIORITY]
```

- **ID** - a short label, no spaces
- **ARRIVAL** - the tick at which it shows up; 0 or more
- **BURST** - how much CPU time it needs; at least 1
- **PRIORITY** - optional, defaults to 0. **Lower means more important.**

Blank lines are ignored, and everything after a `#` is a comment:

```
# A small mixed workload.
# ID  ARRIVAL  BURST  PRIORITY
P1    0        5      3
P2    1        3      1
P3    2        1      2
P4    4        4      4
```

Two examples ship with the project: [`examples/sample.txt`](../../examples/sample.txt)
and [`examples/starvation.txt`](../../examples/starvation.txt).

### When the file is wrong

Every problem is reported at once rather than one per run:

```
Problems in the workload:
  line 1: burst time 'five' is not a whole number
  line 2: arrival time cannot be negative
  line 4: duplicate process id 'P1'
```

## The web interface

```bash
cd frontend
npm install     # first time only
npm run dev
```

Then open **http://localhost:5173**.

That starts two things: the React app on 5173, and a small API server on 5174
that runs the C++ program. Requests to `/api` are proxied, so there is no CORS
to configure.

The interface has two tabs:

- **Simulate** - edit a workload, pick an algorithm, see a Gantt chart, per
  process metrics, and the averages.
- **Compare** - pick two or more algorithms and see their timelines stacked on a
  shared time scale, plus a chart and a table.

The workload is shared between the tabs, so you can simulate something and then
compare it without retyping.

If the page says *"scheduler binary not found"*, the C++ program has not been
built - see [building](building.md).
