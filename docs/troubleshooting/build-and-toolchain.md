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

**Options:**

1. **Turn Smart App Control off** - Windows Security → App & browser control →
   Smart App Control → Off. **This is permanent**; re-enabling it requires
   reinstalling Windows. Microsoft designed it that way.
2. **Build in a second directory.** A differently built binary often gets a
   different verdict:
   ```bash
   cmake -S . -B build-dbg -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build-dbg
   ```
   A workaround, not a fix - and it creates the stale-binary problem above.
3. **Build under WSL.** Linux binaries are not subject to it.

There is no way to sign your own builds around this.

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
