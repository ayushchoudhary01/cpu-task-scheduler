import { useEffect, useState } from "react";

import { health, simulate } from "./api";
import type { SimulationResult } from "./types";

// A deliberately small workload, used to prove the browser can reach the API
// and the API can reach the C++ program. The real interface arrives next.
const SAMPLE = [
  { id: "P1", arrivalTime: 0, burstTime: 5, priority: 3 },
  { id: "P2", arrivalTime: 1, burstTime: 3, priority: 1 },
  { id: "P3", arrivalTime: 2, burstTime: 1, priority: 2 },
];

export default function App() {
  const [status, setStatus] = useState("checking...");
  const [ready, setReady] = useState(false);
  const [result, setResult] = useState<SimulationResult | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    health()
      .then((info) => {
        setStatus(info.message);
        setReady(info.ready);
      })
      .catch(() => setStatus("cannot reach the API server"));
  }, []);

  const run = async () => {
    setError(null);
    try {
      setResult(await simulate({ processes: SAMPLE, algorithm: "SRTF" }));
    } catch (problem) {
      setError(problem instanceof Error ? problem.message : String(problem));
      setResult(null);
    }
  };

  return (
    <main className="mx-auto max-w-3xl px-6 py-12">
      <h1 className="text-2xl font-semibold text-ink">CPU Scheduling Simulator</h1>
      <p className="mt-2 text-sm text-ink-dim">
        The scheduling itself is done by a C++ program. This page sends it a workload and
        draws what comes back.
      </p>

      <div className="mt-8 rounded-lg border border-edge bg-surface-raised p-5">
        <h2 className="text-sm font-medium tracking-wide text-ink-dim uppercase">
          Connection
        </h2>
        <p className="mt-2 flex items-center gap-2 text-sm">
          <span
            className={`inline-block h-2 w-2 rounded-full ${
              ready ? "bg-emerald-400" : "bg-amber-400"
            }`}
          />
          {status}
        </p>

        <button
          onClick={run}
          disabled={!ready}
          className="mt-4 rounded-md bg-accent px-4 py-2 text-sm font-medium text-slate-900 hover:brightness-110 disabled:cursor-not-allowed disabled:opacity-40"
        >
          Run a test simulation
        </button>
      </div>

      {error && (
        <pre className="mt-6 rounded-lg border border-red-900 bg-red-950/40 p-4 text-sm whitespace-pre-wrap text-red-200">
          {error}
        </pre>
      )}

      {result && (
        <div className="mt-6 rounded-lg border border-edge bg-surface-raised p-5">
          <h2 className="text-sm font-medium tracking-wide text-ink-dim uppercase">
            {result.algorithm}
          </h2>
          <p className="mt-2 text-sm text-ink-dim">
            finished at tick {result.totalTime}, average waiting time{" "}
            {result.averages.waitingTime.toFixed(2)}
          </p>
          <div className="mt-4 flex overflow-hidden rounded border border-edge">
            {result.timeline.map((slice, index) => (
              <div
                key={index}
                className={`px-2 py-3 text-center text-xs font-mono ${
                  slice.processId ? "bg-accent/20 text-ink" : "bg-surface text-ink-dim"
                }`}
                style={{ flexGrow: slice.end - slice.start }}
              >
                {slice.processId ?? "idle"}
              </div>
            ))}
          </div>
        </div>
      )}
    </main>
  );
}
