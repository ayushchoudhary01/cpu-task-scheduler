import type { Process, SimulationRequest } from "../src/types.js";

// Process ids become whitespace separated fields on a line the C++ program
// reads, so an id containing a space would silently turn into two fields.
// Rather than quietly rewriting what the user typed, reject it.
const ID_PATTERN = /^[A-Za-z0-9_-]{1,16}$/;

const MAX_PROCESSES = 200;
const MAX_BURST = 10000;
const MAX_ARRIVAL = 100000;

export const KNOWN_ALGORITHMS = ["FCFS", "SJF", "SRTF", "RR", "Priority"] as const;

function isKnownAlgorithm(name: string): boolean {
  return KNOWN_ALGORITHMS.some(
    (known) => known.toLowerCase() === name.toLowerCase(),
  );
}

function isWholeNumber(value: unknown): value is number {
  return typeof value === "number" && Number.isInteger(value);
}

// Check a request from the browser. Returns every problem found, so a form with
// three mistakes reports all three.
export function validateRequest(body: unknown): {
  request: SimulationRequest | null;
  errors: string[];
} {
  const errors: string[] = [];

  if (typeof body !== "object" || body === null) {
    return { request: null, errors: ["request body must be an object"] };
  }

  const raw = body as Record<string, unknown>;

  if (!Array.isArray(raw.processes)) {
    errors.push("processes must be an array");
    return { request: null, errors };
  }
  if (raw.processes.length === 0) {
    errors.push("add at least one process");
  }
  if (raw.processes.length > MAX_PROCESSES) {
    errors.push(`at most ${MAX_PROCESSES} processes`);
  }

  const processes: Process[] = [];
  const seen = new Set<string>();

  raw.processes.forEach((entry: unknown, index: number) => {
    const where = `process ${index + 1}`;
    if (typeof entry !== "object" || entry === null) {
      errors.push(`${where}: not an object`);
      return;
    }
    const p = entry as Record<string, unknown>;

    if (typeof p.id !== "string" || !ID_PATTERN.test(p.id)) {
      errors.push(`${where}: id must be 1-16 letters, digits, dash or underscore`);
      return;
    }
    if (seen.has(p.id)) {
      errors.push(`${where}: duplicate id '${p.id}'`);
      return;
    }
    seen.add(p.id);

    if (!isWholeNumber(p.arrivalTime) || p.arrivalTime < 0 || p.arrivalTime > MAX_ARRIVAL) {
      errors.push(`${where}: arrival time must be a whole number from 0 to ${MAX_ARRIVAL}`);
      return;
    }
    if (!isWholeNumber(p.burstTime) || p.burstTime < 1 || p.burstTime > MAX_BURST) {
      errors.push(`${where}: burst time must be a whole number from 1 to ${MAX_BURST}`);
      return;
    }
    const priority = p.priority === undefined ? 0 : p.priority;
    if (!isWholeNumber(priority)) {
      errors.push(`${where}: priority must be a whole number`);
      return;
    }

    processes.push({
      id: p.id,
      arrivalTime: p.arrivalTime,
      burstTime: p.burstTime,
      priority,
    });
  });

  const algorithm = raw.algorithm === undefined ? "FCFS" : raw.algorithm;
  if (typeof algorithm !== "string" || !isKnownAlgorithm(algorithm)) {
    errors.push(`unknown algorithm '${String(algorithm)}'`);
  }

  let compare: string[] | undefined;
  if (raw.compare !== undefined) {
    if (!Array.isArray(raw.compare)) {
      errors.push("compare must be an array of algorithm names");
    } else {
      compare = [];
      for (const name of raw.compare) {
        if (typeof name !== "string" || !isKnownAlgorithm(name)) {
          errors.push(`unknown algorithm '${String(name)}'`);
        } else {
          compare.push(name);
        }
      }
      if (compare.length < 2) {
        errors.push("comparing needs at least two algorithms");
      }
    }
  }

  const quantum = raw.quantum === undefined ? 2 : raw.quantum;
  if (!isWholeNumber(quantum) || quantum < 1 || quantum > 1000) {
    errors.push("quantum must be a whole number from 1 to 1000");
  }

  const agingRate = raw.agingRate === undefined ? 0 : raw.agingRate;
  if (!isWholeNumber(agingRate) || agingRate < 0 || agingRate > 1000) {
    errors.push("aging rate must be a whole number from 0 to 1000");
  }

  if (errors.length > 0) {
    return { request: null, errors };
  }

  return {
    request: {
      processes,
      algorithm: algorithm as string,
      quantum: quantum as number,
      agingRate: agingRate as number,
      compare,
    },
    errors: [],
  };
}
