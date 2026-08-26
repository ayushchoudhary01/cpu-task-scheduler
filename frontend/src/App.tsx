import { Cpu, GitCompare, Play } from "lucide-react";
import { useEffect, useMemo, useState } from "react";

import { health } from "./api";
import { DEFAULT_PROCESSES } from "./components/ProcessEditor";
import { buildColorMap } from "./theme";
import type { Process } from "./types";
import CompareView from "./views/CompareView";
import SimulatorView from "./views/SimulatorView";

type View = "simulate" | "compare";

const TABS: { id: View; label: string; icon: typeof Play }[] = [
  { id: "simulate", label: "Simulate", icon: Play },
  { id: "compare", label: "Compare", icon: GitCompare },
];

export default function App() {
  const [view, setView] = useState<View>("simulate");

  // The workload lives here rather than in either view, so switching tabs keeps
  // whatever you typed - and a comparison runs on exactly what you just
  // simulated.
  const [processes, setProcesses] = useState<Process[]>(DEFAULT_PROCESSES);

  const [ready, setReady] = useState<boolean | null>(null);
  const [readyMessage, setReadyMessage] = useState("");

  // Colours follow the workload, not the run, so a process keeps its colour
  // across algorithms and across both views.
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

  return (
    <div className="min-h-screen">
      <header className="border-b border-edge">
        <div className="mx-auto max-w-5xl px-6 pt-5">
          <div className="flex items-center gap-3">
            <Cpu className="text-accent" size={22} />
            <div>
              <h1 className="text-lg font-semibold text-ink">CPU Scheduling Simulator</h1>
              <p className="text-xs text-ink-dim">
                Scheduling is done by a C++ program; this page sends it a workload and draws
                the result.
              </p>
            </div>
          </div>

          <nav className="mt-5 flex gap-1">
            {TABS.map((tab) => {
              const Icon = tab.icon;
              const active = view === tab.id;
              return (
                <button
                  key={tab.id}
                  type="button"
                  onClick={() => setView(tab.id)}
                  aria-current={active ? "page" : undefined}
                  className={`flex items-center gap-2 rounded-t-md border-b-2 px-4 py-2 text-sm transition-colors ${
                    active
                      ? "border-accent text-ink"
                      : "border-transparent text-ink-dim hover:text-ink"
                  }`}
                >
                  <Icon size={15} />
                  {tab.label}
                </button>
              );
            })}
          </nav>
        </div>
      </header>

      <main className="mx-auto max-w-5xl px-6 py-8">
        {view === "simulate" ? (
          <SimulatorView
            processes={processes}
            onProcessesChange={setProcesses}
            colors={colors}
            ready={ready}
            readyMessage={readyMessage}
          />
        ) : (
          <CompareView
            processes={processes}
            onProcessesChange={setProcesses}
            colors={colors}
            ready={ready}
          />
        )}
      </main>
    </div>
  );
}
