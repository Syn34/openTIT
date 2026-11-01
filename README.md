# openTIT

A static recompilation of **Sly Cooper: Thieves in Time** (PS3, Sanzaru Games, 2013) into native PC code, in C++.

The game's PowerPC (Cell PPU) executable is translated, function by function, into C++. That C++ is compiled with a normal PC compiler and linked against a runtime that stands in for the PS3 operating system and libraries. No emulator interprets or JITs the game's CPU code at run time.

> **This repository contains no game code or data.** You need your own disc dump. The recompiler reads *your* decrypted `EBOOT.elf` and writes the generated C++ to a local folder. That output is derived from the game, so never commit or share it (`.gitignore` blocks it).

## Status

Early, but the whole pipeline runs end to end:

| Stage | State |
|---|---|
| ELF loader: segments, TLS, process param, OPD scan, PRX import tables | ✅ |
| Import identification (NID → name, SHA-1 verified) | ✅ 250 / 251 imports named |
| PPU decoder: full PowerPC 64 + VMX + Cell `lvlx`/`stvlx` family | ✅ 0 undecodable reachable words |
| Function discovery (recursive descent, tail calls, jump tables) | ✅ 24,163 functions, 175 switch tables |
| C++ emitter (integer, FPU, VMX, CR, reservations, syscalls) | ✅ every reachable instruction emitted, no fallbacks |
| Full EBOOT → C++ | ✅ 2.77 M instructions → 223 files in ~3 s |
| Runtime: memory, dispatcher, threads, LV2 syscalls, HLE modules | 🟡 boot-path subset (see below) |
| End-to-end tests (synthetic PPC program, recompiled and run) | ✅ 25 tests / 234 checks |
| Native boot of the recompiled game | 🟡 runs CRT + static init + `main`, sets up GCM and 720p video, stops at SPURS init |
| RSX graphics | 🔴 null GPU: the FIFO is consumed, nothing is drawn |
| SPU / SPURS (jobs, tasks) | 🔴 not started (next big milestone) |
| Audio / input | 🔴 null audio device, one idle pad |

Numbers above come from the European disc release (`BCES01284`, v01.02). Other regions should work the same way. Run `opentit-recomp funcs` on your EBOOT to see its numbers.

### Where boot gets to today

The whole game compiles to one ~56 MB native executable. It runs the game's own CRT startup and static constructors, then `main` and its command-line parsing. It prints the engine banner (`Versions:SNC:41001`), passes `cellGameBootCheck`/`ContentPermit`, initialises GCM with the null RSX, configures 1280x720, and registers both display buffers. It stops at an assertion in Sony's EDGE post-processing (`edgepost_mlaa_handler.cpp`) during SPURS task setup, which is the expected wall until SPU/SPURS support lands.

See [docs/ROADMAP.md](docs/ROADMAP.md) for what's next and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how it works.

## Quick start

You need CMake ≥ 3.20, a C++20 compiler (MSVC 2022, clang 15+, or GCC 12+), and Ninja (recommended).

### 1. Dump and decrypt

Dump your disc, then decrypt `PS3_GAME/USRDIR/EBOOT.BIN` with RPCS3 (*Utilities → Decrypt PS3 Binaries*). Put the result next to it as `EBOOT.elf`:

```
<dump>/PS3_DISC.SFB
<dump>/PS3_GAME/PARAM.SFO
<dump>/PS3_GAME/USRDIR/EBOOT.elf     <- decrypted
<dump>/PS3_GAME/USRDIR/Sly4.psarc
...
```

### 2. Build the tools

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/opentit_tests        # optional: end-to-end recompiler tests
```

### 3. Inspect and recompile

```sh
./build/opentit-recomp info      <dump>/PS3_GAME/USRDIR/EBOOT.elf
./build/opentit-recomp imports   <dump>/PS3_GAME/USRDIR/EBOOT.elf
./build/opentit-recomp funcs     <dump>/PS3_GAME/USRDIR/EBOOT.elf
./build/opentit-recomp disasm    <dump>/PS3_GAME/USRDIR/EBOOT.elf 4FC7B4 40
./build/opentit-recomp recompile <dump>/PS3_GAME/USRDIR/EBOOT.elf -o gen
```

### 4. Build and run the game

```sh
cmake -S . -B build-game -G Ninja -DOPENTIT_GENERATED_DIR=$PWD/gen -DOPENTIT_BUILD_TESTS=OFF
cmake --build build-game
./build-game/opentit <dump> --log info
./build-game/opentit <dump> --log info -- -T      # pass switches to the game's own argv parser
```

On Windows, `tools\build.ps1 -Dump "C:\path\to\dump"` does steps 2–4.

## Layout

```
src/common     types, endian helpers, SHA-1
src/loader     ELF / PRX import tables / NID names / PARAM.SFO
src/ppu        decoder (ppu_ops.inc is the full opcode list) and disassembler
src/analysis   function discovery, jump-table recovery
src/recomp     C++ emitter and the opentit-recomp CLI
runtime/       what recompiled code links against
  include/opentit/  recomp.h (ABI for generated code), vmx.h, memory.h, ppu_context.h
  src/              dispatcher, LV2 kernel + syscalls, threads, VFS
  src/hle/          system library implementations (sysPrxForUser, sys_fs, GCM, sysutil...)
tests/         synthetic PPC test program + unit tests
docs/          architecture, roadmap
```

## Legal

openTIT is an independent project. It is not affiliated with or endorsed by Sony Interactive Entertainment or Sanzaru Games. *Sly Cooper* is a trademark of Sony Interactive Entertainment. You must own the game to use this project, and the repository will never host game code, assets, or recompiled output.
