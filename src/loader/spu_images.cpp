#include "loader/spu_images.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace opentit::loader {

namespace {

u32 file_to_va(const Elf& elf, u32 off) {
    for (const auto& s : elf.segments())
        if (s.type == PT_LOAD && off >= s.offset && off < s.offset + s.filesz) return static_cast<u32>(s.vaddr + (off - s.offset));
    return 0;
}

std::string find_hint(const u8* p, std::size_t n) {
    // Prefer a source-file-like string (".cpp"/".c"), else the longest printable run.
    std::string best, fallback;
    std::string cur;
    for (std::size_t i = 0; i < n; ++i) {
        const char c = static_cast<char>(p[i]);
        if (std::isprint(static_cast<unsigned char>(c))) {
            cur.push_back(c);
            continue;
        }
        if (cur.size() >= 8) {
            if (best.empty() && (cur.find(".cpp") != std::string::npos || cur.find(".c") + 2 == cur.size()))
                best = cur;
            if (cur.size() > fallback.size() && cur.find("GCC") == std::string::npos) fallback = cur;
        }
        cur.clear();
    }
    std::string h = best.empty() ? fallback : best;
    if (h.size() > 60) h = h.substr(0, 60);
    return h;
}

}  // namespace

std::vector<SpuImage> find_spu_images(const Elf& elf) {
    std::vector<SpuImage> out;
    const auto& d = elf.data();
    const u8* base = d.data();
    for (std::size_t i = 64; i + 52 <= d.size(); i += 4) {
        if (std::memcmp(base + i, "\x7f" "ELF", 4) != 0) continue;
        const u8* e = base + i;
        if (e[4] != 1 || e[5] != 2 || load_be<u16>(e + 18) != 23) continue;
        SpuImage img;
        img.file_offset = static_cast<u32>(i);
        img.vaddr = file_to_va(elf, img.file_offset);
        img.entry = load_be<u32>(e + 24);
        const u32 phoff = load_be<u32>(e + 28);
        const u16 phnum = load_be<u16>(e + 44);
        const u32 shoff = load_be<u32>(e + 32);
        const u16 shnum = load_be<u16>(e + 48);
        u32 end = 52;
        bool ok = phnum > 0 && phnum < 16 && i + phoff + 32u * phnum <= d.size();
        for (u16 k = 0; ok && k < phnum; ++k) {
            const u8* p = e + phoff + 32u * k;
            SpuSegment s{load_be<u32>(p + 8), load_be<u32>(p + 4), load_be<u32>(p + 16), load_be<u32>(p + 20),
                         load_be<u32>(p + 24)};
            if (load_be<u32>(p) != 1) continue;
            if (s.ls_addr + s.memsz > 0x40000 || i + s.file_offset + s.filesz > d.size()) ok = false;
            img.segments.push_back(s);
            end = std::max(end, s.file_offset + s.filesz);
        }
        if (!ok || img.segments.empty()) continue;
        if (shoff && shnum && shnum < 64) end = std::max<u32>(end, shoff + 40u * shnum);
        img.size = std::min<u32>(end, static_cast<u32>(d.size() - i));
        img.hint = find_hint(e, img.size);
        const u32 skip = img.size & ~3u;
        out.push_back(std::move(img));
        if (skip > 4) i += skip - 4;  // don't rescan the image body
    }
    return out;
}

}  // namespace opentit::loader
