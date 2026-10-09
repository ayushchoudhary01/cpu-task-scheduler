import assert from "node:assert/strict";
import { describe, it } from "node:test";

import { buildInvocation } from "../server/app.js";
import { validateRequest } from "../server/validate.js";

// A request that should always be accepted, so each test can change one thing.
function goodRequest(overrides: Record<string, unknown> = {}) {
  return {
    processes: [
      { id: "P1", arrivalTime: 0, burstTime: 5, priority: 3 },
      { id: "P2", arrivalTime: 1, burstTime: 3, priority: 1 },
    ],
    algorithm: "SRTF",
    ...overrides,
  };
}

// Checks the request is rejected and that the reason mentions `fragment`,
// because an error nobody can act on is barely better than no error.
function rejects(body: unknown, fragment: string) {
  const { request, errors } = validateRequest(body);
  assert.equal(request, null, `expected a rejection mentioning "${fragment}"`);
  assert.ok(
    errors.some((error) => error.toLowerCase().includes(fragment.toLowerCase())),
    `no error mentioned "${fragment}". Got: ${errors.join(" | ")}`,
  );
}

describe("validateRequest", () => {
  it("accepts a sensible request and fills in the defaults", () => {
    const { request, errors } = validateRequest(goodRequest());

    assert.deepEqual(errors, []);
    assert.ok(request);
    assert.equal(request.processes.length, 2);
    assert.equal(request.quantum, 2);
    assert.equal(request.agingRate, 0);
    assert.equal(request.switchCost, 0);
  });

  it("defaults a missing priority to zero", () => {
    const { request } = validateRequest({
      processes: [{ id: "P1", arrivalTime: 0, burstTime: 4 }],
    });

    assert.equal(request?.processes[0].priority, 0);
  });

  it("rejects anything that is not an object", () => {
    rejects(null, "object");
    rejects("P1 0 5", "object");
  });

  it("rejects an empty or missing process list", () => {
    rejects({ processes: [] }, "at least one");
    rejects({}, "array");
  });

  it("rejects process ids that would break the text format", () => {
    // Ids become whitespace separated fields, so a space would become two.
    rejects(goodRequest({ processes: [{ id: "bad name", arrivalTime: 0, burstTime: 1 }] }), "id");
    rejects(goodRequest({ processes: [{ id: "", arrivalTime: 0, burstTime: 1 }] }), "id");
    rejects(
      goodRequest({ processes: [{ id: "x".repeat(20), arrivalTime: 0, burstTime: 1 }] }),
      "id",
    );
  });

  it("rejects duplicate ids", () => {
    rejects(
      goodRequest({
        processes: [
          { id: "P1", arrivalTime: 0, burstTime: 1 },
          { id: "P1", arrivalTime: 1, burstTime: 1 },
        ],
      }),
      "duplicate",
    );
  });

  it("rejects impossible timings", () => {
    rejects(goodRequest({ processes: [{ id: "P1", arrivalTime: -1, burstTime: 5 }] }), "arrival");
    rejects(goodRequest({ processes: [{ id: "P1", arrivalTime: 0, burstTime: 0 }] }), "burst");
    rejects(goodRequest({ processes: [{ id: "P1", arrivalTime: 0, burstTime: 1.5 }] }), "burst");
  });

  it("rejects a workload too large to draw", () => {
    // Every value here is individually legal; the total is not. This is the
    // case that used to time out after five seconds instead of being refused.
    const processes = Array.from({ length: 50 }, (_unused, i) => ({
      id: `P${i + 1}`,
      arrivalTime: 0,
      burstTime: 1000,
      priority: 0,
    }));

    rejects({ processes, algorithm: "RR", quantum: 1 }, "too large");
  });

  it("accepts a workload right at the size limit", () => {
    const processes = Array.from({ length: 20 }, (_unused, i) => ({
      id: `P${i + 1}`,
      arrivalTime: 0,
      burstTime: 1000,
      priority: 0,
    }));

    const { request, errors } = validateRequest({ processes, algorithm: "RR" });
    assert.ok(request, `expected acceptance, got: ${errors.join(" | ")}`);
  });

  it("rejects unknown algorithms rather than guessing", () => {
    rejects(goodRequest({ algorithm: "MLFQ" }), "unknown algorithm");
  });

  it("matches algorithm names regardless of case", () => {
    const { request } = validateRequest(goodRequest({ algorithm: "srtf" }));
    assert.ok(request);
  });

  it("rejects settings outside their range", () => {
    rejects(goodRequest({ quantum: 0 }), "quantum");
    rejects(goodRequest({ agingRate: -1 }), "aging");
    rejects(goodRequest({ switchCost: -1 }), "switch");
    rejects(goodRequest({ switchCost: 1000 }), "switch");
  });

  it("requires at least two algorithms to compare", () => {
    rejects(goodRequest({ compare: ["FCFS"] }), "at least two");
    rejects(goodRequest({ compare: ["FCFS", "NOPE"] }), "unknown algorithm");

    const { request } = validateRequest(goodRequest({ compare: ["FCFS", "SJF"] }));
    assert.deepEqual(request?.compare, ["FCFS", "SJF"]);
  });

  it("reports every problem at once, not just the first", () => {
    const { errors } = validateRequest({
      processes: [{ id: "P1", arrivalTime: -1, burstTime: 5 }],
      algorithm: "NOPE",
      quantum: 0,
    });

    assert.ok(errors.length >= 3, `expected several errors, got: ${errors.join(" | ")}`);
  });
});

describe("buildInvocation", () => {
  it("passes every setting to the program", () => {
    const { args, input } = buildInvocation({
      processes: [{ id: "P1", arrivalTime: 0, burstTime: 5, priority: 3 }],
      algorithm: "RR",
      quantum: 4,
      agingRate: 2,
      switchCost: 1,
    });

    assert.deepEqual(args, [
      "--format", "json",
      "--algorithm", "RR",
      "--quantum", "4",
      "--aging", "2",
      "--switch-cost", "1",
    ]);
    assert.equal(input, "P1 0 5 3");
  });

  it("uses --compare instead of --algorithm when comparing", () => {
    const { args } = buildInvocation({
      processes: [{ id: "P1", arrivalTime: 0, burstTime: 5, priority: 0 }],
      compare: ["FCFS", "SJF"],
    });

    assert.ok(args.includes("--compare"));
    assert.ok(args.includes("FCFS,SJF"));
    assert.ok(!args.includes("--algorithm"));
  });

  it("writes one line per process in the order given", () => {
    const { input } = buildInvocation({
      processes: [
        { id: "A", arrivalTime: 0, burstTime: 2, priority: 1 },
        { id: "B", arrivalTime: 3, burstTime: 4, priority: 2 },
      ],
    });

    assert.equal(input, "A 0 2 1\nB 3 4 2");
  });
});
