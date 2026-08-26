import type { ApiError, SimulationRequest, SimulationResult } from "./types";

async function readErrors(response: Response): Promise<string[]> {
  try {
    const body = (await response.json()) as ApiError;
    if (Array.isArray(body.errors) && body.errors.length > 0) {
      return body.errors;
    }
  } catch {
    // fall through to the generic message
  }
  return [`request failed with status ${response.status}`];
}

export async function simulate(request: SimulationRequest): Promise<SimulationResult> {
  const response = await fetch("/api/simulate", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(request),
  });
  if (!response.ok) {
    throw new Error((await readErrors(response)).join("\n"));
  }
  return (await response.json()) as SimulationResult;
}

export async function compare(request: SimulationRequest): Promise<SimulationResult[]> {
  const response = await fetch("/api/simulate", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(request),
  });
  if (!response.ok) {
    throw new Error((await readErrors(response)).join("\n"));
  }
  const body = (await response.json()) as { runs: SimulationResult[] };
  return body.runs;
}

export async function health(): Promise<{ ready: boolean; message: string }> {
  const response = await fetch("/api/health");
  return (await response.json()) as { ready: boolean; message: string };
}
