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
- [x] Resolve the two remaining dynamic `bctr` (0x009BD2CC, 0x00AB3ECC): self-relative tables placed after the `bctr`
- [x] `setjmp`/`longjmp`: not used by this EBOOT (no exceptions or longjmp found)
- [x] `sys_timer_*` (70–76): periodic events to event queues
- [x] VBlank handler invoked from a 60 Hz host thread
- [x] Save data (`cellSaveData*2` auto/list/fixed callback flows) backed by `dev_hdd0/home/00000001/savedata/<dir>/` with a real PARAM.SFO
- [x] Every EBOOT import implemented (offline answers for PSN, TUS, sockets, HTTP/SSL)

## M2: SPU / SPURS

Sly 4 imports 61 `cellSpurs` functions (job chains, tasksets, event flags, LF queues), so most of the engine's heavy lifting runs on SPUs.

- [x] SPU decoder, disassembler and interpreter (full ISA, channels, MFC DMA, lock-line atomics)
- [x] Embedded SPU image discovery (`opentit-recomp spu-list`): 10 images incl. EDGE zlib and EDGE MLAA
- [x] LV2 SPU syscalls: images, thread groups, LS/mailbox/SNR access, SPU events, receive_event
- [x] PRX loader + recompile path for firmware modules
- [x] Run the real SPURS (firmware `libsre.prx`, user-supplied and decrypted) on the SPU core. libsre is recompiled to C++ like the EBOOT and linked LLE (RPCS3 also uses LLE libsre for this game). The four SPURS kernel SPUs boot and service the workload
- [x] SPU static recompiler (SPU -> C++): regions keyed by entry + code hash, validated against local store by page write counters; embedded images compiled at build time, runtime code recorded with `--spu-record`
- [x] SSE `shufb` (x86-64-v2 baseline)
- [ ] Cache SPU registers in host locals inside a region; SSE paths for the float and quadword-shift ops

## M3: Graphics

- [x] RSX method decoder (NV4097) and state tracker. Draws, program/constant uploads, reports, and NV0039/NV3089/NV308A copies run through `runtime/src/rsx`
- [x] Vertex and fragment program decoders and disassemblers (`--dump-shaders <dir>` writes every program the game uses)
- [x] Vertex/fragment program translation (RSX microcode → GLSL 450 → SPIR-V via glslang), covering every opcode, conditions, and VP branches/calls. Also FP IFE/LOOP/REP/BRK/RET
- [x] Texture formats (all RSX formats incl. DXT, swizzled/linear, cubemaps, 3D, remap), render targets, and flips shown in an SDL2 window
- [x] Vulkan backend (dynamic rendering): pipelines from RSX state, depth/stencil, blending, MRT, scissor, viewport, Z16/Z24S8; end-to-end tests on lavapipe
- [x] Render targets stay on the GPU; textures that match a target sample it directly (A8R8G8B8/X8R8G8B8, FP16/FP32, B8, with channel remap); write-back only at copies, overlapping texture reads and flips
- [x] Depth textures from depth targets: shadow-compare samplers, depth values, Z24S8 bits read as xRGB8
- [ ] Tiled surfaces (cellGcmSetTileInfo) and MSAA surfaces
- [ ] Native float/depth texture formats instead of RGBA8 decode
- [x] Partial-channel clears (masked full-screen draw)
- [ ] Occlusion queries (reports are faked as visible)

## M4: Platform

- [x] Input: SDL game controller → `cellPad` (buttons, sticks, pressure), plus a keyboard mapping
- [x] Audio: cellAudio ports (2/8 channels) mixed to stereo and played through SDL2
- [ ] ATRAC3 (`.at3`) / MP3 decoding if the game hands compressed streams to libatrac/cellAdec (it decodes on SPU otherwise)
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
