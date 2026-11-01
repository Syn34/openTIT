// RSX FIFO walker and method decoder, driven with hand-built command buffers.
#include <vector>

#include "../runtime/src/rsx/rsx.h"
#include "opentit/memory.h"
#include "test.h"

using namespace opentit;

namespace {

constexpr u32 kIoEa = 0x58000000;     // IO offset 0 lives here in these tests
constexpr u32 kLocal = 0x5A000000;    // stand-in for RSX local memory
constexpr u32 kControl = 0x5B000000;  // {put, get, ref}
constexpr u32 kLabels = 0x5B001000;
constexpr u32 kReports = 0x5B002000;

void setup() {
    mem::commit(kIoEa, 0x100000);
    mem::commit(kLocal, 0x100000);
    mem::commit(kControl, 0x10000);
}

rsx::Environment env(u32* ref = nullptr, u32* flipped = nullptr) {
    rsx::Environment e;
    e.to_ea = [](u32 off, bool local) { return (local ? kLocal : kIoEa) + off; };
    e.label_base = kLabels;
    e.local_report_base = kReports;
    e.on_reference = [ref](u32 v) { if (ref) *ref = v; };
    e.on_flip = [flipped](u32 v) { if (flipped) *flipped = v; };
    return e;
}

// Writes a command stream at IO offset `at` and returns the offset after it.
struct Writer {
    u32 at;
    Writer& w(u32 v) { mem::w32(kIoEa + at, v); at += 4; return *this; }
    Writer& m(u32 reg, std::initializer_list<u32> args) {
        w((static_cast<u32>(args.size()) << 18) | reg);
        for (u32 a : args) w(a);
        return *this;
    }
};

void run(rsx::Decoder& d, u32 put) {
    mem::w32(kControl + 4, 0);  // get
    mem::w32(kControl, put);    // put
    rsx::Fifo f(d, kControl);
    CHECK(f.run_until_idle([](u32 io) { return kIoEa + io; }));
}

struct Recorder : rsx::Backend {
    std::vector<rsx::DrawCall> draws;
    std::vector<u32> clears;
    void draw(const rsx::State&, const rsx::DrawCall& c) override { draws.push_back(c); }
    void clear(const rsx::State&, u32 mask) override { clears.push_back(mask); }
};

}  // namespace

TEST(RsxFifoJumpCallReturnAndSync) {
    setup();
    u32 ref = 0, flipped = 0xFFFF;
    rsx::Decoder d(env(&ref, &flipped));
    Writer w{0};
    w.m(rsx::NV406E_SET_REFERENCE, {0x1234});
    w.w(0x100 | 2);  // call 0x100
    w.w(0x200 | 1);  // jump 0x200
    Writer sub{0x100};
    sub.m(rsx::NV406E_SEMAPHORE_OFFSET, {0x20}).m(rsx::NV406E_SEMAPHORE_RELEASE, {0xCAFE});
    sub.m(rsx::NV4097_SET_SEMAPHORE_OFFSET, {0x30}).m(rsx::NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, {0x11223344});
    sub.w(0x00020000);  // return
    Writer tail{0x200};
    tail.m(rsx::GCM_FLIP_COMMAND, {1});
    run(d, tail.at);
    CHECK_EQ(ref, 0x1234u);
    CHECK_EQ(mem::r32(kLabels + 0x20), 0xCAFEu);
    CHECK_EQ(mem::r32(kLabels + 0x30), 0x11443322u);  // back-end release swaps bytes 0 and 2
    CHECK_EQ(flipped, 1u);
    CHECK_EQ(d.flip_count(), 1u);
    CHECK_EQ(mem::r32(kControl + 4), tail.at);
}

TEST(RsxDrawCapture) {
    setup();
    Recorder rec;
    rsx::Decoder d(env(), &rec);
    Writer w{0};
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 4 * 3, {(16u << 8) | (3u << 4) | 2u});  // float3, stride 16
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 4 * 3, {0x80000000u | 0x4000});         // main memory
    w.m(rsx::NV4097_SET_BEGIN_END, {5});  // triangles
    w.w((2u << 18) | 0x40000000u | rsx::NV4097_DRAW_ARRAYS).w((5u << 24) | 3).w((2u << 24) | 9);  // non-incrementing
    w.m(rsx::NV4097_SET_BEGIN_END, {0});
    w.m(rsx::NV4097_SET_INDEX_ARRAY_ADDRESS, {0x8000}).m(rsx::NV4097_SET_INDEX_ARRAY_DMA, {(1u << 4) | 1u});
    w.m(rsx::NV4097_SET_BEGIN_END, {6}).m(rsx::NV4097_DRAW_INDEX_ARRAY, {(255u << 24) | 0});
    w.m(rsx::NV4097_SET_BEGIN_END, {0});
    w.m(rsx::NV4097_SET_BEGIN_END, {5}).m(rsx::NV4097_SET_BEGIN_END, {0});  // empty: dropped
    w.m(rsx::NV4097_CLEAR_SURFACE, {0xF3});
    run(d, w.at);

    CHECK_EQ(rec.draws.size(), std::size_t(2));
    CHECK_EQ(d.draw_count(), 2u);
    CHECK(rec.draws[0].prim == rsx::Prim::Triangles);
    CHECK(!rec.draws[0].indexed);
    CHECK_EQ(rec.draws[0].ranges.size(), std::size_t(2));
    CHECK_EQ(rec.draws[0].ranges[0].first, 3u);
    CHECK_EQ(rec.draws[0].ranges[0].second, 6u);
    CHECK_EQ(rec.draws[0].ranges[1].first, 9u);
    CHECK_EQ(rec.draws[0].ranges[1].second, 3u);
    CHECK(rec.draws[1].indexed);
    CHECK(rec.draws[1].index16);
    CHECK(!rec.draws[1].index_local);
    CHECK_EQ(rec.draws[1].index_offset, 0x8000u);
    CHECK_EQ(rec.draws[1].ranges[0].second, 256u);
    CHECK_EQ(rec.clears.size(), std::size_t(1));
    CHECK_EQ(rec.clears[0], 0xF3u);

    const rsx::VertexArray a = d.state().vertex_array(3);
    CHECK(a.enabled());
    CHECK(!a.local);
    CHECK_EQ(a.offset, 0x4000u);
    CHECK_EQ(a.size, 3u);
    CHECK_EQ(a.stride, 16u);
    CHECK_EQ(a.type, 2u);
    CHECK(!d.state().vertex_array(0).enabled());
}

TEST(RsxProgramAndConstantUpload) {
    setup();
    rsx::Decoder d(env());
    Writer w{0};
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_LOAD, {2});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM, {1, 2, 3, 4});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM, {5, 6, 7, 8});  // continues where the last packet stopped
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_START, {2});
    w.m(rsx::NV4097_SET_TRANSFORM_CONSTANT_LOAD, {10});
    w.m(rsx::NV4097_SET_TRANSFORM_CONSTANT, {0x3F800000, 0x40000000, 0x40400000, 0x40800000});
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x12340 | 1});
    run(d, w.at);
    const auto& s = d.state();
    for (u32 k = 0; k < 8; ++k) CHECK_EQ(s.vp_ucode[8 + k], k + 1);
    CHECK_EQ(s.vp_ucode[7], 0u);
    CHECK_EQ(s.vp_start(), 2u);
    CHECK_EQ(s.vp_const[40], 0x3F800000u);
    CHECK_EQ(s.vp_const[43], 0x40800000u);
    CHECK_EQ(s.fp_offset(), 0x12340u);
    CHECK(s.fp_local());
}

TEST(RsxSurfaceAndTextureState) {
    setup();
    rsx::Decoder d(env());
    Writer w{0};
    w.m(rsx::NV4097_SET_SURFACE_FORMAT, {(10u << 24) | (11u << 16) | (1u << 8) | (2u << 5) | 8u});
    w.m(rsx::NV4097_SET_SURFACE_CLIP_HORIZONTAL, {(1280u << 16) | 0});
    w.m(rsx::NV4097_SET_SURFACE_CLIP_VERTICAL, {(720u << 16) | 0});
    w.m(rsx::NV4097_SET_SURFACE_COLOR_AOFFSET, {0x100000}).m(rsx::NV4097_SET_SURFACE_PITCH_A, {5120});
    w.m(rsx::NV4097_SET_CONTEXT_DMA_COLOR_A, {rsx::CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER});
    w.m(rsx::NV4097_SET_TEXTURE_OFFSET + 0x20 * 2, {0x2000, (10u << 16) | (0x85u << 8) | (2u << 4) | 1u});
    w.m(rsx::NV4097_SET_TEXTURE_OFFSET + 0x20 * 2 + 0xC, {0x80000000u});
    w.m(rsx::NV4097_SET_TEXTURE_OFFSET + 0x20 * 2 + 0x18, {(256u << 16) | 128u});
    run(d, w.at);
    const rsx::Surface s = d.state().surface();
    CHECK_EQ(s.color_format, 8u);
    CHECK_EQ(s.depth_format, 2u);
    CHECK_EQ(s.width, 1280u);
    CHECK_EQ(s.height, 720u);
    CHECK_EQ(s.color_offset[0], 0x100000u);
    CHECK_EQ(s.color_pitch[0], 5120u);
    CHECK(!s.color_local[0]);
    CHECK(s.color_local[1]);
    const rsx::Texture t = d.state().texture(2);
    CHECK(t.enabled());
    CHECK(t.local);
    CHECK_EQ(t.format, 0x85u);
    CHECK_EQ(t.dimension, 2u);
    CHECK_EQ(t.mipmaps, 10u);
    CHECK_EQ(t.width, 256u);
    CHECK_EQ(t.height, 128u);
    CHECK(!d.state().texture(0).enabled());
}

TEST(RsxCopyEngines) {
    setup();
    rsx::Decoder d(env());
    for (u32 k = 0; k < 64; ++k) mem::w8(kLocal + 0x1000 + k, static_cast<u8>(k));
    Writer w{0};
    // NV0039: two lines of 8 bytes, local pitch 16 -> main pitch 8
    w.m(rsx::NV0039_SET_CONTEXT_DMA_BUFFER_IN, {rsx::CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER});
    w.m(rsx::NV0039_SET_CONTEXT_DMA_BUFFER_OUT, {rsx::CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER});
    w.m(rsx::NV0039_OFFSET_IN, {0x1000, 0x40000, 16, 8, 8, 2, 0x101, 0});  // offsets .. format, notify
    // NV3089 -> NV3062: 2x2 block of 32-bit pixels, unscaled
    for (u32 k = 0; k < 16; ++k) mem::w32(kLocal + 0x2000 + 4 * k, 0xA0000000u + k);  // 4x4 source, pitch 16
    w.m(rsx::NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN, {rsx::CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER});
    w.m(rsx::NV3062_SET_COLOR_FORMAT, {0xA, 32u << 16, 0, 0x3000});  // format, pitch, src, dst
    w.m(rsx::NV3089_SET_CONTEXT_DMA_IMAGE, {rsx::CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER});
    w.m(rsx::NV3089_SET_CONTEXT_SURFACE, {0x313371C3});
    w.m(rsx::NV3089_SET_COLOR_FORMAT, {3});
    w.m(rsx::NV3089_CLIP_POINT, {0, (8u << 16) | 8u, 0, (2u << 16) | 2u, 1u << 20, 1u << 20});
    w.m(rsx::NV3089_IMAGE_IN_SIZE, {(4u << 16) | 4u, 16u, 0x2000, (1u << 4) | ((1u << 4) << 16)});  // from (1,1)
    run(d, w.at);
    for (u32 k = 0; k < 8; ++k) {
        CHECK_EQ(mem::r8(kIoEa + 0x40000 + k), static_cast<u8>(k));
        CHECK_EQ(mem::r8(kIoEa + 0x40008 + k), static_cast<u8>(16 + k));
    }
    CHECK_EQ(mem::r32(kLocal + 0x3000), 0xA0000005u);       // src (1,1)
    CHECK_EQ(mem::r32(kLocal + 0x3004), 0xA0000006u);       // src (2,1)
    CHECK_EQ(mem::r32(kLocal + 0x3000 + 32), 0xA0000009u);  // src (1,2)
    CHECK_EQ(d.copy_count(), 2u);
}

TEST(RsxReportsAndNames) {
    setup();
    rsx::Decoder d(env());
    Writer w{0};
    w.m(rsx::NV4097_SET_CONTEXT_DMA_REPORT, {rsx::CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL});
    w.m(rsx::NV4097_GET_REPORT, {(1u << 24) | 0x40});
    run(d, w.at);
    CHECK(mem::r64(kReports + 0x40) != 0);
    CHECK(mem::r32(kReports + 0x48) != 0);  // reported visible
    CHECK_STR(rsx::describe_method(0x1814), "NV4097_DRAW_ARRAYS");
    CHECK_STR(rsx::describe_method(0x1A24), "NV4097_SET_TEXTURE_FORMAT+1");
    CHECK_STR(rsx::describe_method(0x1F08), "NV4097_SET_TRANSFORM_CONSTANT+2");
    CHECK_STR(rsx::describe_method(0xFEAC), "GCM_FLIP_COMMAND");
}

// ---------------------------------------------------------------- vertex programs
#include "../runtime/src/rsx/vertex_program.h"

namespace {
namespace vp = opentit::rsx::vp;

constexpr u32 kIdent = (3u << 8) | (2u << 10) | (1u << 12);  // swizzle .xyzw
u32 vsrc(u32 type, u32 tmp = 0, u32 swz = kIdent, bool neg = false) {
    return type | (tmp << 2) | swz | (neg ? 1u << 16 : 0);
}
struct VpBuilder {
    vp::Insn i;
    VpBuilder() {
        i.d[0] = 0x3Fu << 15;                  // no temp destination
        i.d[3] = (0x1Fu << 2) | (0x3Fu << 7);  // no output, no scalar temp
    }
    VpBuilder& vec(u32 op) { i.d[1] |= op << 22; return *this; }
    VpBuilder& sca(u32 op) { i.d[1] |= op << 27; return *this; }
    VpBuilder& input(u32 n) { i.d[1] |= n << 8; return *this; }
    VpBuilder& cnst(u32 n) { i.d[1] |= n << 12; return *this; }
    VpBuilder& src0(u32 s) { i.d[1] |= s >> 9; i.d[2] |= (s & 0x1FF) << 23; return *this; }
    VpBuilder& src1(u32 s) { i.d[2] |= s << 6; return *this; }
    VpBuilder& src2(u32 s) { i.d[2] |= s >> 11; i.d[3] |= (s & 0x7FF) << 21; return *this; }
    VpBuilder& out(u32 o, u32 mask) { i.d[0] |= 1u << 30; i.d[3] = (i.d[3] & ~(0x1Fu << 2)) | (o << 2) | (mask << 13); return *this; }
    VpBuilder& tmp(u32 r, u32 mask) { i.d[0] = (i.d[0] & ~(0x3Fu << 15)) | (r << 15); i.d[3] |= mask << 13; return *this; }
    VpBuilder& stmp(u32 r, u32 mask) { i.d[3] = (i.d[3] & ~(0x3Fu << 7)) | (r << 7) | (mask << 17); return *this; }
    VpBuilder& target(u32 t) { i.d[0] |= ((t >> 9) & 1) << 23; i.d[2] |= ((t >> 3) & 0x3F); i.d[3] |= (t & 7) << 29; return *this; }
    VpBuilder& end() { i.d[3] |= 1; return *this; }
    void put(u32* ucode, u32 slot) const { for (int k = 0; k < 4; ++k) ucode[slot * 4 + k] = i.d[k]; }
};
}  // namespace

TEST(RsxVertexProgramDisasm) {
    const vp::Insn dp4 = VpBuilder().vec(vp::VEC_DP4).input(0).cnst(4).src0(vsrc(vp::REG_INPUT)).src1(vsrc(vp::REG_CONST))
                             .out(0, 8).end().i;
    CHECK_STR(vp::disasm(dp4), "DP4 o[HPOS].x, v0, c[4]  END");
    CHECK_EQ(dp4.src(1).index, 4u);
    CHECK(dp4.end());

    const u32 wwww = (3u << 8) | (3u << 10) | (3u << 12) | (3u << 14);
    const vp::Insn dual = VpBuilder().vec(vp::VEC_MUL).sca(vp::SCA_RSQ).input(2).cnst(1)
                              .src0(vsrc(vp::REG_INPUT)).src1(vsrc(vp::REG_CONST, 0, kIdent, true))
                              .src2(vsrc(vp::REG_TEMP, 5, wwww)).tmp(0, 0xE).stmp(1, 8).i;
    CHECK_STR(vp::disasm(dual), "MUL R0.xyz, v2, -c[1]; RSQ R1.x, R5.w");

    const vp::Insn add = VpBuilder().vec(vp::VEC_ADD).src0(vsrc(vp::REG_TEMP, 3)).src2(vsrc(vp::REG_TEMP, 7)).out(7, 0xF).i;
    CHECK_STR(vp::disasm(add), "ADD o[TEX0], R3, R7");  // ADD reads src0 and src2
}

TEST(RsxVertexProgramAnalyze) {
    std::vector<u32> ucode(512 * 4, 0);
    // slot 10: MOV R2, v3          slot 11: BRA 14
    // slot 12: (skipped code)      slot 14: DP4 o[HPOS].x, R2, c[7]  END
    VpBuilder().vec(vp::VEC_MOV).input(3).src0(vsrc(vp::REG_INPUT)).tmp(2, 0xF).put(ucode.data(), 10);
    VpBuilder().sca(vp::SCA_BRA).target(14).put(ucode.data(), 11);
    VpBuilder().vec(vp::VEC_MOV).input(8).src0(vsrc(vp::REG_INPUT)).out(7, 0xF).put(ucode.data(), 12);
    VpBuilder().vec(vp::VEC_DP4).cnst(7).src0(vsrc(vp::REG_TEMP, 2)).src1(vsrc(vp::REG_CONST)).out(0, 8).end()
        .put(ucode.data(), 14);
    const vp::Program p = vp::analyze(ucode.data(), 10);
    CHECK(p.has_end);
    CHECK_EQ(p.insns.size(), std::size_t(5));       // 10..14 inclusive
    CHECK_EQ(p.branch_targets.count(14), std::size_t(1));
    CHECK_EQ(vp::fetch(ucode.data(), 11).branch_target(), 14u);
    CHECK_EQ(p.inputs_read, (1u << 3) | (1u << 8));  // the skipped slot is still inside the range
    CHECK_EQ(p.outputs_written, 1u | (1u << 7));
    CHECK_EQ(p.constants.count(7), std::size_t(1));
    CHECK(!p.indexed_constants);
    CHECK_EQ(p.temps_used, 1ull << 2);
}

// ---------------------------------------------------------------- fragment programs
#include "../runtime/src/rsx/fragment_program.h"

namespace {
namespace fp = opentit::rsx::fp;

constexpr u32 kFpIdent = (1u << 11) | (2u << 13) | (3u << 15);           // .xyzw
constexpr u32 kFpExecAll = (7u << 18) | (1u << 23) | (2u << 25) | (3u << 27);  // TR, cond .xyzw
u32 fdst(u32 op, u32 reg, u32 mask, bool half = false, u32 attr = 0, u32 tex = 0, bool end = false) {
    return (end ? 1u : 0u) | (reg << 1) | (half ? 1u << 7 : 0u) | (mask << 9) | (attr << 13) | (tex << 17) |
           ((op & 0x3F) << 24);
}
u32 fsrc(u32 type, u32 reg = 0, bool neg = false) { return type | (reg << 2) | kFpIdent | (neg ? 1u << 17 : 0u); }

struct FpWriter {
    std::vector<u8> bytes;
    void word(u32 v) {  // stored with halfwords swapped, big endian
        const u32 stored = (v << 16) | (v >> 16);
        u8 b[4];
        store_be<u32>(b, stored);
        bytes.insert(bytes.end(), b, b + 4);
    }
    void insn(u32 d, u32 s0, u32 s1 = 0, u32 s2 = 0) { word(d); word(s0 | kFpExecAll); word(s1); word(s2); }
    void constant(float a, float b, float c, float d) {
        for (float f : {a, b, c, d}) { u32 v; std::memcpy(&v, &f, 4); word(v); }
    }
};
}  // namespace

TEST(RsxFragmentProgramParse) {
    FpWriter w;
    w.insn(fdst(fp::NOP, 0, 0), 0);  // leading NOP, skipped
    w.insn(fdst(fp::TEX, 0, 0xF, false, fp::IN_TEX0, 0), fsrc(fp::REG_INPUT));
    w.insn(fdst(fp::MUL, 0, 0xF), fsrc(fp::REG_TEMP, 0), fsrc(fp::REG_CONST));
    w.constant(0.5f, 0.25f, 1.0f, 2.0f);
    w.insn(fdst(fp::MOV, 2, 0x3, true, 0, 0, true), fsrc(fp::REG_TEMP, 0, true));
    w.bytes.resize(w.bytes.size() + 64, 0xEE);  // garbage after END must not be read
    const fp::Program p = fp::parse(w.bytes.data(), static_cast<u32>(w.bytes.size()));
    CHECK(!p.truncated);
    CHECK_EQ(p.start, 16u);
    CHECK_EQ(p.size, 64u);
    CHECK_EQ(p.insns.size(), std::size_t(3));
    CHECK_EQ(p.textures, 1u);
    CHECK_EQ(p.inputs, 1u << fp::IN_TEX0);
    CHECK_EQ(p.temps_written, 1u);
    CHECK_EQ(p.halves_written, 1u << 2);
    CHECK(!p.writes_depth);
    CHECK(p.insns[1].has_constant);
    CHECK_EQ(p.insns[2].offset, 48u);
    CHECK_STR(fp::disasm(p.insns[0]), "TEXR R0, f[TEX0], TEX0");
    CHECK_STR(fp::disasm(p.insns[1]), "MULR R0, R0, {0.5, 0.25, 1, 2}");
    CHECK_STR(fp::disasm(p.insns[2]), "MOVR H2.xy, -R0  END");
}

TEST(RsxFragmentProgramTruncated) {
    FpWriter w;
    w.insn(fdst(fp::MOV, 0, 0xF), fsrc(fp::REG_TEMP, 1));  // no END
    const fp::Program p = fp::parse(w.bytes.data(), static_cast<u32>(w.bytes.size()));
    CHECK(p.truncated);
    CHECK_EQ(p.insns.size(), std::size_t(1));
}

// ---------------------------------------------------------------- shader dump
#include <filesystem>

#include "../runtime/src/rsx/shader_dump.h"

TEST(RsxShaderDump) {
    setup();
    const std::string dir = (std::filesystem::temp_directory_path() / "opentit_shader_dump_test").string();
    std::filesystem::remove_all(dir);
    rsx::ShaderDumpBackend dump(dir, [](u32 off, bool local) { return (local ? kLocal : kIoEa) + off; });
    rsx::Decoder d(env(), &dump);

    FpWriter fw;
    fw.insn(fdst(fp::MOV, 0, 0xF, false, 0, 0, true), fsrc(fp::REG_INPUT));
    for (std::size_t k = 0; k < fw.bytes.size(); ++k) mem::w8(kLocal + 0x8000 + static_cast<u32>(k), fw.bytes[k]);
    const vp::Insn mov = VpBuilder().vec(vp::VEC_MOV).src0(vsrc(vp::REG_INPUT)).out(0, 0xF).end().i;

    Writer w{0};
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_LOAD, {0});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM, {mov.d[0], mov.d[1], mov.d[2], mov.d[3]});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_START, {0});
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x8000 | 1});
    for (int rep = 0; rep < 3; ++rep)  // same programs three times: dumped once
        w.m(rsx::NV4097_SET_BEGIN_END, {5}).m(rsx::NV4097_DRAW_ARRAYS, {2u << 24}).m(rsx::NV4097_SET_BEGIN_END, {0});
    run(d, w.at);
    CHECK_EQ(d.draw_count(), 3u);
    CHECK_EQ(dump.vertex_programs(), std::size_t(1));
    CHECK_EQ(dump.fragment_programs(), std::size_t(1));
    std::size_t files = 0;
    for (const auto& e : std::filesystem::directory_iterator(dir)) files += e.is_regular_file();
    CHECK_EQ(files, std::size_t(2));
    std::filesystem::remove_all(dir);
}
