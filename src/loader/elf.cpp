#include "loader/elf.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace opentit::loader {

namespace {
constexpr u32 kPrxParamMagic = 0x1B434CEC;
constexpr u32 kProcParamMagic = 0x13BCC5F6;
}  // namespace

std::optional<Elf> Elf::load(const std::string& path, std::string* error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return std::nullopt;
    }
    Elf elf;
    elf.data_.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    if (!elf.parse(error)) return std::nullopt;
    return elf;
}

bool Elf::parse(std::string* error) {
    auto fail = [&](const char* msg) {
        if (error) *error = msg;
        return false;
    };
    if (data_.size() < 64) return fail("file too small to be an ELF");
    if (data_[0] == 'S' && data_[1] == 'C' && data_[2] == 'E' && data_[3] == 0)
        return fail("this is an encrypted SELF; decrypt it first (e.g. RPCS3: Utilities > Decrypt PS3 Binaries)");
    if (std::memcmp(data_.data(), "\x7f" "ELF", 4) != 0) return fail("not an ELF file");
    if (data_[4] != 2) return fail("expected ELF64");
    if (data_[5] != 2) return fail("expected big-endian ELF");

    const u8* d = data_.data();
    const u16 machine = load_be<u16>(d + 18);
    if (machine != 21) return fail("expected EM_PPC64 (PS3 PPU executable)");

    entry_ = load_be<u64>(d + 24);
    const u64 phoff = load_be<u64>(d + 32);
    const u64 shoff = load_be<u64>(d + 40);
    const u16 phentsize = load_be<u16>(d + 54);
    const u16 phnum = load_be<u16>(d + 56);
    const u16 shentsize = load_be<u16>(d + 58);
    const u16 shnum = load_be<u16>(d + 60);

    u32 prx_param_va = 0;
    u32 proc_param_va = 0;
    for (u16 i = 0; i < phnum; ++i) {
        const u64 o = phoff + u64(i) * phentsize;
        if (o + 56 > data_.size()) return fail("program header table truncated");
        Segment s;
        s.type = load_be<u32>(d + o);
        s.flags = load_be<u32>(d + o + 4);
        s.offset = load_be<u64>(d + o + 8);
        s.vaddr = load_be<u64>(d + o + 16);
        s.filesz = load_be<u64>(d + o + 32);
        s.memsz = load_be<u64>(d + o + 40);
        s.align = load_be<u64>(d + o + 48);
        if (s.offset + s.filesz > data_.size()) return fail("segment extends past end of file");
        segments_.push_back(s);
        if (s.type == PT_PROC_PRX_PARAM) prx_param_va = static_cast<u32>(s.vaddr);
        if (s.type == PT_PROC_PARAM) proc_param_va = static_cast<u32>(s.vaddr);
        if (s.type == PT_TLS) tls_ = s;
    }

    if (shoff && shnum) {
        for (u16 i = 0; i < shnum; ++i) {
            const u64 o = shoff + u64(i) * shentsize;
            if (o + 64 > data_.size()) break;
            Section s;
            s.name = load_be<u32>(d + o);
            s.type = load_be<u32>(d + o + 4);
            s.flags = load_be<u64>(d + o + 8);
            s.addr = load_be<u64>(d + o + 16);
            s.offset = load_be<u64>(d + o + 24);
            s.size = load_be<u64>(d + o + 32);
            s.entsize = load_be<u64>(d + o + 56);
            sections_.push_back(s);
        }
    }

    if (proc_param_va && ptr(proc_param_va, 32)) {
        proc_param_.size = read_u32(proc_param_va);
        proc_param_.magic = read_u32(proc_param_va + 4);
        proc_param_.version = read_u32(proc_param_va + 8);
        proc_param_.sdk_version = read_u32(proc_param_va + 12);
        proc_param_.primary_prio = static_cast<s32>(read_u32(proc_param_va + 16));
        proc_param_.primary_stacksize = read_u32(proc_param_va + 20);
        proc_param_.malloc_pagesize = read_u32(proc_param_va + 24);
        proc_param_.ppc_seg = read_u32(proc_param_va + 28);
        if (proc_param_.magic != kProcParamMagic) proc_param_ = ProcessParam{};
    }

    if (prx_param_va && !parse_imports(prx_param_va, error)) return false;
    return true;
}


// prx_param layout: size, magic, version, unk0, libent_start, libent_end,
//                   libstub_start, libstub_end, ...
// Each libstub record: u8 size, u8 unk, u16 version, u16 attr, u16 num_func,
//   u16 num_var, u16 num_tlsvar, u8 hash_info, u8 tls_hash_info, u16 reserved,
//   u32 name, u32 func_nid[], u32 func_stub[], u32 var_nid[], u32 var_stub[], ...
bool Elf::parse_imports(u32 pp, std::string* error) {
    if (!ptr(pp, 0x20)) return true;
    if (read_u32(pp + 4) != kPrxParamMagic) {
        if (error) *error = "unexpected prx_param magic";
        return false;
    }
    parse_stub_records(read_u32(pp + 0x18), read_u32(pp + 0x1C));
    return true;
}

void Elf::parse_stub_records(u32 start, u32 end) {
    for (u32 va = start; va < end;) {
        const u8 rec = read_u8(va);
        if (rec == 0) break;
        const u16 num_func = read_u16(va + 0x06);
        const u16 num_var = read_u16(va + 0x08);
        const u32 name_va = read_u32(va + 0x10);
        const u32 fnid = read_u32(va + 0x14);
        const u32 fstub = read_u32(va + 0x18);
        const u32 vnid = read_u32(va + 0x1C);
        const u32 vstub = read_u32(va + 0x20);
        const std::string lib = name_va ? read_cstr(name_va) : "<unnamed>";
        for (u32 i = 0; i < num_func; ++i) {
            Import im;
            im.library = lib;
            im.nid = read_u32(fnid + 4 * i);
            im.slot = fstub + 4 * i;
            im.initial = read_u32(im.slot);
            imports_.push_back(im);
        }
        for (u32 i = 0; i < num_var && vnid && vstub; ++i) {
            Import im;
            im.library = lib;
            im.nid = read_u32(vnid + 4 * i);
            im.slot = vstub + 4 * i;
            im.initial = read_u32(im.slot);
            im.is_variable = true;
            imports_.push_back(im);
        }
        va += rec;
    }
}

std::optional<Elf> Elf::load_prx(const std::string& path, u32 base, std::string* error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return std::nullopt;
    }
    std::vector<u8> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return load_prx_from_memory(std::move(d), base, error);
}

std::optional<Elf> Elf::load_prx_from_memory(std::vector<u8> file, u32 base, std::string* error) {
    Elf elf;
    elf.data_ = std::move(file);
    if (!elf.parse(error)) return std::nullopt;
    if (!elf.relocate_prx(base, error)) return std::nullopt;
    return elf;
}

u32 Elf::image_end() const {
    u64 end = 0;
    for (const auto& s : segments_)
        if (s.type == PT_LOAD && s.memsz) end = std::max(end, s.vaddr + s.memsz);
    return static_cast<u32>(end);
}

// PRX relocation (SCE_PPURELA, p_type 0x700000A4) record:
//   u64 offset; u16 unk; u8 index_value; u8 index_addr; u32 type; u64 ptr
bool Elf::relocate_prx(u32 base, std::string* error) {
    const u8* d = data_.data();
    if (load_be<u16>(d + 16) != 0xFFA4) {
        if (error) *error = "not a PRX (expected e_type 0xFFA4)";
        return false;
    }
    is_prx_ = true;
    const u64 phoff = load_be<u64>(d + 32);
    const u16 phnum = load_be<u16>(d + 56);

    // 1. Lay out PT_LOAD segments and build the relocated image (memsz each).
    struct Placed { std::size_t phdr_index; u32 addr; u32 memsz; std::size_t image_off; u64 p_offset, p_paddr; };
    std::vector<Placed> placed;
    std::vector<u8> image;
    std::vector<Segment> new_segments;
    u32 cursor = base;
    for (std::size_t k = 0; k < segments_.size(); ++k) {
        const Segment& s = segments_[k];
        if (s.type != PT_LOAD) {
            new_segments.push_back(s);
            continue;
        }
        const u32 memsz = static_cast<u32>(s.memsz);
        Placed p{k, cursor, memsz, image.size(), s.offset, load_be<u64>(d + phoff + 56 * k + 24)};
        image.resize(image.size() + memsz, 0);
        std::memcpy(image.data() + p.image_off, d + s.offset, s.filesz);
        placed.push_back(p);
        Segment ns = s;
        ns.vaddr = cursor;
        ns.offset = p.image_off;
        ns.filesz = memsz;
        new_segments.push_back(ns);
        cursor += (memsz + 0xFFFF) & ~0xFFFFu;
    }

    // 2. Apply relocations.
    auto seg_addr = [&](u8 index) -> std::optional<u32> {
        if (index == 0xFF) return 0u;
        if (index >= placed.size()) return std::nullopt;
        return placed[index].addr;
    };
    for (u16 k = 0; k < phnum; ++k) {
        const u8* ph = d + phoff + 56u * k;
        if (load_be<u32>(ph) != 0x700000A4) continue;
        const u64 off = load_be<u64>(ph + 8), filesz = load_be<u64>(ph + 32);
        for (u64 i = 0; i + 24 <= filesz; i += 24) {
            const u8* rel = d + off + i;
            const u64 roff = load_be<u64>(rel);
            const u8 index_value = rel[10];
            const u8 index_addr = rel[11];
            const u32 type = load_be<u32>(rel + 12);
            const u64 ptr = load_be<u64>(rel + 16);
            if (index_addr >= placed.size() || roff + 8 > placed[index_addr].memsz + 0x100) {
                if (error) *error = "relocation outside of its segment";
                return false;
            }
            const auto dbase = seg_addr(index_value);
            if (!dbase) {
                if (error) *error = "relocation references a missing segment";
                return false;
            }
            const u32 raddr = placed[index_addr].addr + static_cast<u32>(roff);
            const u64 v = *dbase + ptr;
            u8* t = image.data() + placed[index_addr].image_off + roff;
            switch (type) {
            case 1: store_be<u32>(t, static_cast<u32>(v)); break;                                     // ADDR32
            case 4: store_be<u16>(t, static_cast<u16>(v)); break;                                     // ADDR16_LO
            case 5: store_be<u16>(t, static_cast<u16>(v >> 16)); break;                               // ADDR16_HI
            case 6: store_be<u16>(t, static_cast<u16>((v >> 16) + ((v & 0x8000) ? 1 : 0))); break;    // ADDR16_HA
            case 10: {                                                                                // REL24
                const u32 w = load_be<u32>(t);
                store_be<u32>(t, (w & 0xFC000003u) | ((static_cast<u32>(v - raddr)) & 0x03FFFFFCu));
                break;
            }
            case 11: {                                                                                // REL14
                const u32 w = load_be<u32>(t);
                store_be<u32>(t, (w & 0xFFFF0003u) | ((static_cast<u32>(v - raddr)) & 0xFFFCu));
                break;
            }
            case 38: store_be<u64>(t, v); break;                                                      // ADDR64
            case 44: store_be<u64>(t, v - raddr); break;                                              // REL64
            case 57: {                                                                                // ADDR16_LO_DS
                const u16 h = load_be<u16>(t);
                store_be<u16>(t, static_cast<u16>((h & 3) | (static_cast<u16>(v) & ~3u)));
                break;
            }
            default:
                if (error) *error = "unsupported PRX relocation type " + std::to_string(type);
                return false;
            }
        }
    }

    data_ = std::move(image);
    segments_ = std::move(new_segments);
    sections_.clear();
    tls_.reset();
    for (const auto& s : segments_)
        if (s.type == PT_TLS) tls_ = s;

    // 3. Library info lives at seg0 + p_paddr - p_offset:
    //    u16 attributes; u8 version[2]; char name[28]; u32 toc, exports_start, exports_end, imports_start, imports_end
    if (placed.empty() || !placed[0].p_paddr) {
        if (error) *error = "PRX has no module info";
        return false;
    }
    const u32 info = placed[0].addr + static_cast<u32>(placed[0].p_paddr - placed[0].p_offset);
    module_name_ = read_cstr(info + 4, 28);
    prx_toc_ = read_u32(info + 32);
    const u32 exports_start = read_u32(info + 36), exports_end = read_u32(info + 40);
    const u32 imports_start = read_u32(info + 44), imports_end = read_u32(info + 48);

    // Exports share the stub record layout (size, attributes, counts, name, nids, addrs).
    for (u32 va = exports_start; va < exports_end;) {
        const u8 rec = read_u8(va);
        const u16 attr = read_u16(va + 4);
        const u16 num_func = read_u16(va + 6);
        const u16 num_var = read_u16(va + 8);
        const u32 name_va = read_u32(va + 0x10);
        const u32 nids = read_u32(va + 0x14);
        const u32 addrs = read_u32(va + 0x18);
        const bool library = attr & 1;
        const bool management = !library && (attr & 0x8000);
        for (u32 i = 0; i < u32(num_func) + num_var; ++i) {
            const u32 nid = read_u32(nids + 4 * i);
            const u32 addr = read_u32(addrs + 4 * i);
            if (management) {
                if (nid == 0xBC9A0086) module_start_ = addr;
                if (nid == 0xAB779874) module_stop_ = addr;
                continue;
            }
            if (!library) continue;
            Export e;
            e.library = name_va ? read_cstr(name_va) : module_name_;
            e.nid = nid;
            e.addr = addr;
            e.is_variable = i >= num_func;
            exports_.push_back(e);
        }
        va += rec ? rec : 0x1C;
    }
    imports_.clear();
    parse_stub_records(imports_start, imports_end);
    entry_ = module_start_ ? module_start_ : (exports_.empty() ? 0 : exports_.front().addr);
    return true;
}

const u8* Elf::ptr(u64 va, u64 size) const {
    for (const auto& s : segments_) {
        if (s.type != PT_LOAD || !s.filesz) continue;
        if (va >= s.vaddr && va + size <= s.vaddr + s.filesz) return data_.data() + s.offset + (va - s.vaddr);
    }
    return nullptr;
}

bool Elf::is_exec(u64 va) const {
    for (const auto& s : segments_)
        if (s.type == PT_LOAD && (s.flags & PF_X) && va >= s.vaddr && va < s.vaddr + s.filesz) return true;
    return false;
}

bool Elf::is_mapped(u64 va) const {
    for (const auto& s : segments_)
        if (s.type == PT_LOAD && s.memsz && va >= s.vaddr && va < s.vaddr + s.memsz) return true;
    return false;
}

u32 Elf::read_u32(u64 va) const {
    const u8* p = ptr(va, 4);
    return p ? load_be<u32>(p) : 0;
}
u16 Elf::read_u16(u64 va) const {
    const u8* p = ptr(va, 2);
    return p ? load_be<u16>(p) : 0;
}
u8 Elf::read_u8(u64 va) const {
    const u8* p = ptr(va, 1);
    return p ? *p : 0;
}

std::string Elf::read_cstr(u64 va, std::size_t limit) const {
    std::string s;
    for (std::size_t i = 0; i < limit; ++i) {
        const u8* p = ptr(va + i, 1);
        if (!p || !*p) break;
        s.push_back(static_cast<char>(*p));
    }
    return s;
}

std::vector<Elf::Range> Elf::exec_ranges() const {
    std::vector<Range> r;
    // Prefer section headers (SHF_EXECINSTR) - the executable PT_LOAD segment
    // also contains .rodata, which must not be treated as code.
    constexpr u64 SHF_EXECINSTR = 4;
    for (const auto& s : sections_)
        if ((s.flags & SHF_EXECINSTR) && s.size && s.addr) r.push_back({s.addr, s.addr + s.size});
    if (!r.empty()) return r;
    for (const auto& s : segments_)
        if (s.type == PT_LOAD && (s.flags & PF_X) && s.filesz) r.push_back({s.vaddr, s.vaddr + s.filesz});
    return r;
}

Opd Elf::entry_opd() const {
    Opd o;
    o.addr = static_cast<u32>(entry_);
    o.func = read_u32(entry_);
    o.toc = read_u32(entry_ + 4);
    return o;
}

std::vector<Opd> Elf::scan_opds() const {
    std::vector<Opd> out;
    // Locate the section containing the entry descriptor; that is .opd.
    const Section* opd = nullptr;
    for (const auto& s : sections_)
        if (s.addr && entry_ >= s.addr && entry_ < s.addr + s.size) opd = &s;

    u64 begin, end;
    if (opd) {
        begin = opd->addr;
        end = opd->addr + opd->size;
    } else {
        // Stripped section table: walk outward from the entry while entries
        // keep looking like descriptors with the same TOC.
        const u32 toc = read_u32(entry_ + 4);
        begin = entry_;
        while (ptr(begin - 8, 8) && is_exec(read_u32(begin - 8)) && read_u32(begin - 4) == toc) begin -= 8;
        end = entry_;
        while (ptr(end, 8) && is_exec(read_u32(end)) && read_u32(end + 4) == toc) end += 8;
    }
    for (u64 va = begin; va + 8 <= end; va += 8) {
        Opd o;
        o.addr = static_cast<u32>(va);
        o.func = read_u32(va);
        o.toc = read_u32(va + 4);
        if (is_exec(o.func) && (o.func & 3) == 0) out.push_back(o);
    }
    return out;
}

}  // namespace opentit::loader
