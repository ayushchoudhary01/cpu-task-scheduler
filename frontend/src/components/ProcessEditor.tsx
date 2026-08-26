import { Plus, Shuffle, Trash2 } from "lucide-react";

import type { Process } from "../types";

interface Props {
  processes: Process[];
  colors: Map<string, string>;
  onChange: (processes: Process[]) => void;
  disabled: boolean;
}

const SAMPLE: Process[] = [
  { id: "P1", arrivalTime: 0, burstTime: 5, priority: 3 },
  { id: "P2", arrivalTime: 1, burstTime: 3, priority: 1 },
  { id: "P3", arrivalTime: 2, burstTime: 1, priority: 2 },
  { id: "P4", arrivalTime: 4, burstTime: 4, priority: 4 },
];

export const DEFAULT_PROCESSES = SAMPLE;

function randomWorkload(count: number): Process[] {
  const between = (low: number, high: number) =>
    low + Math.floor(Math.random() * (high - low + 1));

  return Array.from({ length: count }, (_unused, index) => ({
    id: `P${index + 1}`,
    arrivalTime: between(0, 8),
    burstTime: between(1, 9),
    priority: between(1, 5),
  })).sort((a, b) => a.arrivalTime - b.arrivalTime);
}

export default function ProcessEditor({ processes, colors, onChange, disabled }: Props) {
  const update = (index: number, field: keyof Process, value: string) => {
    const next = processes.map((process, position) => {
      if (position !== index) return process;
      if (field === "id") return { ...process, id: value };
      // An empty box should read as 0 while typing rather than NaN.
      const parsed = value === "" ? 0 : Number.parseInt(value, 10);
      return { ...process, [field]: Number.isNaN(parsed) ? 0 : parsed };
    });
    onChange(next);
  };

  const addProcess = () => {
    const used = new Set(processes.map((process) => process.id));
    let n = processes.length + 1;
    while (used.has(`P${n}`)) n += 1;
    onChange([...processes, { id: `P${n}`, arrivalTime: 0, burstTime: 3, priority: 1 }]);
  };

  const removeProcess = (index: number) => {
    onChange(processes.filter((_unused, position) => position !== index));
  };

  return (
    <section className="rounded-xl border border-edge bg-surface-raised p-5">
      <div className="flex items-center justify-between">
        <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">Workload</h2>
        <div className="flex gap-2">
          <button
            type="button"
            onClick={() => onChange(randomWorkload(5))}
            disabled={disabled}
            className="flex items-center gap-1.5 rounded-md border border-edge px-3 py-1.5 text-xs text-ink-dim hover:border-slate-500 hover:text-ink disabled:opacity-40"
          >
            <Shuffle size={14} /> Random
          </button>
          <button
            type="button"
            onClick={() => onChange(SAMPLE)}
            disabled={disabled}
            className="rounded-md border border-edge px-3 py-1.5 text-xs text-ink-dim hover:border-slate-500 hover:text-ink disabled:opacity-40"
          >
            Sample
          </button>
        </div>
      </div>

      <div className="mt-4 overflow-x-auto">
        <table className="w-full min-w-[420px] text-sm">
          <thead>
            <tr className="text-left text-xs tracking-wide text-ink-dim uppercase">
              <th className="pb-2 font-medium">Process</th>
              <th className="pb-2 font-medium">Arrival</th>
              <th className="pb-2 font-medium">Burst</th>
              <th className="pb-2 font-medium">Priority</th>
              <th className="pb-2" />
            </tr>
          </thead>
          <tbody>
            {processes.map((process, index) => (
              <tr key={index} className="border-t border-edge/60">
                <td className="py-2 pr-3">
                  <div className="flex items-center gap-2">
                    <span
                      className="h-3 w-3 shrink-0 rounded-sm"
                      style={{ backgroundColor: colors.get(process.id) ?? "#64748b" }}
                      aria-hidden
                    />
                    <input
                      value={process.id}
                      onChange={(event) => update(index, "id", event.target.value)}
                      disabled={disabled}
                      aria-label={`Name of process ${index + 1}`}
                      className="w-24 rounded border border-edge bg-surface px-2 py-1 font-mono text-ink focus:border-accent focus:outline-none"
                    />
                  </div>
                </td>
                {(["arrivalTime", "burstTime", "priority"] as const).map((field) => (
                  <td key={field} className="py-2 pr-3">
                    <input
                      type="number"
                      value={process[field]}
                      onChange={(event) => update(index, field, event.target.value)}
                      disabled={disabled}
                      aria-label={`${field} of ${process.id}`}
                      className="w-20 rounded border border-edge bg-surface px-2 py-1 font-mono text-ink focus:border-accent focus:outline-none"
                    />
                  </td>
                ))}
                <td className="py-2">
                  <button
                    type="button"
                    onClick={() => removeProcess(index)}
                    disabled={disabled || processes.length === 1}
                    aria-label={`Remove ${process.id}`}
                    className="rounded p-1.5 text-ink-dim hover:bg-red-500/10 hover:text-red-400 disabled:opacity-30 disabled:hover:bg-transparent"
                  >
                    <Trash2 size={15} />
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <button
        type="button"
        onClick={addProcess}
        disabled={disabled}
        className="mt-3 flex items-center gap-1.5 rounded-md border border-dashed border-edge px-3 py-1.5 text-xs text-ink-dim hover:border-accent hover:text-accent disabled:opacity-40"
      >
        <Plus size={14} /> Add process
      </button>
    </section>
  );
}
