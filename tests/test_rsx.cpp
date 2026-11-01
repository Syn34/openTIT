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
