// ABI between statically recompiled SPU code and the runtime.
//
// Each region function runs from `pc` (one of its entries) until it leaves
// the region, stops, or the thread is killed, and returns the next pc.
// It is only used while local store still holds the exact words it was
// compiled from (checked by hash).
#pragma once

#include <cstddef>

#include "common/types.h"
#include "opentit/spu.h"

namespace opentit::spu {

using RegionFn = u32 (*)(SPUThread& t, u32 pc);

struct CompiledRegion {
    u32 entry;
    u64 hash;
    u32 nranges;
    const u32* ranges;   // (start, word count) pairs
    u32 nentries;
    const u32* entries;  // LS addresses it can be entered at
    u32 instructions;
    RegionFn fn;
};

void register_regions(const CompiledRegion* regions, std::size_t count);

struct RegionRegistrar {
    RegionRegistrar(const CompiledRegion* r, std::size_t n) { register_regions(r, n); }
};

// Recording of SPU code the build did not know about (opentit --spu-record).
void start_recording(const std::string& path);
void flush_recording();

struct DispatchStats {
    u64 region_calls = 0, validations = 0, recorded = 0;
};
DispatchStats dispatch_stats();
std::size_t registered_region_count();

}  // namespace opentit::spu
