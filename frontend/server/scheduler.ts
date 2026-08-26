import { spawn } from "node:child_process";
import { existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(here, "..", "..");

// Give up on a simulation after this long. A workload is normally solved in
// milliseconds, so anything approaching this means something has gone wrong.
const TIMEOUT_MS = 5000;

// Refuse absurd output rather than buffering it forever.
const MAX_OUTPUT_BYTES = 8 * 1024 * 1024;

export function findScheduler(): string | null {
  const override = process.env.SCHEDULER_BIN;
  if (override) {
    return existsSync(override) ? override : null;
  }

  const name = process.platform === "win32" ? "scheduler.exe" : "scheduler";
  const candidates = [
    path.join(repoRoot, "build", "bin", name),
    path.join(repoRoot, "build-dbg", "bin", name),
  ];
  return candidates.find((candidate) => existsSync(candidate)) ?? null;
}

export interface RunOutcome {
  stdout: string;
  stderr: string;
  code: number | null;
  timedOut: boolean;
}

// Run the scheduler with `args`, feeding it `input` on stdin.
//
// Every failure mode ends in a resolved promise rather than an exception: a
// missing binary, a crash, a timeout, or a flood of output. The caller decides
// what to tell the user - the server must not fall over because a child
// process misbehaved.
export function runScheduler(
  binary: string,
  args: string[],
  input: string,
): Promise<RunOutcome> {
  return new Promise((resolve) => {
    const child = spawn(binary, args, { stdio: ["pipe", "pipe", "pipe"] });

    let stdout = "";
    let stderr = "";
    let settled = false;
    let timedOut = false;

    const finish = (outcome: RunOutcome) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      resolve(outcome);
    };

    const timer = setTimeout(() => {
      timedOut = true;
      child.kill();
      finish({ stdout, stderr, code: null, timedOut: true });
    }, TIMEOUT_MS);

    child.stdout.on("data", (chunk: Buffer) => {
      stdout += chunk.toString();
      if (stdout.length > MAX_OUTPUT_BYTES) {
        child.kill();
        finish({ stdout: "", stderr: "scheduler produced too much output", code: null, timedOut: false });
      }
    });

    child.stderr.on("data", (chunk: Buffer) => {
      stderr += chunk.toString();
    });

    // Without this listener a missing or unrunnable binary takes the whole
    // server down with an unhandled 'error' event.
    child.on("error", (error: Error) => {
      finish({ stdout: "", stderr: error.message, code: null, timedOut: false });
    });

    child.on("close", (code) => {
      finish({ stdout, stderr, code, timedOut });
    });

    child.stdin.on("error", () => {
      // The child may exit before we finish writing; nothing useful to do.
    });
    child.stdin.write(input);
    child.stdin.end();
  });
}
