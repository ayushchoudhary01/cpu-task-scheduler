import assert from "node:assert/strict";
import type { Server } from "node:http";
import { after, before, describe, it } from "node:test";

import { createApp } from "../server/app.js";
import type { SimulationResult } from "../src/types.js";

// These are integration tests: they start the real server and it runs the real
// C++ program. If the project has not been built they are skipped rather than
// failed, because a missing binary is a setup problem, not a broken API.
let baseUrl = "";
let server: Server;
let schedulerReady = false;

const SAMPLE = [
  { id: "P1", arrivalTime: 0, burstTime: 5, priority: 3 },
  { id: "P2", arrivalTime: 1, burstTime: 3, priority: 1 },
  { id: "P3", arrivalTime: 2, burstTime: 1, priority: 2 },
  { id: "P4", arrivalTime: 4, burstTime: 4, priority: 4 },
];

function post(body: unknown): Promise<Response> {
  return fetch(`${baseUrl}/api/simulate`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
}

before(async () => {
  // Port 0 asks the operating system for any free port, so the tests never
  // collide with a development server that happens to be running.
  server = createApp().listen(0);
  await new Promise((resolve) => server.once("listening", resolve));
  const address = server.address();
  if (address === null || typeof address === "string") {
    throw new Error("server did not report a port");
  }
  baseUrl = `http://127.0.0.1:${address.port}`;

  const health = (await (await fetch(`${baseUrl}/api/health`)).json()) as { ready: boolean };
  schedulerReady = health.ready;
  if (!schedulerReady) {
    console.log("  (scheduler not built - integration tests will be skipped)");
  }
});

after(() => {
  server.close();
});

describe("GET /api/algorithms", () => {
  it("lists every algorithm the UI may offer", async () => {
    const body = (await (await fetch(`${baseUrl}/api/algorithms`)).json()) as {
      algorithms: string[];
    };

    assert.deepEqual(body.algorithms, ["FCFS", "SJF", "SRTF", "RR", "Priority"]);
  });
});

describe("GET /api/health", () => {
  it("says whether the scheduler can be found, and which one", async () => {
    const body = (await (await fetch(`${baseUrl}/api/health`)).json()) as {
      ready: boolean;
      binary: string | null;
      message: string;
    };

    assert.equal(typeof body.ready, "boolean");
    assert.ok(body.message.length > 0);
    if (body.ready) {
      assert.ok(body.binary, "a ready server must name the binary it will run");
    }
  });
});

describe("POST /api/simulate - rejections", () => {
  it("rejects a bad request with 400 and a list of reasons", async () => {
    const response = await post({ processes: [{ id: "bad name", arrivalTime: 0, burstTime: 1 }] });
    const body = (await response.json()) as { errors: string[] };

    assert.equal(response.status, 400);
    assert.ok(Array.isArray(body.errors) && body.errors.length > 0);
  });

  it("rejects an oversized workload without running anything", async () => {
    const started = Date.now();
    const response = await post({
      processes: Array.from({ length: 100 }, (_unused, i) => ({
        id: `P${i + 1}`,
        arrivalTime: 0,
        burstTime: 5000,
        priority: 0,
      })),
      algorithm: "RR",
      quantum: 1,
    });

    assert.equal(response.status, 400);
    // The point of the limit: refused immediately, not after a long timeout.
    assert.ok(Date.now() - started < 2000, "an oversized workload should fail fast");
  });
});

describe("POST /api/simulate - running", { skip: false }, () => {
  it("returns a complete result for one algorithm", async (t) => {
    if (!schedulerReady) return t.skip("scheduler not built");

    const response = await post({ processes: SAMPLE, algorithm: "SRTF" });
    assert.equal(response.status, 200);

    const result = (await response.json()) as SimulationResult;

    assert.equal(result.algorithm, "SRTF");
    assert.equal(result.totalTime, 13);
    assert.equal(result.busyTime, 13);
    assert.equal(result.switchTime, 0);
    assert.equal(result.processes.length, 4);

    // Hand-computed, and identical to what the command line prints.
    assert.equal(result.averages.waitingTime, 2.5);
    assert.equal(result.averages.responseTime, 1.25);
  });

  it("keeps the timeline continuous and accounted for", async (t) => {
    if (!schedulerReady) return t.skip("scheduler not built");

    const result = (await (await post({ processes: SAMPLE, algorithm: "RR" })).json()) as
      SimulationResult;

    assert.equal(result.timeline[0].start, 0);
    for (let i = 1; i < result.timeline.length; i++) {
      assert.equal(
        result.timeline[i - 1].end,
        result.timeline[i].start,
        "timeline slices must join up with no gap or overlap",
      );
    }
    assert.equal(result.timeline[result.timeline.length - 1].end, result.totalTime);
  });

  it("charges for context switches when asked", async (t) => {
    if (!schedulerReady) return t.skip("scheduler not built");

    const free = (await (await post({ processes: SAMPLE, algorithm: "RR", quantum: 1 })).json()) as
      SimulationResult;
    const costly = (await (
      await post({ processes: SAMPLE, algorithm: "RR", quantum: 1, switchCost: 2 })
    ).json()) as SimulationResult;

    assert.equal(free.switchTime, 0);
    assert.ok(costly.switchTime > 0, "switching should cost something");
    assert.equal(costly.busyTime, free.busyTime, "the work itself does not change");
    assert.ok(costly.totalTime > free.totalTime, "but the run takes longer");
    assert.ok(costly.timeline.some((slice) => slice.kind === "switch"));
  });

  it("returns one entry per algorithm when comparing", async (t) => {
    if (!schedulerReady) return t.skip("scheduler not built");

    const response = await post({ processes: SAMPLE, compare: ["FCFS", "SJF", "SRTF"] });
    const body = (await response.json()) as { runs: SimulationResult[] };

    assert.equal(body.runs.length, 3);
    assert.deepEqual(
      body.runs.map((run) => run.algorithm),
      ["FCFS", "SJF", "SRTF"],
    );

    // Same work, so every algorithm must take the same total time.
    const totals = new Set(body.runs.map((run) => run.totalTime));
    assert.equal(totals.size, 1, "scheduling changes who waits, not how much work there is");
  });

  it("agrees with the C++ program's own numbers", async (t) => {
    if (!schedulerReady) return t.skip("scheduler not built");

    const body = (await (
      await post({ processes: SAMPLE, compare: ["FCFS", "SJF", "SRTF", "RR"] })
    ).json()) as { runs: SimulationResult[] };

    const waiting = Object.fromEntries(
      body.runs.map((run) => [run.algorithm, run.averages.waitingTime]),
    );

    assert.equal(waiting["FCFS"], 3.75);
    assert.equal(waiting["SJF"], 3.25);
    assert.equal(waiting["SRTF"], 2.5);
    assert.equal(waiting["RR(q=2)"], 4.75);
  });
});
