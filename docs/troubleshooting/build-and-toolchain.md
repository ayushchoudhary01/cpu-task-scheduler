# Troubleshooting: build and toolchain

## `g++: command not found` / `'g++' is not recognized`

There is no C++ compiler installed, or it is not on PATH.

Windows does not ship one. See
[installing a compiler](../getting-started/installing-a-compiler.md).

If you installed one and still see this, you are probably in a terminal that was
open *before* the install. PATH changes do not reach existing terminals - open a
new one.

## `CMake was unable to find a build program`

CMake found no generator. With WinLibs, `ninja` is included, so:

```bash
cmake -S . -B build -G Ninja
```

If Ninja is genuinely absent, use the Makefile generator instead - note the
executable is `mingw32-make`, not `make`:

```bash
cmake -S . -B build -G "MinGW Makefiles"
```

## `The CXX compiler identification is unknown`

CMake found something called a compiler but could not run it. Usually a broken
or half-installed toolchain. Check it works on its own:

```bash
g++ --version
```

Then delete `build/` and configure again - CMake caches the compiler it found,
so a stale cache survives fixing the underlying problem:

```bash
rm -rf build
cmake -S . -B build -G Ninja
```

## Changes are not taking effect

Two usual causes.

**You edited `CMakeLists.txt`** - re-run the configure step, not just the build:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

**You have two build directories.** If both `build/` and `build-dbg/` exist,
building one leaves the other stale. Whatever you run may not be what you just
compiled. Build both, or delete the one you are not using.

## `An Application Control policy has blocked this file`

Windows **Smart App Control** blocking a binary you just compiled. It blocks
unsigned executables without an established reputation, which describes every
program you build yourself.

Check whether it is on:

```powershell
(Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy').VerifiedAndReputablePolicyState
```

`1` means enforcing. For the details, look in Event Viewer under
**Microsoft-Windows-CodeIntegrity/Operational** - a block appears as event 3077
or 3118.

The verdict is per-binary and inconsistent: one executable runs while another,
compiled seconds later from the same project, does not.

It can also escalate. Once it starts blocking the *toolchain* - `cmake`, `ninja`
and `g++`'s internal `cc1plus` - you cannot build at all, and `g++` fails with
exit code 1 and no error message whatsoever.

**Options:**

1. **Turn Smart App Control off** - Windows Security → App & browser control →
   Smart App Control → Off. **This is permanent**; re-enabling it requires
   reinstalling Windows. Microsoft designed it that way.
2. **Build under WSL** (see below). Linux binaries are not subject to it, and
   this is reversible.

There is no way to sign your own builds around it, and Smart App Control has no
exclusion list - it is all or nothing by design.

## Building under WSL

The route taken on the machine this project was developed on. Windows keeps its
security settings; the C++ is built in Linux and reached through `wsl.exe`,
which is signed by Microsoft and so is never blocked.

```powershell
wsl --install -d Ubuntu
```

Then inside Ubuntu:

```bash
sudo apt update && sudo apt install -y build-essential cmake ninja-build
```

```bash
cmake -S /mnt/c/Projects/cpu-task-scheduler -B ~/scheduler-build -G Ninja
cmake --build ~/scheduler-build
~/scheduler-build/bin/scheduler_tests
```

**Build into `~`, not into the project directory.** CMake sets permissions on
the files it writes, and the Windows drive mounted at `/mnt/c` does not support
Linux permissions - every `configure_file` fails with *"Operation not
permitted"*. Building in the Linux filesystem avoids that entirely and is
several times faster, since file access no longer crosses between the two
systems. The source stays on `C:`; only the build output lives in Linux.

The web server finds this automatically - see
[running the web app](running-the-web-app.md#which-binary-is-the-server-using).

Two things worth knowing:

- **`wsl.exe` on its own runs the *default* distribution**, which on a machine
  with Docker Desktop installed is Docker's own helper image, not Ubuntu. The
  distribution always has to be named: `wsl -d Ubuntu`.
- **Docker Desktop's `docker-desktop` entry is not a usable distribution.** It
  is Alpine-based, managed by Docker, and reset whenever Docker is. Having it
  does not mean you have a Linux environment to work in.

## MSYS2: `msys-2.0.dll ... Error status 0xc0e90002`

MSYS2's Unix emulation layer failing to initialise. Nothing in MSYS2 runs when
this happens, including its own shell.

Usually caused by antivirus interference with the DLL's memory layout, or a
clash with another copy of `msys-2.0.dll` - Git for Windows ships one.

Rather than debugging it, use WinLibs, which has no emulation layer:

```bash
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
```

MSYS2 can be left installed; it is inert if you do not launch it.

## Tests fail after pulling changes

Run them and read the failure - each prints what it got and what it expected:

```
FAIL: P1 finishes at 8  (got 7, expected 8)
```

If it is an invariant failure from `RobustnessTests.cpp`, the message names the
algorithm and the seed:

```
FAIL: no invariant was broken  (got SRTF on seed 7: busy time does not match
the total burst time, expected )
```

That is reproducible - the generator is seeded, so the same seed gives the same
workload every time.

## Starting completely fresh

```bash
rm -rf build build-dbg
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/scheduler_tests.exe
```

Nothing you wrote lives in either directory.
