#include "spu/spu_analysis.h"

#include <cstring>
#include <fstream>
#include <map>

#include "spu/spu_decoder.h"

namespace opentit::spu {

u32 Region::instruction_count() const {
    u32 n = 0;
    for (const auto& r : ranges) n += static_cast<u32>(r.words.size());
    return n;
}

namespace {
constexpr u32 kLsMask = 0x3FFFC;

u64 fnv_word(u64 h, u32 w) {
    for (int b = 3; b >= 0; --b) h = (h ^ ((w >> (8 * b)) & 0xFF)) * 0x100000001B3ull;
    return h;
}
}  // namespace

u64 region_hash(const std::vector<CodeRange>& ranges) {
    u64 h = 0xCBF29CE484222325ull;
    for (const auto& r : ranges) {
        h = fnv_word(h, r.start);
        for (u32 w : r.words) h = fnv_word(h, w);
    }
    return h;
}

u64 region_hash_ls(const u8* ls, const u32* ranges, u32 nranges) {
    u64 h = 0xCBF29CE484222325ull;
    for (u32 k = 0; k < nranges; ++k) {
        const u32 start = ranges[2 * k], count = ranges[2 * k + 1];
        h = fnv_word(h, start);
        for (u32 i = 0; i < count; ++i) h = fnv_word(h, load_be<u32>(ls + ((start + 4 * i) & kLsMask)));
    }
    return h;
}

Region discover(const u8* ls, u32 entry, u32 max_instructions) {
    Region r;
    r.entry = entry & kLsMask;
    std::set<u32> seen;
    std::vector<u32> work{r.entry};
    r.entries.insert(r.entry);
    while (!work.empty() && seen.size() < max_instructions) {
        u32 pc = work.back();
        work.pop_back();
        while (pc < 0x40000 && !seen.count(pc) && seen.size() < max_instructions) {
            seen.insert(pc);
            const u32 raw = load_be<u32>(ls + pc);
            const Op op = decode(raw);
            const Insn in(raw);
            const u32 next = (pc + 4) & kLsMask;
            const u32 rel = (pc + (static_cast<u32>(in.i16()) << 2)) & kLsMask;
            const u32 abs = (static_cast<u32>(in.i16()) << 2) & kLsMask;
            bool fallthrough = true;
            switch (op) {
            case Op::INVALID:
                fallthrough = false;
                break;
            case Op::BR: case Op::BRA:
                r.entries.insert(op == Op::BR ? rel : abs);
                work.push_back(op == Op::BR ? rel : abs);
                fallthrough = false;
                break;
            case Op::BRZ: case Op::BRNZ: case Op::BRHZ: case Op::BRHNZ:
                r.entries.insert(rel);
                work.push_back(rel);
                break;
            case Op::BRSL: case Op::BRASL:
                r.calls.insert(op == Op::BRSL ? rel : abs);
                r.entries.insert(next);  // the callee returns here through the dispatcher
                break;
            case Op::BISL: case Op::BISLED:
                r.entries.insert(next);
                break;
            case Op::BI: case Op::IRET:
                fallthrough = false;
                break;
            case Op::STOP: case Op::STOPD:
                // A handled stop-and-signal resumes at the next word. A zero word
                // (stop 0) is padding/data: the code ends here.
                if (raw == 0) fallthrough = false;
                else r.entries.insert(next);
                break;
            default:
                break;  // BIZ & co. fall through when not taken; the taken path leaves the region
            }
            if (!fallthrough) break;
            pc = next;
        }
    }
    // Contiguous ranges
    CodeRange cur;
    bool open = false;
    for (u32 a : seen) {
        if (open && a == cur.start + 4 * static_cast<u32>(cur.words.size())) {
            cur.words.push_back(load_be<u32>(ls + a));
            continue;
        }
        if (open) r.ranges.push_back(std::move(cur));
        cur = CodeRange{};
        cur.start = a;
        cur.words.push_back(load_be<u32>(ls + a));
        open = true;
    }
    if (open) r.ranges.push_back(std::move(cur));
    // Only keep entries that are inside the region.
    for (auto it = r.entries.begin(); it != r.entries.end();) it = seen.count(*it) ? std::next(it) : r.entries.erase(it);
    r.hash = region_hash(r.ranges);
    return r;
}

// ---------------------------------------------------------------- record files
// "OTSPUREC" u32 version(1) u32 count; per region: u32 entry, u32 nentries, entries[], u32 nranges,
// per range: u32 start, u32 count, words[]. Little endian.
namespace {
void put(std::ofstream& f, u32 v) { f.write(reinterpret_cast<const char*>(&v), 4); }
bool get(std::ifstream& f, u32* v) { return static_cast<bool>(f.read(reinterpret_cast<char*>(v), 4)); }
}  // namespace

bool write_records(const std::string& path, const std::vector<Region>& regions) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write("OTSPUREC", 8);
    put(f, 1);
    put(f, static_cast<u32>(regions.size()));
    for (const auto& r : regions) {
        put(f, r.entry);
        put(f, static_cast<u32>(r.entries.size()));
        for (u32 e : r.entries) put(f, e);
        put(f, static_cast<u32>(r.ranges.size()));
        for (const auto& c : r.ranges) {
            put(f, c.start);
            put(f, static_cast<u32>(c.words.size()));
            for (u32 w : c.words) put(f, w);
        }
    }
    return static_cast<bool>(f);
}

bool read_records(const std::string& path, std::vector<Region>* out, std::string* error) {
    std::ifstream f(path, std::ios::binary);
    char magic[8];
    u32 version = 0, count = 0;
    if (!f || !f.read(magic, 8) || std::memcmp(magic, "OTSPUREC", 8) != 0 || !get(f, &version) || version != 1 ||
        !get(f, &count)) {
        if (error) *error = "not an openTIT SPU record file: " + path;
        return false;
    }
    for (u32 k = 0; k < count; ++k) {
        Region r;
        u32 ne = 0, nr = 0;
        if (!get(f, &r.entry) || !get(f, &ne) || ne > 0x10000) break;
        for (u32 e = 0; e < ne; ++e) {
            u32 v;
            if (!get(f, &v)) break;
            r.entries.insert(v);
        }
        if (!get(f, &nr) || nr > 0x10000) break;
        for (u32 c = 0; c < nr; ++c) {
            CodeRange cr;
            u32 n = 0;
            if (!get(f, &cr.start) || !get(f, &n) || n > 0x10000) break;
            cr.words.resize(n);
            if (n && !f.read(reinterpret_cast<char*>(cr.words.data()), 4 * static_cast<std::streamsize>(n))) break;
            r.ranges.push_back(std::move(cr));
        }
        r.hash = region_hash(r.ranges);
        out->push_back(std::move(r));
    }
    return true;
}

}  // namespace opentit::spu
