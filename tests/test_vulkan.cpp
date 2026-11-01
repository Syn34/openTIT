// End-to-end RSX rendering through the Vulkan backend (software rasteriser
// such as lavapipe is fine). Skips when no Vulkan device is available.
#include <cstdio>
#include <cstring>
#include <vector>

#include "../runtime/src/rsx/vk/vk_backend.h"
#include "opentit/memory.h"
#include "test.h"

using namespace opentit;

namespace {
constexpr u32 kVkIo = 0x5D000000;     // IO offset 0
constexpr u32 kVkLocal = 0x5E000000;  // "RSX local memory"
constexpr u32 kVkCtl = 0x5F000000;

struct W {
    u32 at = 0;
    W& m(u32 reg, std::initializer_list<u32> a) {
        mem::w32(kVkIo + at, (static_cast<u32>(a.size()) << 18) | reg);
        at += 4;
        for (u32 v : a) { mem::w32(kVkIo + at, v); at += 4; }
        return *this;
    }
    W& f(u32 reg, std::initializer_list<float> a) {
        mem::w32(kVkIo + at, (static_cast<u32>(a.size()) << 18) | reg);
        at += 4;
        for (float v : a) { u32 b; std::memcpy(&b, &v, 4); mem::w32(kVkIo + at, b); at += 4; }
        return *this;
    }
};

u32 fbits(float f) { u32 b; std::memcpy(&b, &f, 4); return b; }

// Fragment-program word as stored in memory (halfwords swapped).
void fp_word(u32 ea, u32 v) { mem::w32(ea, (v << 16) | (v >> 16)); }
}  // namespace

TEST(VulkanClearAndDrawQuad) {
    if (!rsx::have_vulkan()) { std::printf("  (built without Vulkan: skipped)\n"); return; }
    mem::commit(kVkIo, 0x100000);
    mem::commit(kVkLocal, 0x100000);
    mem::commit(kVkCtl, 0x1000);
    std::string err;
    auto vk = rsx::create_vulkan_backend([](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; }, &err);
    if (!vk) { std::printf("  (no Vulkan device: %s - skipped)\n", err.c_str()); return; }
    std::printf("  device: %s\n", vk->device_name().c_str());

    rsx::Environment env;
    env.to_ea = [](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; };
    env.label_base = kVkCtl + 0x100;
    rsx::Decoder d(env, vk.get());

    constexpr u32 Wd = 64, Ht = 64, kColor = 0x10000, kPitch = Wd * 4;
    // Vertex data in main (IO) memory: 4 x float4 positions, 4 x ubyte4 colours.
    const float pos[4][2] = {{-1, -1}, {0, -1}, {0, 1}, {-1, 1}};
    for (u32 v = 0; v < 4; ++v) {
        mem::w32(kVkIo + 0x40000 + 16 * v + 0, fbits(pos[v][0]));
        mem::w32(kVkIo + 0x40000 + 16 * v + 4, fbits(pos[v][1]));
        mem::w32(kVkIo + 0x40000 + 16 * v + 8, fbits(0.5f));
        mem::w32(kVkIo + 0x40000 + 16 * v + 12, fbits(1.0f));
        mem::w32(kVkIo + 0x41000 + 4 * v, 0xFF0000FFu);  // r=255 g=0 b=0 a=255
    }
    // Fragment program at local 0x8000: MOVR R0, f[COL0]  END
    const u32 fdst = 1u | (0u << 1) | (0xFu << 9) | (1u << 13) | (1u << 24);
    const u32 fsrc0 = 1u | (1u << 11) | (2u << 13) | (3u << 15) | (7u << 18) | (1u << 23) | (2u << 25) | (3u << 27);
    fp_word(kVkLocal + 0x8000, fdst);
    fp_word(kVkLocal + 0x8004, fsrc0);
    fp_word(kVkLocal + 0x8008, 0);
    fp_word(kVkLocal + 0x800C, 0);

    // Vertex program: MOV o[HPOS], v0 ; MOV o[COL0], v3 END
    auto vp_mov = [](u32 input, u32 out, bool end) {
        const u32 src = 2u | (3u << 8) | (2u << 10) | (1u << 12);  // input, .xyzw
        u32 d0 = (0x3Fu << 15) | (1u << 30);
        u32 d1 = (1u << 22) | (input << 8) | (src >> 9);
        u32 d2 = (src & 0x1FF) << 23;
        u32 d3 = (end ? 1u : 0u) | (out << 2) | (0x3Fu << 7) | (0xFu << 13);
        return std::array<u32, 4>{d0, d1, d2, d3};
    };
    const auto i0 = vp_mov(0, 0, false), i1 = vp_mov(3, 1, true);

    W w;
    w.m(rsx::NV4097_SET_SURFACE_FORMAT, {(6u << 24) | (6u << 16) | (1u << 8) | 8u});  // pitch, A8R8G8B8, no depth
    w.m(rsx::NV4097_SET_SURFACE_CLIP_HORIZONTAL, {Wd << 16, Ht << 16});
    w.m(rsx::NV4097_SET_SURFACE_PITCH_A, {kPitch, kColor});
    w.m(rsx::NV4097_SET_CONTEXT_DMA_COLOR_A, {rsx::CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER});
    w.m(rsx::NV4097_SET_SURFACE_COLOR_TARGET, {1});
    w.m(0x0324, {0x01010101});  // colour mask: all channels
    w.m(0x1D60, {0x40});        // shader control: R exports
    w.f(0x0A20, {Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f, Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f});  // viewport offset, scale
    w.m(rsx::NV4097_SET_COLOR_CLEAR_VALUE, {0xFF102030});
    w.m(rsx::NV4097_CLEAR_SURFACE, {0xF0});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_LOAD, {0});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM, {i0[0], i0[1], i0[2], i0[3], i1[0], i1[1], i1[2], i1[3]});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_START, {0});
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x8000 | 1});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 0, {(16u << 8) | (4u << 4) | 2u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 12, {(4u << 8) | (4u << 4) | 4u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 0, {0x80000000u | 0x40000});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 12, {0x80000000u | 0x41000});
    w.m(rsx::NV4097_SET_BEGIN_END, {8}).m(rsx::NV4097_DRAW_ARRAYS, {3u << 24}).m(rsx::NV4097_SET_BEGIN_END, {0});
    w.m(rsx::GCM_FLIP_COMMAND, {0});

    mem::w32(kVkCtl + 4, 0);
    mem::w32(kVkCtl, w.at);
    rsx::Fifo fifo(d, kVkCtl);
    CHECK(fifo.run_until_idle([](u32 io) { return kVkIo + io; }));

    CHECK_EQ(vk->stats().draws, 1u);
    CHECK_EQ(vk->stats().clears, 1u);
    CHECK_EQ(vk->stats().shader_failures, 0u);
    auto px = [&](u32 x, u32 y) { return mem::r32(kVkLocal + kColor + y * kPitch + x * 4); };
    std::printf("  left 0x%08X  right 0x%08X\n", px(8, 32), px(48, 32));
    CHECK_EQ(px(8, 32), 0xFFFF0000u);   // drawn: opaque red (A R G B)
    CHECK_EQ(px(8, 2), 0xFFFF0000u);
    CHECK_EQ(px(48, 32), 0xFF102030u);  // cleared
    CHECK_EQ(px(63, 63), 0xFF102030u);
}

TEST(VulkanTextureAndDepth) {
    if (!rsx::have_vulkan()) return;
    std::string err;
    auto vk = rsx::create_vulkan_backend([](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; }, &err);
    if (!vk) return;
    mem::commit(kVkIo, 0x100000);
    mem::commit(kVkLocal, 0x100000);
    mem::commit(kVkCtl, 0x1000);
    rsx::Environment env;
    env.to_ea = [](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; };
    env.label_base = kVkCtl + 0x100;
    rsx::Decoder d(env, vk.get());

    constexpr u32 Wd = 64, Ht = 64, kColor = 0x10000, kDepth = 0x30000, kPitch = Wd * 4, kTex = 0x20000;
    // 4x4 swizzled A8R8G8B8 texture: texel (x, y) = A=FF R=60x G=60y B=0
    auto swz = [](u32 x, u32 y) { return (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2); };
    for (u32 y = 0; y < 4; ++y)
        for (u32 x = 0; x < 4; ++x) mem::w32(kVkLocal + kTex + 4 * swz(x, y), 0xFF000000u | (60 * x) << 16 | (60 * y) << 8);
    // FP: TEXR R0, f[TEX0], TEX0  END
    const u32 fdst = 1u | (0xFu << 9) | (4u << 13) | (0u << 17) | (0x17u << 24);
    const u32 fsrc0 = 1u | (1u << 11) | (2u << 13) | (3u << 15) | (7u << 18) | (1u << 23) | (2u << 25) | (3u << 27);
    fp_word(kVkLocal + 0x8000, fdst);
    fp_word(kVkLocal + 0x8004, fsrc0);
    fp_word(kVkLocal + 0x8008, 0);
    fp_word(kVkLocal + 0x800C, 0);
    auto vp_mov = [](u32 input, u32 out, bool end) {
        const u32 src = 2u | (3u << 8) | (2u << 10) | (1u << 12);
        return std::array<u32, 4>{(0x3Fu << 15) | (1u << 30), (1u << 22) | (input << 8) | (src >> 9),
                                  (src & 0x1FF) << 23, (end ? 1u : 0u) | (out << 2) | (0x3Fu << 7) | (0xFu << 13)};
    };
    const auto i0 = vp_mov(0, 0, false), i1 = vp_mov(8, 7, true);

    // Three quads: (x0, x1, clip z, u, v)
    struct Q { float x0, x1, z, u0, v0, u1, v1; };
    const Q quads[3] = {{-1, 1, 0.0f, 0, 0, 1, 1},             // full screen, depth 0.5
                        {-1, 0, 0.8f, 0.8f, 0.1f, 0.8f, 0.1f},  // behind: must not show
                        {0, 1, -0.8f, 0.1f, 0.8f, 0.1f, 0.8f}}; // in front: right half texel (0,3)
    for (u32 q = 0; q < 3; ++q) {
        const float px[4] = {quads[q].x0, quads[q].x1, quads[q].x1, quads[q].x0}, py[4] = {-1, -1, 1, 1};
        const float uu[4] = {quads[q].u0, quads[q].u1, quads[q].u1, quads[q].u0};
        const float vv[4] = {quads[q].v0, quads[q].v0, quads[q].v1, quads[q].v1};
        for (u32 v = 0; v < 4; ++v) {
            const u32 b = kVkIo + 0x40000 + q * 0x100 + 16 * v;
            mem::w32(b, fbits(px[v])); mem::w32(b + 4, fbits(py[v])); mem::w32(b + 8, fbits(quads[q].z)); mem::w32(b + 12, fbits(1));
            mem::w32(kVkIo + 0x42000 + q * 0x100 + 8 * v, fbits(uu[v]));
            mem::w32(kVkIo + 0x42004 + q * 0x100 + 8 * v, fbits(vv[v]));
        }
    }
    W w;
    w.m(rsx::NV4097_SET_SURFACE_FORMAT, {(6u << 24) | (6u << 16) | (1u << 8) | (2u << 5) | 8u});  // Z24S8
    w.m(rsx::NV4097_SET_SURFACE_CLIP_HORIZONTAL, {Wd << 16, Ht << 16});
    w.m(rsx::NV4097_SET_SURFACE_PITCH_A, {kPitch, kColor, kDepth});
    w.m(rsx::NV4097_SET_SURFACE_PITCH_Z, {kPitch});
    w.m(rsx::NV4097_SET_SURFACE_COLOR_TARGET, {1});
    w.m(0x0324, {0x01010101});
    w.m(0x1D60, {0x40});
    w.f(0x0A20, {Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f, Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f});
    w.m(0x0A6C, {0x201, 1, 1});  // depth func LESS, mask, test enable
    w.m(rsx::NV4097_SET_ZSTENCIL_CLEAR_VALUE, {0xFFFFFF00u, 0xFF000000u});
    w.m(rsx::NV4097_CLEAR_SURFACE, {0xF3});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_LOAD, {0});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM, {i0[0], i0[1], i0[2], i0[3], i1[0], i1[1], i1[2], i1[3]});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_START, {0});
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x8000 | 1});
    // texture unit 0: offset, format, address, control0, control1, filter, image rect
    w.m(rsx::NV4097_SET_TEXTURE_OFFSET, {kTex, 1u | (2u << 4) | (0x85u << 8) | (1u << 16), 3u | (3u << 8) | (3u << 16),
                                         0x80000000u | (0xFFFu << 7), 0xAAE4, (1u << 16) | (1u << 24), (4u << 16) | 4u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 0, {(16u << 8) | (4u << 4) | 2u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 32, {(8u << 8) | (2u << 4) | 2u});
    for (u32 q = 0; q < 3; ++q) {
        w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 0, {0x80000000u | (0x40000 + q * 0x100)});
        w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 32, {0x80000000u | (0x42000 + q * 0x100)});
        w.m(rsx::NV4097_SET_BEGIN_END, {8}).m(rsx::NV4097_DRAW_ARRAYS, {3u << 24}).m(rsx::NV4097_SET_BEGIN_END, {0});
    }
    w.m(rsx::GCM_FLIP_COMMAND, {0});
    mem::w32(kVkCtl + 4, 0);
    mem::w32(kVkCtl, w.at);
    rsx::Fifo fifo(d, kVkCtl);
    CHECK(fifo.run_until_idle([](u32 io) { return kVkIo + io; }));

    CHECK_EQ(vk->stats().draws, 3u);
    CHECK_EQ(vk->stats().shader_failures, 0u);
    CHECK_EQ(vk->stats().textures, 1u);
    auto px = [&](u32 x, u32 y) { return mem::r32(kVkLocal + kColor + y * kPitch + x * 4); };
    std::printf("  (8,8) 0x%08X  (24,56) 0x%08X  (48,8) 0x%08X  depth 0x%08X\n", px(8, 8), px(24, 56), px(48, 8),
                mem::r32(kVkLocal + kDepth + 8 * kPitch + 48 * 4));
    CHECK_EQ(px(8, 8), 0xFF000000u);    // texel (0,0)
    CHECK_EQ(px(24, 56), 0xFF3CB400u);  // texel (1,3): the far quad stayed hidden
    CHECK_EQ(px(48, 8), 0xFF00B400u);   // near quad: texel (0,3)
    const u32 dz = mem::r32(kVkLocal + kDepth + 8 * kPitch + 48 * 4);
    CHECK((dz >> 8) > 0x180000u && (dz >> 8) < 0x200000u);  // ~0.1 * 2^24
    CHECK_EQ(mem::r32(kVkLocal + kDepth + 8 * kPitch + 8 * 4) >> 8, 0x800000u);  // 0.5 where only quad 0 drew
}

TEST(VulkanRenderToTexture) {
    if (!rsx::have_vulkan()) return;
    std::string err;
    auto vk = rsx::create_vulkan_backend([](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; }, &err);
    if (!vk) return;
    mem::commit(kVkIo, 0x100000);
    mem::commit(kVkLocal, 0x100000);
    mem::commit(kVkCtl, 0x1000);
    rsx::Environment env;
    env.to_ea = [](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; };
    env.label_base = kVkCtl + 0x100;
    rsx::Decoder d(env, vk.get());

    constexpr u32 Wd = 64, Ht = 64, kScreen = 0x10000, kOff = 0x50000, kPitch = Wd * 4;
    const u32 fsrc_in = 1u | (1u << 11) | (2u << 13) | (3u << 15) | (7u << 18) | (1u << 23) | (2u << 25) | (3u << 27);
    // FP at 0x8000: MOV R0, f[COL0];  FP at 0x8100: TEX R0, f[TEX0], TEX0
    const u32 mov = 1u | (0xFu << 9) | (1u << 13) | (1u << 24), tex = 1u | (0xFu << 9) | (4u << 13) | (0x17u << 24);
    for (u32 k = 0; k < 4; ++k) {
        fp_word(kVkLocal + 0x8000 + 4 * k, k == 0 ? mov : k == 1 ? fsrc_in : 0);
        fp_word(kVkLocal + 0x8100 + 4 * k, k == 0 ? tex : k == 1 ? fsrc_in : 0);
    }
    auto vp_mov = [](u32 input, u32 out, bool end) {
        const u32 src = 2u | (3u << 8) | (2u << 10) | (1u << 12);
        return std::array<u32, 4>{(0x3Fu << 15) | (1u << 30), (1u << 22) | (input << 8) | (src >> 9),
                                  (src & 0x1FF) << 23, (end ? 1u : 0u) | (out << 2) | (0x3Fu << 7) | (0xFu << 13)};
    };
    const auto p0 = vp_mov(0, 0, false), p1 = vp_mov(3, 1, false), p2 = vp_mov(8, 7, true);
    // Full-screen quad: position, colour (green), uv
    const float px[4] = {-1, 1, 1, -1}, py[4] = {-1, -1, 1, 1}, uu[4] = {0, 1, 1, 0}, vv[4] = {0, 0, 1, 1};
    for (u32 v = 0; v < 4; ++v) {
        const u32 b = kVkIo + 0x40000 + 16 * v;
        mem::w32(b, fbits(px[v])); mem::w32(b + 4, fbits(py[v])); mem::w32(b + 8, fbits(0)); mem::w32(b + 12, fbits(1));
        mem::w32(kVkIo + 0x41000 + 4 * v, 0x00FF00FFu);  // r=0 g=255 b=0 a=255
        mem::w32(kVkIo + 0x42000 + 8 * v, fbits(uu[v]));
        mem::w32(kVkIo + 0x42004 + 8 * v, fbits(vv[v]));
    }
    W w;
    auto surface = [&](u32 offset) {
        w.m(rsx::NV4097_SET_SURFACE_FORMAT, {(6u << 24) | (6u << 16) | (1u << 8) | 8u});
        w.m(rsx::NV4097_SET_SURFACE_CLIP_HORIZONTAL, {Wd << 16, Ht << 16});
        w.m(rsx::NV4097_SET_SURFACE_PITCH_A, {kPitch, offset});
        w.m(rsx::NV4097_SET_SURFACE_COLOR_TARGET, {1});
    };
    w.m(0x0324, {0x01010101});
    w.m(0x1D60, {0x40});
    w.f(0x0A20, {Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f, Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_LOAD, {0});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM, {p0[0], p0[1], p0[2], p0[3], p1[0], p1[1], p1[2], p1[3], p2[0], p2[1], p2[2], p2[3]});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_START, {0});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 0, {(16u << 8) | (4u << 4) | 2u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 12, {(4u << 8) | (4u << 4) | 4u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 32, {(8u << 8) | (2u << 4) | 2u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 0, {0x80000000u | 0x40000});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 12, {0x80000000u | 0x41000});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 32, {0x80000000u | 0x42000});
    // Pass 1: green into the offscreen target
    surface(kOff);
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x8000 | 1});
    w.m(rsx::NV4097_SET_BEGIN_END, {8}).m(rsx::NV4097_DRAW_ARRAYS, {3u << 24}).m(rsx::NV4097_SET_BEGIN_END, {0});
    // Pass 2: sample it onto the screen (linear A8R8G8B8, pitch 256)
    surface(kScreen);
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x8100 | 1});
    w.m(rsx::NV4097_SET_TEXTURE_OFFSET, {kOff, 1u | (2u << 4) | ((0x85u | 0x20u) << 8) | (1u << 16), 3u | (3u << 8) | (3u << 16),
                                         0x80000000u | (0xFFFu << 7), 0xAAE4, (1u << 16) | (1u << 24), (Wd << 16) | Ht});
    w.m(0x1840, {kPitch});  // TEXTURE_CONTROL3: pitch
    w.m(rsx::NV4097_SET_BEGIN_END, {8}).m(rsx::NV4097_DRAW_ARRAYS, {3u << 24}).m(rsx::NV4097_SET_BEGIN_END, {0});
    w.m(rsx::GCM_FLIP_COMMAND, {0});
    mem::w32(kVkCtl + 4, 0);
    mem::w32(kVkCtl, w.at);
    rsx::Fifo fifo(d, kVkCtl);
    CHECK(fifo.run_until_idle([](u32 io) { return kVkIo + io; }));
    CHECK_EQ(vk->stats().draws, 2u);
    CHECK_EQ(vk->stats().rt_samples, 1u);  // sampled straight from the GPU target
    CHECK_EQ(vk->stats().textures, 0u);    // nothing decoded from guest memory
    CHECK_EQ(vk->stats().flushes, 1u);     // the only write-back is at the flip
    CHECK_EQ(mem::r32(kVkLocal + kScreen + 32 * kPitch + 32 * 4), 0xFF00FF00u);
    CHECK_EQ(mem::r32(kVkLocal + kOff + 5 * kPitch + 5 * 4), 0xFF00FF00u);  // and the offscreen target reached memory at flip
}

TEST(VulkanShadowMapCompare) {
    if (!rsx::have_vulkan()) return;
    std::string err;
    auto vk = rsx::create_vulkan_backend([](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; }, &err);
    if (!vk) return;
    mem::commit(kVkIo, 0x100000);
    mem::commit(kVkLocal, 0x100000);
    mem::commit(kVkCtl, 0x1000);
    rsx::Environment env;
    env.to_ea = [](u32 off, bool local) { return (local ? kVkLocal : kVkIo) + off; };
    env.label_base = kVkCtl + 0x100;
    rsx::Decoder d(env, vk.get());

    constexpr u32 Wd = 64, Ht = 64, kScreen = 0x10000, kShadow = 0x60000, kPitch = Wd * 4;
    const u32 fsrc_in = 1u | (1u << 11) | (2u << 13) | (3u << 15) | (7u << 18) | (1u << 23) | (2u << 25) | (3u << 27);
    const u32 mov = 1u | (0xFu << 9) | (1u << 13) | (1u << 24), tex = 1u | (0xFu << 9) | (4u << 13) | (0x17u << 24);
    for (u32 k = 0; k < 4; ++k) {
        fp_word(kVkLocal + 0x8000 + 4 * k, k == 0 ? mov : k == 1 ? fsrc_in : 0);
        fp_word(kVkLocal + 0x8100 + 4 * k, k == 0 ? tex : k == 1 ? fsrc_in : 0);
    }
    auto vp_mov = [](u32 input, u32 out, bool end) {
        const u32 src = 2u | (3u << 8) | (2u << 10) | (1u << 12);
        return std::array<u32, 4>{(0x3Fu << 15) | (1u << 30), (1u << 22) | (input << 8) | (src >> 9),
                                  (src & 0x1FF) << 23, (end ? 1u : 0u) | (out << 2) | (0x3Fu << 7) | (0xFu << 13)};
    };
    const auto p0 = vp_mov(0, 0, false), p1 = vp_mov(8, 7, true);
    // Quad A: left half at clip z -0.4 (depth 0.3). Quad B: full screen, uv + reference 0.5 in tex0.z
    auto quad = [&](u32 at, float x0, float x1, float z) {
        const float px[4] = {x0, x1, x1, x0}, py[4] = {-1, -1, 1, 1}, uu[4] = {0, 1, 1, 0}, vv[4] = {0, 0, 1, 1};
        for (u32 v = 0; v < 4; ++v) {
            const u32 b = kVkIo + at + 16 * v;
            mem::w32(b, fbits(px[v])); mem::w32(b + 4, fbits(py[v])); mem::w32(b + 8, fbits(z)); mem::w32(b + 12, fbits(1));
            const u32 t = kVkIo + at + 0x100 + 16 * v;
            mem::w32(t, fbits(uu[v])); mem::w32(t + 4, fbits(vv[v])); mem::w32(t + 8, fbits(0.5f)); mem::w32(t + 12, fbits(1));
        }
    };
    quad(0x40000, -1, 0, -0.4f);
    quad(0x41000, -1, 1, 0.0f);
    W w;
    w.m(0x0324, {0x01010101});
    w.m(0x1D60, {0x40});
    w.f(0x0A20, {Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f, Wd / 2.0f, Ht / 2.0f, 0.5f, 0.0f});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_LOAD, {0});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM, {p0[0], p0[1], p0[2], p0[3], p1[0], p1[1], p1[2], p1[3]});
    w.m(rsx::NV4097_SET_TRANSFORM_PROGRAM_START, {0});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 0, {(16u << 8) | (4u << 4) | 2u});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 32, {(16u << 8) | (4u << 4) | 2u});
    // Pass 1: depth only (no colour target), Z24S8 at kShadow
    w.m(rsx::NV4097_SET_SURFACE_FORMAT, {(6u << 24) | (6u << 16) | (1u << 8) | (2u << 5) | 8u});
    w.m(rsx::NV4097_SET_SURFACE_CLIP_HORIZONTAL, {Wd << 16, Ht << 16});
    w.m(rsx::NV4097_SET_SURFACE_ZETA_OFFSET, {kShadow});
    w.m(rsx::NV4097_SET_SURFACE_PITCH_Z, {kPitch});
    w.m(rsx::NV4097_SET_SURFACE_COLOR_TARGET, {0});
    w.m(0x0A6C, {0x201, 1, 1});  // depth LESS, write, test
    w.m(rsx::NV4097_SET_ZSTENCIL_CLEAR_VALUE, {0xFFFFFF00u});
    w.m(rsx::NV4097_CLEAR_SURFACE, {0x03});
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x8000 | 1});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 0, {0x80000000u | 0x40000});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 32, {0x80000000u | 0x40100});
    w.m(rsx::NV4097_SET_BEGIN_END, {8}).m(rsx::NV4097_DRAW_ARRAYS, {3u << 24}).m(rsx::NV4097_SET_BEGIN_END, {0});
    // Pass 2: colour only, sample the shadow map with a LESS compare
    w.m(rsx::NV4097_SET_SURFACE_FORMAT, {(6u << 24) | (6u << 16) | (1u << 8) | 8u});
    w.m(rsx::NV4097_SET_SURFACE_PITCH_A, {kPitch, kScreen});
    w.m(rsx::NV4097_SET_SURFACE_COLOR_TARGET, {1});
    w.m(0x0A74, {0});  // depth test off
    w.m(rsx::NV4097_SET_SHADER_PROGRAM, {0x8100 | 1});
    w.m(rsx::NV4097_SET_TEXTURE_OFFSET, {kShadow, 1u | (2u << 4) | ((0x90u | 0x20u) << 8) | (1u << 16),
                                         3u | (3u << 8) | (3u << 16) | (1u << 28),  // clamp, zfunc LESS
                                         0x80000000u | (0xFFFu << 7), 0xAAE4, (1u << 16) | (1u << 24), (Wd << 16) | Ht});
    w.m(0x1840, {kPitch});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 0, {0x80000000u | 0x41000});
    w.m(rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 32, {0x80000000u | 0x41100});
    w.m(rsx::NV4097_SET_BEGIN_END, {8}).m(rsx::NV4097_DRAW_ARRAYS, {3u << 24}).m(rsx::NV4097_SET_BEGIN_END, {0});
    w.m(rsx::GCM_FLIP_COMMAND, {0});
    mem::w32(kVkCtl + 4, 0);
    mem::w32(kVkCtl, w.at);
    rsx::Fifo fifo(d, kVkCtl);
    CHECK(fifo.run_until_idle([](u32 io) { return kVkIo + io; }));
    CHECK_EQ(vk->stats().shader_failures, 0u);
    CHECK_EQ(vk->stats().rt_samples, 1u);
    const u32 left = mem::r32(kVkLocal + kScreen + 32 * kPitch + 8 * 4), right = mem::r32(kVkLocal + kScreen + 32 * kPitch + 56 * 4);
    std::printf("  shadow: left 0x%08X right 0x%08X\n", left, right);
    CHECK_EQ(left, 0x00000000u);   // 0.5 < 0.3 fails: in shadow
    CHECK_EQ(right, 0xFFFFFFFFu);  // 0.5 < 1.0 passes: lit
}
