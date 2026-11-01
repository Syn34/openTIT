# Roadmap

Rough order. Each milestone should leave the tests green and the game booting at least as far as before.

## M1: PPU boot to the first frame (in progress)

- [x] Recompile 100% of reachable PPU code
- [x] HLE for the boot path: TLS, threads, lwmutex/lwcond, sys_fs, sysutil, cellGame, GCM init
- [x] Boot through CRT, static constructors and `main` to GCM/video setup
- [ ] Boot-log triage: run `opentit <dump> --log debug`, implement HLE functions in the order the game hits them
- [x] Past the EDGE MLAA SPURS assertion (fixed by running the real libsre)
- [ ] Current stop (cloud test box only): `Sly4.psarc` missing there, so asset init fails. Next boot logs need a full dump on a real PC
- [ ] `--entries` workflow for any `indirect branch ... has no recompiled function` reports
- [ ] Resolve the two remaining dynamic `bctr` (0x009BD2CC, 0x00AB3ECC)
- [ ] `setjmp`/`longjmp` and any other code that manipulates LR or r1 non-locally
- [x] `sys_timer_*` (70–76): periodic events to event queues
- [x] VBlank handler invoked from a 60 Hz host thread
- [ ] Save data (`cellSaveData*` callback flow) backed by `dev_hdd0/home/.../savedata`

## M2: SPU / SPURS

Sly 4 imports 61 `cellSpurs` functions (job chains, tasksets, event flags, LF queues), so most of the engine's heavy lifting runs on SPUs.

- [x] SPU decoder, disassembler and interpreter (full ISA, channels, MFC DMA, lock-line atomics)
- [x] Embedded SPU image discovery (`opentit-recomp spu-list`): 10 images incl. EDGE zlib and EDGE MLAA
- [x] LV2 SPU syscalls: images, thread groups, LS/mailbox/SNR access, SPU events, receive_event
- [x] PRX loader + recompile path for firmware modules
- [x] Run the real SPURS (firmware `libsre.prx`, user-supplied and decrypted) on the SPU core. libsre is recompiled to C++ like the EBOOT and linked LLE (RPCS3 also uses LLE libsre for this game). The four SPURS kernel SPUs boot and service the workload
- [ ] SPU static recompiler (SPU -> C++) for the embedded images and the SPURS kernel (performance)

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
