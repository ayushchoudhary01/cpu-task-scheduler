import { AlertCircle, Trophy } from "lucide-react";
import { useState } from "react";

import { compare } from "../api";
import { ALGORITHMS } from "../algorithms";
import ErrorList from "../components/ErrorList";
import GanttChart from "../components/GanttChart";
import MetricComparison from "../components/MetricComparison";
import ProcessEditor from "../components/ProcessEditor";
import type { Process, SimulationResult } from "../types";

interface Props {
  processes: Process[];
  onProcessesChange: (processes: Process[]) => void;
  colors: Map<string, string>;
  ready: boolean | null;
}

// Which run has the lowest value for a given measure.
function bestBy(
  results: SimulationResult[],
  pick: (result: SimulationResult) => number,
): string {
  let best = results[0];
  for (const result of results) {
    if (pick(result) < pick(best)) best = result;
  }
  return best.algorithm;
}

export default function CompareView({
  processes,
  onProcessesChange,
  colors,
  ready,
}: Props) {
  const [selected, setSelected] = useState<string[]>(["FCFS", "SJF", "SRTF", "RR"]);
  const [quantum, setQuantum] = useState(2);
  const [agingRate, setAgingRate] = useState(0);

  const [results, setResults] = useState<SimulationResult[] | null>(null);
  const [errors, setErrors] = useState<string[]>([]);
  const [running, setRunning] = useState(false);

  const toggle = (name: string) => {
    setSelected((current) =>
      current.includes(name) ? current.filter((item) => item !== name) : [...current, name],
    );
  };

  const run = async () => {
    setRunning(true);
    setErrors([]);
    try {
      setResults(await compare({ processes, compare: selected, quantum, agingRate }));
    } catch (problem) {
      setErrors(problem instanceof Error ? problem.message.split("\n") : [String(problem)]);
      setResults(null);
    } finally {
      setRunning(false);
    }
  };

  const bestWaiting = results ? bestBy(results, (r) => r.averages.waitingTime) : "";
  const bestResponse = results ? bestBy(results, (r) => r.averages.responseTime) : "";

  return (
    <div className="space-y-6">
      <section className="rounded-xl border border-edge bg-surface-raised p-5">
        <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">
          Algorithms to compare
        </h2>
        <p className="mt-1 text-xs text-ink-dim">Pick at least two.</p>

        <div className="mt-4 flex flex-wrap gap-2">
          {ALGORITHMS.map((name) => (
            <button
              key={name}
              type="button"
              aria-pressed={selected.includes(name)}
              onClick={() => toggle(name)}
              className={`rounded-md border px-3 py-1.5 font-mono text-sm transition-colors ${
                selected.includes(name)
                  ? "border-accent bg-accent/15 text-accent"
                  : "border-edge text-ink-dim hover:border-slate-500 hover:text-ink"
              }`}
            >
              {name}
            </button>
          ))}
        </div>

        <div className="mt-4 flex flex-wrap gap-6">
          {selected.includes("RR") && (
            <label className="flex items-center gap-3 text-sm">
              <span className="text-ink-dim">Time quantum</span>
              <input
                type="number"
                min={1}
                value={quantum}
                onChange={(event) => setQuantum(Number(event.target.value) || 1)}
                className="w-20 rounded border border-edge bg-surface px-2 py-1 font-mono text-ink focus:border-accent focus:outline-none"
              />
            </label>
          )}
          {selected.includes("Priority") && (
            <label className="flex items-center gap-3 text-sm">
              <span className="text-ink-dim">Aging rate</span>
              <input
                type="number"
                min={0}
                value={agingRate}
                onChange={(event) => setAgingRate(Number(event.target.value) || 0)}
                className="w-20 rounded border border-edge bg-surface px-2 py-1 font-mono text-ink focus:border-accent focus:outline-none"
              />
            </label>
          )}
        </div>
      </section>

      <ProcessEditor
        processes={processes}
        colors={colors}
        onChange={onProcessesChange}
        disabled={running}
      />

      <button
        type="button"
        onClick={run}
        disabled={running || ready === false || selected.length < 2}
        className="flex w-full items-center justify-center gap-2 rounded-lg bg-accent px-4 py-3 font-medium text-slate-900 transition hover:brightness-110 disabled:cursor-not-allowed disabled:opacity-40"
      >
        {running ? "Running..." : `Compare ${selected.length} algorithms`}
      </button>

      {selected.length < 2 && (
        <p className="flex items-center gap-2 text-xs text-amber-300">
          <AlertCircle size={14} /> Comparing needs at least two algorithms.
        </p>
      )}

      <ErrorList errors={errors} />

      {results && (
        <div className="space-y-6">
          {/* Every run shares the same total time, so the charts can share one
              scale - which is what makes them comparable at a glance. */}
          <section className="rounded-xl border border-edge bg-surface-raised p-5">
            <div className="flex items-baseline justify-between">
              <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">
                Timelines
              </h2>
              <span className="font-mono text-xs text-ink-dim">
                all finish at tick {results[0].totalTime}
              </span>
            </div>

            <div className="mt-5 space-y-6">
              {results.map((result) => (
                <div key={result.algorithm}>
                  <div className="mb-2 font-mono text-xs text-ink">{result.algorithm}</div>
                  <GanttChart timeline={result.timeline} colors={colors} />
                </div>
              ))}
            </div>
          </section>

          <MetricComparison results={results} />

          <section className="overflow-x-auto rounded-xl border border-edge bg-surface-raised p-5">
            <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">
              Side by side
            </h2>
            <table className="mt-4 w-full min-w-[520px] text-sm">
              <thead>
                <tr className="text-xs tracking-wide text-ink-dim uppercase">
                  <th className="pb-2 text-left font-medium">Algorithm</th>
                  <th className="pb-2 text-right font-medium">Waiting</th>
                  <th className="pb-2 text-right font-medium">Turnaround</th>
                  <th className="pb-2 text-right font-medium">Response</th>
                  <th className="pb-2 text-right font-medium">CPU %</th>
                  <th className="pb-2 text-right font-medium">Total</th>
                </tr>
              </thead>
              <tbody className="font-mono">
                {results.map((result) => (
                  <tr key={result.algorithm} className="border-t border-edge/60">
                    <td className="py-2 text-left text-ink">{result.algorithm}</td>
                    <td className="py-2 text-right">
                      <span className={result.algorithm === bestWaiting ? "text-emerald-400" : ""}>
                        {result.averages.waitingTime.toFixed(2)}
                      </span>
                    </td>
                    <td className="py-2 text-right">
                      {result.averages.turnaroundTime.toFixed(2)}
                    </td>
                    <td className="py-2 text-right">
                      <span className={result.algorithm === bestResponse ? "text-emerald-400" : ""}>
                        {result.averages.responseTime.toFixed(2)}
                      </span>
                    </td>
                    <td className="py-2 text-right text-ink-dim">
                      {result.averages.cpuUtilization.toFixed(1)}
                    </td>
                    <td className="py-2 text-right text-ink-dim">{result.totalTime}</td>
                  </tr>
                ))}
              </tbody>
            </table>

            <div className="mt-4 space-y-1 text-sm">
              <p className="flex items-center gap-2 text-ink-dim">
                <Trophy size={14} className="text-emerald-400" />
                Lowest average waiting time: <span className="text-ink">{bestWaiting}</span>
              </p>
              <p className="flex items-center gap-2 text-ink-dim">
                <Trophy size={14} className="text-emerald-400" />
                Lowest average response time: <span className="text-ink">{bestResponse}</span>
              </p>
            </div>

            <p className="mt-4 border-t border-edge/60 pt-4 text-xs text-ink-dim">
              Every algorithm finishes at the same tick, because the work is the same either
              way. Scheduling changes who waits, not how much work there is.
            </p>
          </section>
        </div>
      )}
    </div>
  );
}
