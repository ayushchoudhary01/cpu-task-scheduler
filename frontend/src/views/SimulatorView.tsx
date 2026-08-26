import { AlertCircle, Play } from "lucide-react";
import { useState } from "react";

import { simulate } from "../api";
import ErrorList from "../components/ErrorList";
import ProcessEditor from "../components/ProcessEditor";
import ResultView from "../components/ResultView";
import { ALGORITHMS, DESCRIPTIONS } from "../algorithms";
import type { Process, SimulationResult } from "../types";

interface Props {
  processes: Process[];
  onProcessesChange: (processes: Process[]) => void;
  colors: Map<string, string>;
  ready: boolean | null;
  readyMessage: string;
}

export default function SimulatorView({
  processes,
  onProcessesChange,
  colors,
  ready,
  readyMessage,
}: Props) {
  const [algorithm, setAlgorithm] = useState<string>("SRTF");
  const [quantum, setQuantum] = useState(2);
  const [agingRate, setAgingRate] = useState(0);

  const [result, setResult] = useState<SimulationResult | null>(null);
  const [errors, setErrors] = useState<string[]>([]);
  const [running, setRunning] = useState(false);

  const run = async () => {
    setRunning(true);
    setErrors([]);
    try {
      setResult(await simulate({ processes, algorithm, quantum, agingRate }));
    } catch (problem) {
      setErrors(problem instanceof Error ? problem.message.split("\n") : [String(problem)]);
      setResult(null);
    } finally {
      setRunning(false);
    }
  };

  return (
    <div className="space-y-6">
      {ready === false && (
        <div className="flex items-start gap-3 rounded-lg border border-amber-800 bg-amber-950/40 p-4 text-sm text-amber-200">
          <AlertCircle size={18} className="mt-0.5 shrink-0" />
          <div>
            <p className="font-medium">Not ready to run</p>
            <p className="mt-1 text-amber-300/80">{readyMessage}</p>
          </div>
        </div>
      )}

      <section className="rounded-xl border border-edge bg-surface-raised p-5">
        <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">Algorithm</h2>

        <div className="mt-4 flex flex-wrap gap-2">
          {ALGORITHMS.map((name) => (
            <button
              key={name}
              type="button"
              onClick={() => setAlgorithm(name)}
              className={`rounded-md border px-3 py-1.5 font-mono text-sm transition-colors ${
                algorithm === name
                  ? "border-accent bg-accent/15 text-accent"
                  : "border-edge text-ink-dim hover:border-slate-500 hover:text-ink"
              }`}
            >
              {name}
            </button>
          ))}
        </div>

        <p className="mt-3 text-sm text-ink-dim">{DESCRIPTIONS[algorithm]}</p>

        {/* Only show the knob the chosen algorithm actually uses. */}
        {algorithm === "RR" && (
          <label className="mt-4 flex items-center gap-3 text-sm">
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

        {algorithm === "Priority" && (
          <label className="mt-4 flex items-center gap-3 text-sm">
            <span className="text-ink-dim">Aging rate</span>
            <input
              type="number"
              min={0}
              value={agingRate}
              onChange={(event) => setAgingRate(Number(event.target.value) || 0)}
              className="w-20 rounded border border-edge bg-surface px-2 py-1 font-mono text-ink focus:border-accent focus:outline-none"
            />
            <span className="text-xs text-ink-dim">0 turns aging off</span>
          </label>
        )}
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
        disabled={running || ready === false || processes.length === 0}
        className="flex w-full items-center justify-center gap-2 rounded-lg bg-accent px-4 py-3 font-medium text-slate-900 transition hover:brightness-110 disabled:cursor-not-allowed disabled:opacity-40"
      >
        <Play size={17} />
        {running ? "Running..." : "Run simulation"}
      </button>

      <ErrorList errors={errors} />

      {result && <ResultView result={result} colors={colors} />}
    </div>
  );
}
