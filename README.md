# openTIT

A static recompilation of **Sly Cooper: Thieves in Time** (PS3, Sanzaru Games, 2013) into native PC code, in C++.

The game's PowerPC (Cell PPU) executable is translated, function by function, into C++. That C++ is compiled with a normal PC compiler and linked against a runtime that stands in for the PS3 operating system and libraries. No emulator interprets or JITs the game's CPU code at run time.

> **This repository contains no game code or data.** You need your own disc dump. The recompiler reads *your* decrypted `EBOOT.elf` and writes the generated C++ to a local folder. That output is derived from the game, so never commit or share it (`.gitignore` blocks it).

## Status

Early, but the whole pipeline runs end to end:

| Stage | State |
|---|---|
| ELF loader: segments, TLS, process param, OPD scan, PRX import tables | ✅ |
| Import identification (NID → name, SHA-1 verified) | ✅ 250 / 251 imports named; all 187 HLE imports implemented |
| PPU decoder: full PowerPC 64 + VMX + Cell `lvlx`/`stvlx` family | ✅ 0 undecodable reachable words |
| Function discovery (recursive descent, tail calls, jump tables) | ✅ 24,164 functions, 178 switch tables, no unresolved `bctr` |
| C++ emitter (integer, FPU, VMX, CR, reservations, syscalls) | ✅ every reachable instruction emitted, no fallbacks |
| Full EBOOT → C++ | ✅ 2.77 M instructions → 223 files in ~3 s |
| Runtime: memory, dispatcher, threads, LV2 syscalls, HLE modules | 🟡 boot-path subset (see below) |
| Tests (recompiled PPC program, SPU, LV2, RSX, Vulkan) | ✅ 59 tests / 545 checks |
| Native boot of the recompiled game | 🟡 runs `main`, GCM + 720p, SPURS, FIOS, and opens the data files |
| RSX graphics | 🟡 Vulkan renderer: shader translation, textures, depth/stencil, blending, render-to-texture via guest memory. Not yet tried on game content |
| SPU / SPURS (jobs, tasks) | 🟡 SPU code recompiled to C++ (~1.3 G instructions/s), interpreter fallback; real `libsre` running LLE |
| Audio / input | 🟡 cellAudio ports mixed to the host device (SDL2); DualShock 3 from any SDL controller or the keyboard |

Numbers above come from the European disc release (`BCES01284`, v01.02). Other regions should work the same way. Run `opentit-recomp funcs` on your EBOOT to see its numbers.

### Where boot gets to today

The whole game compiles to one native executable, plus Sony's SPURS library (`libsre.prx`) recompiled the same way. Boot runs the game's CRT startup, static constructors and `main`, passes `cellGameBootCheck`/`ContentPermit`, and initialises GCM with the null RSX at 1280x720. It then starts SPURS: the four kernel SPUs (`ApeCoreCellSpursKernel1-4`) run on the SPU interpreter. FIOS I/O threads start, the system cache mounts, and the game opens `/dev_bdvd/PS3_GAME/USRDIR/Sly4.psarc`. That is as far as the development box gets, because it doesn't hold the 16 GB psarc. Boot logs from a full dump are the next step.

See [docs/ROADMAP.md](docs/ROADMAP.md) for what's next and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how it works.

## Quick start

You need CMake ≥ 3.20, a C++20 compiler (MSVC 2022, clang 15+, or GCC 12+), Ninja (recommended), and for graphics the [Vulkan SDK](https://vulkan.lunarg.com) (Vulkan + glslang; on Linux the distro packages `libvulkan-dev glslang-dev` work). SDL2 provides the window; if it isn't installed, CMake downloads it.

### 1. Dump and decrypt

Dump your disc, then decrypt `PS3_GAME/USRDIR/EBOOT.BIN` with RPCS3 (*Utilities → Decrypt PS3 Binaries*). Put the result next to it as `EBOOT.elf`:

```
<dump>/PS3_DISC.SFB
<dump>/PS3_GAME/PARAM.SFO
<dump>/PS3_GAME/USRDIR/EBOOT.elf     <- decrypted
<dump>/PS3_GAME/USRDIR/Sly4.psarc
...
```

### 1b. (Recommended) firmware SPURS module

Sly 4 uses SPURS job chains, so openTIT runs Sony's real SPURS library, the same approach RPCS3 takes. In RPCS3, use *File → Install Firmware* (the `PS3_UPDATE/PS3UPDAT.PUP` from your dump works), then *Utilities → Decrypt PS3 Binaries* on `dev_flash/sys/external/libsre.sprx`, and save the result as `<dump>/firmware/libsre.prx`. The build scripts recompile anything in `firmware/` automatically. openTIT never ships keys or firmware.

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
./build-game/opentit <dump> --renderer null       # no GPU work (debugging)
./build-game/opentit <dump> --dump-shaders shaders  # write every shader the game uses
```

On Windows, `tools\build.ps1 -Dump "C:\path\to\dump"` does steps 2–4.

### 5. (Optional) make SPU code faster

The build recompiles the SPU programs it can find in the EBOOT and libsre. SPURS also loads jobs at run time, and those run on the SPU interpreter until they are recorded:

```sh
./build-game/opentit <dump> --spu-record <dump>/sly4.spurec   # play for a while
tools/build.sh <dump>                                          # picks up sly4.spurec
```

### Controls

Any controller SDL recognises (Xbox, DualShock 4, DualSense...) works as a DualShock 3. On the keyboard:

| Keys | Pad |
|---|---|
| W A S D | left stick |
| arrow keys | right stick (camera) |
| I J K L | d-pad |
| Space / F / C / R | Cross / Square / Circle / Triangle |
| Q / E | L1 / R1 |
| Left Shift / Left Ctrl | L2 / R2 |
| Z / X | L3 / R3 |
| Enter / Backspace | Start / Select |

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
