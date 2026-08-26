# Installing a compiler

C++ is a compiled language. Before any of this project will run you need a
program that turns `.cpp` files into an `.exe`. **Windows does not ship one.**

This trips people up because other languages hide it. Python code runs because
you installed Python; JavaScript runs because you installed Node. Both bring
their own runtime. C++ does not work that way, and a `.cpp` file on a fresh
Windows machine is just a text file.

Note also that **an editor is not a compiler.** VS Code has syntax highlighting,
autocomplete and a debugger interface, but it compiles nothing itself - it calls
whatever is already installed. Even Microsoft's own C/C++ extension tells you to
install a compiler separately. (Visual Studio, the large IDE, is a different
product and does bundle one.)

## Check what you have

```bash
g++ --version
```

If that prints a version number you are done. If it says "command not found",
carry on.

## Recommended: WinLibs

WinLibs is GCC packaged for Windows as a plain folder of executables - no
installer, no Unix emulation layer, no shell.

```bash
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
```

Then **open a new terminal** - PATH changes do not reach terminals that are
already open - and check again:

```bash
g++ --version
```

You should see GCC 16 or newer. That single package also brings `gdb` (the
debugger, which VS Code uses for breakpoints), `cmake`, `ninja` and
`mingw32-make`, so nothing else is needed.

### Is that command safe?

Worth asking of any command someone hands you. The reasoning:

- `winget` is Microsoft's own package manager, built into Windows 11.
- `-e --id` means an exact match on a package ID from the official winget
  repository, so it cannot resolve to something with a similar name.
- The download comes from the WinLibs project's own GitHub releases.
- winget verifies the file's SHA256 against the manifest and aborts on a
  mismatch.
- The installer type is `portable (zip)`: it unpacks a folder and adds it to
  PATH. No admin rights, no registry changes, no services.

One caveat worth knowing: Windows Defender sometimes flags freshly unpacked
compilers and linkers. That is a known false positive with MinGW toolchains.

## Alternative: MSYS2

MSYS2 works too, but it is a **package manager**, not a compiler - installing
MSYS2 alone gives you nothing to compile with. You then have to open the MSYS2
UCRT64 shell and install the toolchain into it:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb make
```

and add `C:\msys64\ucrt64\bin` to PATH yourself.

It is a reasonable choice if you expect to need C libraries later, since
`pacman` makes those easy. It is also more moving parts. During this project the
MSYS2 shell failed to start at all with:

```
C:\msys64\usr\bin\msys-2.0.dll is either not designed to run on Windows
or it contains an error.  Error status 0xc0e90002.
```

That file is MSYS2's Unix emulation layer, and when it fails to initialise
nothing in MSYS2 runs. WinLibs has no such layer, which is why it is the
recommendation here. See
[problems we hit](../troubleshooting/problems-we-hit.md) for the full story.

## Other options

- **Visual Studio Build Tools** - Microsoft's own compiler (MSVC). Heavier, but
  the best debugger integration if you use Visual Studio itself. The project
  builds with it; `CMakeLists.txt` already sets `/W4` for MSVC.
- **WSL** - build inside Linux instead. Sidesteps Windows-specific problems
  entirely, including Smart App Control, at the cost of moving your whole build
  environment.

## Next

[Building the project](building.md).
