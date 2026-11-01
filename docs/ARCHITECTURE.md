# Architecture

```
 EBOOT.elf ──► loader ──► analysis ──► emitter ──► gen/ppu_*.cpp ─┐
 (yours)       (ELF,      (functions,   (C++ per     (local only)   │  C++ compiler
               imports,   jump tables)  function)                   ├──────────────► opentit(.exe)
               NIDs)                                                │
                                         runtime/ (memory, kernel, ─┘
                                         HLE libraries, dispatcher)
```

## 1. Loader (`src/loader`)

* `Elf` parses the decrypted PPU executable: PT_LOAD segments, `PT_TLS`, `PT_PROC_PARAM` (`sys_process_param_t`: SDK version, primary stack size, malloc page size) and `PT_PROC_PRX_PARAM`.
* **Imports.** `prx_param.libstub_start..end` is an array of stub records, one per imported library. Each record holds a NID table and an `fstub` table of the same length. `fstub[i]` is a 32-bit slot that initially points at a 32-byte trampoline in the `.sceStub.text` section. That trampoline loads the slot, then jumps through the function descriptor the slot points to.
* **NIDs.** A function's NID is the first 4 bytes, little-endian, of `SHA-1(name ‖ 6759659904250490566427499489741A)`. `nid_names.inc` lists known export names, which are hashed at startup. A name is attached only when its hash matches, so wrong guesses never mislabel a function. Sly 4 imports 251 functions from 20 libraries, and 250 are identified.
* **OPDs.** PS3 function pointers are 8-byte descriptors `{u32 code, u32 toc}`. `e_entry` points at the entry descriptor, and the section that contains it is `.opd`. Its 13,858 descriptors are the primary function seeds.

## 2. Decoder (`src/ppu`)

`ppu_ops.inc` is an X-macro list of every Cell PPU instruction (≈300). `decode()` is a tree of switches on the primary and extended opcodes (19, 30, 31, 58, 59, 62, 63, and 4 for VMX VA/VX/VC forms). The `Insn` accessors use IBM bit numbering (bit 0 = MSB), so they match the manuals one to one.

On the Sly 4 EBOOT, every reachable word decodes. The ~11.8K words that don't decode are all inline jump-table data or padding, which the analysis never walks into.

## 3. Analysis (`src/analysis`)

The analysis uses recursive descent, not a linear sweep. The text section contains inline jump tables (relative 32-bit offsets right after `bctr`), so sweeping linearly would decode them as garbage.

1. Seeds: all `.opd` code addresses, the entry point, every import trampoline, and optional user `--entries`.
2. Each seed is walked by following fallthrough, conditional and unconditional branches, and recovered jump-table targets. Every `bl` target becomes a new seed. Seeding repeats until no new functions appear.
3. Once the full start set is known, each function is walked again. A branch, or a fallthrough, that reaches another function's entry becomes a **tail call**. Code shared by two functions is simply emitted in both.
4. **Jump tables.** The compiler's idiom is:
   ```
   cmplwi crN, rI, N-1 ; bgt default
   lwz   rT, off(r2)            ; table address from the TOC
   rlwinm/rldic rX, rI, 2       ; index * 4
   lwzx  r0, rX, rT ; extsw r0, r0 ; add r0, r0, rT
   mtctr r0 ; bctr
   ```
   A small backward constant propagator handles `li`/`lis`/`addi`/`ori`/`lwz`/`ld` and TOC reads. It falls back to a function-wide single definition for loop-hoisted bases. The table size comes from the `cmplwi` bound, and is capped where code resumes for tables stored inline. This recovers 175 tables and 4,126 targets. Two `bctr`s stay dynamic and go through the dispatcher at run time.

## 4. Emitter (`src/recomp`)

Each guest function becomes:

```cpp
OPENTIT_FN(004FC7B4) {                 // void f_004FC7B4(PPUContext& ctx)
    u64* const r = ctx.gpr; f64* const f = ctx.fpr; v128* const v = ctx.vr; u8* const cr = ctx.cr;
    // 004FC7B4: mflr r0
    r[0] = ctx.lr;
    // 004FC7B8: std r0, 16(r1)
    mem::w64((u32)(r[1] + 16ull), r[0]);
    ...
}
```

* **Control flow.** Local branches become `goto`. A `bl` becomes a direct C++ call (`ctx.lr = pc+4; f_X(ctx);`). `blr` becomes `return`. A tail branch becomes `return f_X(ctx);`. `bctr` with a recovered table becomes a `switch` of `goto`s, and any other indirect transfer goes through `call_indirect()`.
* **Registers.** Registers live in `PPUContext`. The CR is stored as 32 bytes, one per bit, which makes CR logic and `bc` conditions trivial. Immediates and rotate masks are folded at recompile time.
* **Semantics** follow 64-bit mode (MSR.SF=1, which is how LV2 runs games): 64-bit carries, CR0 from 64-bit results, and 32-bit effective addresses.
* **VMX.** A `v128` holds the vector as one little-endian 128-bit value: host byte *i* is guest byte 15−*i*. Element-wise ops therefore need no swizzling, and only permutes, merges, packs, splats and loads/stores use the big-endian accessors. The helpers in `vmx.h` are plain loops that auto-vectorise and can later be swapped for SSE/NEON intrinsics without regenerating code.
* **FP** is IEEE double. Single-precision ops round through `float`, fused ops use `std::fma`, and the build disables FMA contraction (`-ffp-contract=off`, `/fp:precise`) so separate PPC `fmul`/`fadd` keep their own rounding.
* **Atomics.** `lwarx`/`stwcx.` record a reservation, and `stwcx.` succeeds only if a host CAS on the big-endian word still sees the reserved value.

Output is split into ~12K-instruction files plus `ppu_table.cpp`, which maps every guest entry point to its host function.

## 5. Runtime (`runtime/`)

* **Memory.** The whole 4 GiB guest space is reserved as one host region, so guest→host translation is a single add. Main memory, user heap (`sys_memory_allocate`), mmapper, stacks (`0xD0000000`), RSX local memory (`0xC0000000`) and the HLE area are committed on demand.
* **Dispatcher.** A flat table indexed by `(addr - lo) / 4` gives one load per indirect call.
* **HLE.** Every import slot is patched to point at a fake descriptor whose code address falls in a reserved range. The dispatcher recognises that range and calls the C++ implementation, which is registered by name (`OPENTIT_HLE(lib, name)`). Unimplemented imports log once and return `CELL_OK`, or abort with `--strict`.
* **Kernel.** Guest PPU threads run on host threads. Mutexes, condition variables, event flags, semaphores, event queues and ports, lwmutex and lwcond are built on host primitives. The syscall table covers every syscall number the EBOOT invokes directly. SPU and timer syscalls are logged as unimplemented.
* **VFS.** `/dev_bdvd` maps to your dump, `/app_home` to `PS3_GAME/USRDIR`, and `/dev_hdd0` and `/dev_hdd1` to local folders. Lookups are case-insensitive.
* **Null RSX.** `_cellGcmInitBody` sets up the context, control registers, labels and IO map. A host thread then consumes the FIFO: it follows jumps, calls and returns, applies reference, semaphore and back-end label writes, and acknowledges flips. The game's CPU/GPU synchronisation therefore completes. A renderer will replace the "discard draw" path.

## Testing

`tests/testgen.cpp` assembles a small PowerPC program into a well-formed PS3-style ELF: loops, rotates and masks, carries, every load/store width, nested calls, a compiler-style jump table, FP, VMX, CR logic, a syscall, tail and indirect calls, and atomics. CMake recompiles it with `opentit-recomp`. `opentit_tests` then loads it into guest memory, runs the generated C++, and checks every result against an independent host computation. The tests exercise the real pipeline without any game files.
