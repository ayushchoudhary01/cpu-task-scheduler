import {
  Bar,
  BarChart,
  CartesianGrid,
  Legend,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

import type { SimulationResult } from "../types";

// Three measures, all in ticks, so they can share one axis honestly.
//
// CPU utilisation is deliberately not here: it is a percentage, and putting it
// on the same axis as a tick count would mean two scales on one chart - the
// single most misleading thing a chart can do.
const SERIES = [
  { key: "waiting", label: "Waiting", color: "#3987e5" },
  { key: "turnaround", label: "Turnaround", color: "#d95926" },
  { key: "response", label: "Response", color: "#199e70" },
] as const;

export default function MetricComparison({ results }: { results: SimulationResult[] }) {
  const data = results.map((result) => ({
    algorithm: result.algorithm,
    waiting: Number(result.averages.waitingTime.toFixed(2)),
    turnaround: Number(result.averages.turnaroundTime.toFixed(2)),
    response: Number(result.averages.responseTime.toFixed(2)),
  }));

  return (
    <div className="rounded-xl border border-edge bg-surface-raised p-5">
      <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">
        Averages, in ticks
      </h2>
      <p className="mt-1 text-xs text-ink-dim">Lower is better on all three.</p>

      <div className="mt-5 h-72 w-full">
        <ResponsiveContainer width="100%" height="100%">
          <BarChart data={data} margin={{ top: 8, right: 8, bottom: 8, left: 0 }} barGap={2}>
            <CartesianGrid stroke="#1e293b" vertical={false} />
            <XAxis
              dataKey="algorithm"
              tick={{ fill: "#94a3b8", fontSize: 12 }}
              axisLine={{ stroke: "#334155" }}
              tickLine={false}
            />
            <YAxis
              tick={{ fill: "#94a3b8", fontSize: 12 }}
              axisLine={false}
              tickLine={false}
              width={40}
            />
            <Tooltip
              cursor={{ fill: "#1e293b66" }}
              contentStyle={{
                backgroundColor: "#0f172a",
                border: "1px solid #334155",
                borderRadius: "8px",
                fontSize: "12px",
              }}
              labelStyle={{ color: "#e2e8f0" }}
            />
            <Legend
              wrapperStyle={{ fontSize: "12px", color: "#94a3b8" }}
              iconType="square"
              iconSize={10}
            />
            {SERIES.map((series) => (
              <Bar
                key={series.key}
                dataKey={series.key}
                name={series.label}
                fill={series.color}
                radius={[4, 4, 0, 0]}
                // Drawn immediately rather than animated in. A comparison
                // chart is there to be read, and animated bars do not render
                // at all if the tab is not painting when the data arrives.
                isAnimationActive={false}
              />
            ))}
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
