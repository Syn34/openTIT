// Shared layout of the synthetic test ELF written by testgen.cpp.
#pragma once

#include <cstdint>

namespace testgen {

constexpr std::uint32_t kTextBase = 0x00010000;
constexpr std::uint32_t kDataBase = 0x00020000;
constexpr std::uint32_t kOpdBase = kDataBase;           // 8-byte descriptors
constexpr std::uint32_t kTocBase = kDataBase + 0x200;   // r2
constexpr std::uint32_t kScratch = kDataBase + 0x1000;  // test buffers
constexpr std::uint32_t kDataSize = 0x4000;

enum Fn : std::uint32_t {
    SumToN = 0,   // r3 = 1 + 2 + ... + r3 (ctr loop)
    Bits,         // r3 = x, r4 = out buffer: rotate/mask/shift/arith results
    MemOps,       // r3 = buffer: load/store widths, update forms; returns checksum
    CallChain,    // r3 = n: sum(n) + sum(n-1) through two nested calls
    Switch,       // r3 = k: jump table, 100/200/300/400 or -1
    Fp,           // f1, f2: returns (int)(f1 * f2 + f1) and single-precision path
    Vec,          // r3 = in (16B aligned), r4 = out
    Add128,       // (r3:r4) + (r5:r6) -> r3:r4, plus 128-bit subtract into r5:r6
    CrLogic,      // r3, r4: returns mfcr after compares and CR logic
    Syscall,      // returns sys_time_get_timebase_frequency via `sc`
    TailCall,     // r3 = n: tail-calls SumToN(n + 1)
    AtomicInc,    // r3 = word address: lwarx/stwcx. increment, returns new value
    Indirect,     // r3 = n: calls SumToN through its descriptor with bctrl
    Count
};

constexpr std::uint32_t opd(Fn f) { return kOpdBase + 8 * static_cast<std::uint32_t>(f); }

}  // namespace testgen
