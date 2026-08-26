import express from "express";
import { existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { findScheduler, runScheduler } from "./scheduler.js";
import { KNOWN_ALGORITHMS, validateRequest } from "./validate.js";
import type { SimulationRequest } from "../src/types.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const webRoot = path.resolve(here, "..");

const app = express();
app.use(express.json({ limit: "256kb" }));

// Turn a validated request into command line arguments and stdin text.
function buildInvocation(request: SimulationRequest): {
  args: string[];
  input: string;
} {
  const args = ["--format", "json"];

  if (request.compare && request.compare.length > 1) {
    args.push("--compare", request.compare.join(","));
  } else {
    args.push("--algorithm", request.algorithm ?? "FCFS");
  }
  args.push("--quantum", String(request.quantum ?? 2));
  args.push("--aging", String(request.agingRate ?? 0));

  const input = request.processes
    .map((p) => `${p.id} ${p.arrivalTime} ${p.burstTime} ${p.priority}`)
    .join("\n");

  return { args, input };
}

app.get("/api/algorithms", (_req, res) => {
  res.json({ algorithms: KNOWN_ALGORITHMS });
});

app.get("/api/health", (_req, res) => {
  const binary = findScheduler();
  res.json({
    ready: binary !== null,
    binary,
    message: binary
      ? "scheduler binary found"
      : "scheduler binary not found - build the C++ project first",
  });
});

app.post("/api/simulate", async (req, res) => {
  const { request, errors } = validateRequest(req.body);
  if (!request) {
    res.status(400).json({ errors });
    return;
  }

  const binary = findScheduler();
  if (!binary) {
    res.status(503).json({
      errors: [
        "The scheduler program has not been built yet.",
        "Run: cmake -S . -B build -G Ninja && cmake --build build",
      ],
    });
    return;
  }

  const { args, input } = buildInvocation(request);
  const outcome = await runScheduler(binary, args, input);

  if (outcome.timedOut) {
    res.status(504).json({ errors: ["the simulation took too long and was stopped"] });
    return;
  }
  if (outcome.code !== 0) {
    const detail = outcome.stderr.trim() || `exited with code ${outcome.code}`;
    res.status(500).json({ errors: ["the scheduler failed", detail] });
    return;
  }

  try {
    res.json(JSON.parse(outcome.stdout));
  } catch {
    res.status(500).json({ errors: ["the scheduler produced output that was not valid JSON"] });
  }
});

// In production the built front end is served from here too, so the whole
// thing runs as a single process on a single port.
const builtUi = path.join(webRoot, "dist");
if (existsSync(builtUi)) {
  app.use(express.static(builtUi));
}

const port = Number(process.env.PORT ?? 5174);
app.listen(port, () => {
  const binary = findScheduler();
  console.log(`API listening on http://localhost:${port}`);
  console.log(binary ? `Using scheduler: ${binary}` : "WARNING: scheduler binary not built yet");
});
