import { AlertCircle, Cpu, Play } from "lucide-react";
import { useEffect, useMemo, useState } from "react";

import { health, simulate } from "./api";
import ProcessEditor, { DEFAULT_PROCESSES } from "./components/ProcessEditor";
import ResultView from "./components/ResultView";
import { buildColorMap } from "./theme";
import type { Process, SimulationResult } from "./types";

const ALGORITHMS = ["FCFS", "SJF", "SRTF", "RR", "Priority"] as const;

const DESCRIPTIONS: Record<string, string> = {
  FCFS: "Runs in arrival order, each to completion. Simple, but one long job delays everyone behind it.",
  SJF: "Runs the shortest waiting job first. Best average waiting time, but long jobs can starve.",
  SRTF: "Like SJF, but a shorter arrival takes over immediately. Best waiting time of all, at the cost of switching.",
  RR: "Everyone takes turns of a fixed length. Nobody starves and it feels responsive, but nothing finishes early.",
  Priority: "Most important first. Aging raises the priority of anything left waiting, so nothing starves forever.",
};

export default function App() {
  const [processes, setProcesses] = useState<Process[]>(DEFAULT_PROCESSES);
  const [algorithm, setAlgorithm] = useState<string>("SRTF");
  const [quantum, setQuantum] = useState(2);
  const [agingRate, setAgingRate] = useState(0);

  const [result, setResult] = useState<SimulationResult | null>(null);
  const [errors, setErrors] = useState<string[]>([]);
  const [running, setRunning] = useState(false);
  const [ready, setReady] = useState<boolean | null>(null);
  const [readyMessage, setReadyMessage] = useState("");

  // Colours follow the workload, not the run, so a process keeps its colour
  // when you switch algorithms.
  const colors = useMemo(
    () => buildColorMap(processes.map((process) => process.id)),
    [processes],
  );

  useEffect(() => {
    health()
      .then((info) => {
        setReady(info.ready);
        setReadyMessage(info.message);
      })
      .catch(() => {
        setReady(false);
        setReadyMessage("Cannot reach the API server. Is it running?");
      });
  }, []);

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
    <div className="min-h-screen">
      <header className="border-b border-edge">
        <div className="mx-auto flex max-w-5xl items-center gap-3 px-6 py-5">
          <Cpu className="text-accent" size={22} />
          <div>
            <h1 className="text-lg font-semibold text-ink">CPU Scheduling Simulator</h1>
            <p className="text-xs text-ink-dim">
              Scheduling is done by a C++ program; this page sends it a workload and draws
              the result.
            </p>
          </div>
        </div>
      </header>

      <main className="mx-auto max-w-5xl space-y-6 px-6 py-8">
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
          <h2 className="text-sm font-semibold tracking-wide text-ink-dim uppercase">
            Algorithm
          </h2>

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

          {/* Only show the knob that the chosen algorithm actually uses. */}
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
          onChange={setProcesses}
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

        {errors.length > 0 && (
          <div className="rounded-lg border border-red-900 bg-red-950/40 p-4 text-sm text-red-200">
            <p className="flex items-center gap-2 font-medium">
              <AlertCircle size={16} /> That did not work
            </p>
            <ul className="mt-2 list-inside list-disc space-y-1 text-red-300/90">
              {errors.map((message, index) => (
                <li key={index}>{message}</li>
              ))}
            </ul>
          </div>
        )}

        {result && <ResultView result={result} colors={colors} />}
      </main>
    </div>
  );
}
