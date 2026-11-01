# Roadmap

Rough order. Each milestone should leave the tests green and the game booting at least as far as before.

## M1: PPU boot to the first frame (in progress)

- [x] Recompile 100% of reachable PPU code
- [x] HLE for the boot path: TLS, threads, lwmutex/lwcond, sys_fs, sysutil, cellGame, GCM init
- [ ] Boot-log triage: run `opentit <dump> --log debug`, implement HLE functions in the order the game hits them
- [ ] `--entries` workflow for any `indirect branch ... has no recompiled function` reports
- [ ] Resolve the two remaining dynamic `bctr` (0x009BD2CC, 0x00AB3ECC)
- [ ] `setjmp`/`longjmp` and any other code that manipulates LR or r1 non-locally
- [ ] `sys_timer_*` (70–76): periodic events to event queues
- [ ] VBlank and flip handlers invoked from a 60 Hz host thread
- [ ] Save data (`cellSaveData*` callback flow) backed by `dev_hdd0/home/.../savedata`

## M2: SPU / SPURS

Sly 4 imports 61 `cellSpurs` functions (job chains, tasksets, event flags, LF queues), so most of the engine's heavy lifting runs on SPUs.

- [ ] SPU ELF extraction: find embedded SPU images in the PPU binary and in `Sly4.psarc`
- [ ] SPU decoder + static recompiler (same approach: SPU → C++), including channel and MFC DMA ops
- [ ] SPURS kernel emulation: workloads, tasksets, job chains, built on the recompiled SPU code
- [ ] SPU thread groups (`sys_spu_thread_group_*` syscalls)

## M3: Graphics

- [ ] RSX method decoder (NV4097) and state tracker on top of the null-RSX FIFO consumer
- [ ] Vertex/fragment program translation (RSX microcode → SPIR-V / HLSL)
- [ ] Texture formats, tiling and swizzling, render targets, and `GCM_FLIP` to a swapchain
- [ ] Backend: Vulkan first (Windows + Linux), D3D12 optional

## M4: Platform

- [ ] Input: XInput / SDL gamepad → `cellPad`, plus a keyboard mapping
- [ ] Audio: cellAudio ports → WASAPI/SDL, and an ATRAC3 (`.at3`) decoder for streams
- [ ] Bink (`.bik`) movies
- [ ] `Sly4.psarc` reader (to load assets directly and to support mods)

## Performance (any time)

- [ ] Cache GPRs in locals within a function and flush them at calls/returns (big win for the compiler)
- [ ] SSE/NEON intrinsics in `vmx.h`
- [ ] Direct calls into HLE functions, bypassing the import trampoline
- [ ] Guard-page or fastmem tricks for `dcbz` and large memcpy paths

## Nice to have

- [ ] Symbol and annotation database: name functions as they're understood, and re-emit with names
- [ ] Widescreen / FPS / resolution patches as recompile-time hooks
