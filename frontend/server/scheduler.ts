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

// How to launch the scheduler. Usually just a path, but on Windows it may be a
// Linux build invoked through WSL, which needs a command plus some arguments in
// front of our own.
export interface SchedulerCommand {
  command: string;
  prefixArgs: string[];
  label: string;
}

// Where a WSL build is expected, and which distribution to use.
// `wsl.exe` on its own picks the *default* distribution, which on a machine
// with Docker Desktop installed is Docker's own helper image - so the
// distribution always has to be named explicitly.
const WSL_DISTRO = process.env.SCHEDULER_WSL_DISTRO ?? "Ubuntu";
const WSL_BUILD_PATH = process.env.SCHEDULER_WSL_PATH ?? "scheduler-build/bin/scheduler";

function nativeScheduler(): string | null {
  const name = process.platform === "win32" ? "scheduler.exe" : "scheduler";
  const candidates = [
    path.join(repoRoot, "build", "bin", name),
    path.join(repoRoot, "build-dbg", "bin", name),
  ];
  return candidates.find((candidate) => existsSync(candidate)) ?? null;
}

// Ask WSL whether a Linux build exists, and where. Returns its absolute path
// inside the distribution, or null.
//
// This runs `sh -c` rather than the binary directly so that `$HOME` is
// expanded by the Linux shell - the Windows side has no idea what the Linux
// home directory is called.
function findWslScheduler(): Promise<string | null> {
  return new Promise((resolve) => {
    if (process.platform !== "win32") {
      resolve(null);
      return;
    }

    const probe = `p="$HOME/${WSL_BUILD_PATH}"; [ -x "$p" ] && printf %s "$p"`;
    const child = spawn("wsl.exe", ["-d", WSL_DISTRO, "-e", "sh", "-c", probe]);

    let found = "";
    child.stdout.on("data", (chunk: Buffer) => {
      found += chunk.toString();
    });
    child.on("error", () => resolve(null));
    child.on("close", (code) => resolve(code === 0 && found.trim() ? found.trim() : null));
  });
}

// Work out how to run the scheduler, once, and remember the answer.
//
// On Windows a Linux build is preferred when one exists. That looks backwards
// until you have met Smart App Control, which blocks the unsigned GCC runtime
// DLLs a MinGW build depends on - the program is killed before reaching main()
// and reports status 0xC0E90002. A Linux build inside WSL is not subject to it,
// and `wsl.exe` itself is signed by Microsoft.
let cached: Promise<SchedulerCommand | null> | null = null;

export function findScheduler(): Promise<SchedulerCommand | null> {
  if (cached) {
    return cached;
  }

  cached = (async () => {
    const override = process.env.SCHEDULER_BIN;
    if (override) {
      return existsSync(override)
        ? { command: override, prefixArgs: [], label: `${override} (SCHEDULER_BIN)` }
        : null;
    }

    const wslPath = await findWslScheduler();
    if (wslPath) {
      return {
        command: "wsl.exe",
        prefixArgs: ["-d", WSL_DISTRO, "-e", wslPath],
        label: `${wslPath} (WSL: ${WSL_DISTRO})`,
      };
    }

    const native = nativeScheduler();
    return native ? { command: native, prefixArgs: [], label: native } : null;
  })();

  return cached;
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
  scheduler: SchedulerCommand,
  args: string[],
  input: string,
): Promise<RunOutcome> {
  return new Promise((resolve) => {
    const child = spawn(scheduler.command, [...scheduler.prefixArgs, ...args], {
      stdio: ["pipe", "pipe", "pipe"],
    });

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
