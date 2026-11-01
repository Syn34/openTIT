// PRX loader: relocation, module info, exports and module_start.
#include <cstring>
#include <vector>

#include "loader/elf.h"
#include "test.h"

using namespace opentit;

namespace {

struct Rel { u64 offset; u8 index_value; u8 index_addr; u32 type; u64 ptr; };

std::vector<u8> make_prx() {
    std::vector<u8> f(0x3100, 0);
    auto w16 = [&](std::size_t o, u16 v) { store_be<u16>(&f[o], v); };
    auto w32 = [&](std::size_t o, u32 v) { store_be<u32>(&f[o], v); };
    auto w64 = [&](std::size_t o, u64 v) { store_be<u64>(&f[o], v); };
    std::memcpy(f.data(), "\x7f" "ELF", 4);
    f[4] = 2; f[5] = 2; f[6] = 1; f[7] = 0x66;
    w16(16, 0xFFA4); w16(18, 21); w32(20, 1);
    w64(32, 64); w16(52, 64); w16(54, 56); w16(56, 3);
    auto ph = [&](int i, u32 type, u32 flags, u64 off, u64 va, u64 pa, u64 sz) {
        const std::size_t o = 64 + 56 * i;
        w32(o, type); w32(o + 4, flags); w64(o + 8, off); w64(o + 16, va); w64(o + 24, pa);
        w64(o + 32, sz); w64(o + 40, sz); w64(o + 48, 0x10);
    };
    ph(0, 1, 5, 0x1000, 0, 0x1000 + 0x80, 0x100);  // code + lib info at +0x80
    ph(1, 1, 6, 0x2000, 0x10000, 0, 0x100);        // data
    std::vector<Rel> rels;
    // seg0: li r3, 7 ; blr
    w32(0x1000, 0x38600007); w32(0x1004, 0x4E800020);
    // seg0+0x80 lib info: attributes, version, name[28], toc, exports_start/end, imports_start/end
    w16(0x1080, 0); std::memcpy(&f[0x1084], "testmod", 8);
    rels.push_back({0x80 + 32, 1, 0, 1, 0x8000});   // toc = data + 0x8000
    rels.push_back({0x80 + 36, 1, 0, 1, 0x20});     // exports_start
    rels.push_back({0x80 + 40, 1, 0, 1, 0x58});     // exports_end
    rels.push_back({0x80 + 44, 1, 0, 1, 0x58});     // imports_start (empty)
    rels.push_back({0x80 + 48, 1, 0, 1, 0x58});     // imports_end
    // seg1: OPDs
    rels.push_back({0x00, 0, 1, 1, 0x0});    w32(0x2004, 0); rels.push_back({0x04, 1, 1, 1, 0x8000});  // exported fn
    rels.push_back({0x08, 0, 1, 1, 0x0});    rels.push_back({0x0C, 1, 1, 1, 0x8000});                   // module_start
    // management export record @0x20 (size 0x1C, attr 0x8000, 1 func)
    f[0x2020] = 0x1C; w16(0x2024, 0x8000); w16(0x2026, 1);
    rels.push_back({0x20 + 0x14, 1, 1, 1, 0x60}); rels.push_back({0x20 + 0x18, 1, 1, 1, 0x68});
    // library export record @0x3C (attr 1, name, 1 func)
    f[0x203C] = 0x1C; w16(0x2040, 0x0001); w16(0x2042, 1);
    rels.push_back({0x3C + 0x10, 1, 1, 1, 0x70}); rels.push_back({0x3C + 0x14, 1, 1, 1, 0x78});
    rels.push_back({0x3C + 0x18, 1, 1, 1, 0x7C});
    w32(0x2060, 0xBC9A0086); rels.push_back({0x68, 1, 1, 1, 0x08});
    std::memcpy(&f[0x2070], "testlib", 8);
    w32(0x2078, 0x12345678); rels.push_back({0x7C, 1, 1, 1, 0x00});
    // seg0 instruction patched by REL24 (b to seg0+0) at seg0+0x10, and an ADDR16_HA/LO pair
    w32(0x1010, 0x48000000); rels.push_back({0x10, 0, 0, 10, 0x0});
    w32(0x1014, 0x3D200000); rels.push_back({0x16, 1, 0, 6, 0x9000});   // lis r9, ha(data+0x9000)
    w32(0x1018, 0x39290000); rels.push_back({0x1A, 1, 0, 4, 0x9000});   // addi r9, r9, lo(...)
    // relocation segment
    ph(2, 0x700000A4, 0, 0x3000, 0, 0, 24 * rels.size());
    for (std::size_t k = 0; k < rels.size(); ++k) {
        const std::size_t o = 0x3000 + 24 * k;
        if (o + 24 > f.size()) f.resize(o + 24);
        w64(o, rels[k].offset); f[o + 10] = rels[k].index_value; f[o + 11] = rels[k].index_addr;
        w32(o + 12, rels[k].type); w64(o + 16, rels[k].ptr);
    }
    return f;
}

}  // namespace

TEST(prx_relocate_and_exports) {
    std::string err;
    auto m = loader::Elf::load_prx_from_memory(make_prx(), 0x01800000, &err);
    CHECK(m.has_value());
    if (!m) { std::printf("    %s\n", err.c_str()); return; }
    const u32 code = 0x01800000, data = 0x01810000;
    CHECK_STR(m->module_name(), "testmod");
    CHECK_EQ(m->prx_toc(), data + 0x8000);
    CHECK_EQ(m->module_start_opd(), data + 0x08);
    CHECK_EQ(m->exports().size(), 1u);
    if (!m->exports().empty()) {
        CHECK_STR(m->exports()[0].library, "testlib");
        CHECK_EQ(m->exports()[0].nid, 0x12345678u);
        CHECK_EQ(m->exports()[0].addr, data);
    }
    CHECK_EQ(m->read_u32(data), code);              // OPD func relocated
    CHECK_EQ(m->read_u32(data + 4), data + 0x8000);  // OPD toc relocated
    CHECK_EQ(m->read_u32(code + 0x10), 0x4BFFFFF0u); // b -16 (REL24 back to seg0+0)
    const u32 target = data + 0x9000;
    const u16 ha = static_cast<u16>((target >> 16) + ((target & 0x8000) ? 1 : 0));
    CHECK_EQ(m->read_u32(code + 0x14), 0x3D200000u | ha);
    CHECK_EQ(m->read_u32(code + 0x18), 0x39290000u | (target & 0xFFFF));
    CHECK_EQ(m->entry(), u64(data + 0x08));
}
