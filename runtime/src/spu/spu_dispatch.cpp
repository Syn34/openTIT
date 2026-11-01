// Finds recompiled SPU regions for the interpreter, validates them against
// local store, and records unknown hot code for the next build.
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include "../log.h"
#include "opentit/spu_recomp.h"
#include "spu/spu_analysis.h"
#include "spu_dispatch.h"

namespace opentit::spu {

namespace {

std::unordered_map<u32, std::vector<const CompiledRegion*>>& registry() {
    static std::unordered_map<u32, std::vector<const CompiledRegion*>> r;
    return r;
}
std::size_t g_region_count = 0;
std::atomic<u64> g_calls{0}, g_validations{0};

// ---- recorder
struct Recorder {
    std::mutex m;
    std::string path;
    std::vector<Region> regions;
    std::set<std::pair<u32, u64>> known;
    std::size_t written = 0;
};
Recorder& recorder() {
    static Recorder r;
    return r;
}
std::atomic<bool> g_recording{false};
constexpr u32 kRecordThreshold = 64;
constexpr std::size_t kRecordLimit = 20000;

// Sum of the write counters of every page a region's code lives on. Counters
// only grow, so any write to those pages changes the sum.
u32 stamp(const SPUThread& t, const CompiledRegion& r) {
    u32 s = 0;
    for (u32 k = 0; k < r.nranges; ++k) {
        const u32 start = r.ranges[2 * k], count = r.ranges[2 * k + 1];
        const u32 first = (start & kLsMask) >> 10, last = ((start + 4 * count - 1) & kLsMask) >> 10;
        for (u32 p = first;; p = (p + 1) & 255) {
            s += t.ls_gen[p];
            if (p == last) break;
        }
    }
    return s;
}

}  // namespace

struct SPUThread::RegionCache {
    enum State : u8 { Unknown = 0, None, Valid, Miss };
    struct Entry {
        const CompiledRegion* region = nullptr;
        const std::vector<const CompiledRegion*>* candidates = nullptr;
        u32 stamp = 0;
        State state = Unknown;
    };
    Entry e[0x10000];
    std::unordered_map<u32, u32> hits;  // recording only
};

void register_regions(const CompiledRegion* regions, std::size_t count) {
    for (std::size_t k = 0; k < count; ++k) {
        const CompiledRegion& r = regions[k];
        for (u32 i = 0; i < r.nentries; ++i) registry()[r.entries[i] & kLsMask].push_back(&r);
    }
    g_region_count += count;
}

std::size_t registered_region_count() { return g_region_count; }

DispatchStats dispatch_stats() {
    DispatchStats s;
    s.region_calls = g_calls.load();
    s.validations = g_validations.load();
    std::lock_guard<std::mutex> lk(recorder().m);
    s.recorded = recorder().regions.size();
    return s;
}

bool dispatch_enabled() { return g_region_count != 0 || g_recording.load(std::memory_order_relaxed); }

void release_region_cache(SPUThread& t) {
    delete t.region_cache;
    t.region_cache = nullptr;
}

static void record(SPUThread& t, u32 pc) {
    auto& rec = recorder();
    Region r = discover(t.ls, pc);
    if (r.instruction_count() < 4) return;
    std::lock_guard<std::mutex> lk(rec.m);
    if (rec.regions.size() >= kRecordLimit || !rec.known.insert({r.entry, r.hash}).second) return;
    rec.regions.push_back(std::move(r));
    if (rec.regions.size() - rec.written >= 64) {
        write_records(rec.path, rec.regions);
        rec.written = rec.regions.size();
    }
}

const CompiledRegion* find_region(SPUThread& t, u32 pc) {
    if (!t.region_cache) t.region_cache = new SPUThread::RegionCache();
    auto& c = *t.region_cache;
    auto& e = c.e[(pc & kLsMask) >> 2];
    if (e.state == SPUThread::RegionCache::Unknown) {
        auto it = registry().find(pc & kLsMask);
        if (it == registry().end()) {
            e.state = SPUThread::RegionCache::None;
        } else {
            e.candidates = &it->second;
            e.state = SPUThread::RegionCache::Miss;
            e.stamp = ~0u;
        }
    }
    if (e.state == SPUThread::RegionCache::None) {
        if (g_recording.load(std::memory_order_relaxed)) {
            u32& h = c.hits[pc];
            if (h < kRecordThreshold && ++h == kRecordThreshold) record(t, pc);
        }
        return nullptr;
    }
    u32 s = 0;
    for (const CompiledRegion* r : *e.candidates) s += stamp(t, *r);
    if (s == e.stamp) {
        if (e.region) g_calls.fetch_add(1, std::memory_order_relaxed);
        return e.region;
    }
    // Local store changed under the candidates: check which one (if any) is there now.
    g_validations.fetch_add(1, std::memory_order_relaxed);
    e.stamp = s;
    e.region = nullptr;
    e.state = SPUThread::RegionCache::Miss;
    for (const CompiledRegion* r : *e.candidates) {
        if (region_hash_ls(t.ls, r->ranges, r->nranges) == r->hash) {
            e.region = r;
            e.state = SPUThread::RegionCache::Valid;
            g_calls.fetch_add(1, std::memory_order_relaxed);
            break;
        }
    }
    return e.region;
}

void start_recording(const std::string& path) {
    auto& rec = recorder();
    std::lock_guard<std::mutex> lk(rec.m);
    rec.path = path;
    std::string err;
    std::vector<Region> old;
    if (read_records(path, &old, &err)) {
        for (auto& r : old)
            if (rec.known.insert({r.entry, r.hash}).second) rec.regions.push_back(std::move(r));
    }
    rec.written = rec.regions.size();
    g_recording = true;
    LOG_INFO("spu", "recording SPU code to %s (%zu regions so far)", path.c_str(), rec.regions.size());
}

void flush_recording() {
    if (!g_recording) return;
    auto& rec = recorder();
    std::lock_guard<std::mutex> lk(rec.m);
    if (rec.regions.size() != rec.written) {
        write_records(rec.path, rec.regions);
        rec.written = rec.regions.size();
    }
    LOG_INFO("spu", "SPU record: %zu regions in %s", rec.regions.size(), rec.path.c_str());
}

}  // namespace opentit::spu
