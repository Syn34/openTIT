// SPU code discovery shared by the recompiler and the runtime recorder.
//
// A region is the code reachable from an entry point through static
// branches, stopping at indirect branches and leaving calls to their own
// regions. It is identified by its entry and a hash of its instruction words,
// because SPU code is DMA'd into local store at run time and the same
// address can hold different programs.
#pragma once

#include <set>
#include <string>
#include <vector>

#include "common/types.h"

namespace opentit::spu {

struct CodeRange {
    u32 start = 0;            // LS byte address
    std::vector<u32> words;   // instruction words (host values of the BE words)
};

struct Region {
    u32 entry = 0;
    std::vector<CodeRange> ranges;  // sorted, non-overlapping
    std::set<u32> entries;          // addresses the region can be entered at (entry, branch targets, call returns)
    std::set<u32> calls;            // static call targets (their own regions)
    u64 hash = 0;
    u32 instruction_count() const;
};

// Discover from `entry` in a local-store image (`ls` is 256 KiB, big endian).
Region discover(const u8* ls, u32 entry, u32 max_instructions = 8192);

// FNV-1a over the words of every range, in address order.
u64 region_hash(const std::vector<CodeRange>& ranges);
u64 region_hash_ls(const u8* ls, const u32* ranges, u32 nranges);  // ranges as (start, count) pairs

// Region record files (opentit --spu-record): a list of regions.
bool write_records(const std::string& path, const std::vector<Region>& regions);
bool read_records(const std::string& path, std::vector<Region>* out, std::string* error);

}  // namespace opentit::spu
