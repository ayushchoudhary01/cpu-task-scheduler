import type { SimulationResult } from "../types";
import GanttChart from "./GanttChart";

interface Props {
  result: SimulationResult;
  colors: Map<string, string>;
}

function StatTile({ label, value, unit }: { label: string; value: string; unit?: string }) {
  return (
    <div className="rounded-lg border border-edge bg-surface px-4 py-3">
      <div className="text-xs tracking-wide text-ink-dim uppercase">{label}</div>
      <div className="mt-1 font-mono text-xl text-ink">
        {value}
        {unit && <span className="ml-1 text-sm text-ink-dim">{unit}</span>}
      </div>
    </div>
  );
}

export default function ResultView({ result, colors }: Props) {
  const { averages } = result;

  return (
    <section className="space-y-6">
      <div className="rounded-xl border border-edge bg-surface-raised p-5">
        <div className="flex items-baseline justify-between">
          <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">
            Timeline
          </h2>
          <span className="font-mono text-xs text-ink-dim">
            {result.algorithm} &middot; finished at tick {result.totalTime}
          </span>
        </div>
        <div className="mt-6">
          <GanttChart timeline={result.timeline} colors={colors} />
        </div>
      </div>

      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
        <StatTile label="Avg waiting" value={averages.waitingTime.toFixed(2)} />
        <StatTile label="Avg turnaround" value={averages.turnaroundTime.toFixed(2)} />
        <StatTile label="Avg response" value={averages.responseTime.toFixed(2)} />
        <StatTile label="CPU used" value={averages.cpuUtilization.toFixed(1)} unit="%" />
      </div>

      {/* The same numbers as a table. The chart above is the quick read; this is
          what you check when the chart tells you something surprising. */}
      <div className="overflow-x-auto rounded-xl border border-edge bg-surface-raised p-5">
        <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">
          Per process
        </h2>
        <table className="mt-4 w-full min-w-[520px] text-sm">
          <thead>
            <tr className="text-xs tracking-wide text-ink-dim uppercase">
              <th className="pb-2 text-left font-medium">Process</th>
              <th className="pb-2 text-right font-medium">Arrival</th>
              <th className="pb-2 text-right font-medium">Burst</th>
              <th className="pb-2 text-right font-medium">Completed</th>
              <th className="pb-2 text-right font-medium">Turnaround</th>
              <th className="pb-2 text-right font-medium">Waiting</th>
              <th className="pb-2 text-right font-medium">Response</th>
            </tr>
          </thead>
          <tbody className="font-mono">
            {result.processes.map((row) => (
              <tr key={row.id} className="border-t border-edge/60">
                <td className="py-2 text-left">
                  <span className="flex items-center gap-2">
                    <span
                      className="h-3 w-3 rounded-sm"
                      style={{ backgroundColor: colors.get(row.id) ?? "#64748b" }}
                      aria-hidden
                    />
                    {row.id}
                  </span>
                </td>
                <td className="py-2 text-right text-ink-dim">{row.arrivalTime}</td>
                <td className="py-2 text-right text-ink-dim">{row.burstTime}</td>
                <td className="py-2 text-right">{row.completionTime}</td>
                <td className="py-2 text-right">{row.turnaroundTime}</td>
                <td className="py-2 text-right">{row.waitingTime}</td>
                <td className="py-2 text-right">{row.responseTime}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </section>
  );
}
