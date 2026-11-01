// RSX CPU-side helpers: texture decoding and draw assembly.
#include <cstring>
#include <vector>

#include "../runtime/src/rsx/geometry.h"
#include "../runtime/src/rsx/texture.h"
#include "opentit/memory.h"
#include "test.h"

using namespace opentit;

TEST(RsxTextureSwizzleOffset) {
    // 4x4: x bits interleave below y bits
    CHECK_EQ(rsx::swizzle_offset(1, 0, 0, 2, 2, 0), 1u);
    CHECK_EQ(rsx::swizzle_offset(0, 1, 0, 2, 2, 0), 2u);
    CHECK_EQ(rsx::swizzle_offset(2, 0, 0, 2, 2, 0), 4u);
    CHECK_EQ(rsx::swizzle_offset(3, 3, 0, 2, 2, 0), 15u);
    // 8x2: once y runs out, x bits continue linearly
    CHECK_EQ(rsx::swizzle_offset(0, 1, 0, 3, 1, 0), 2u);
    CHECK_EQ(rsx::swizzle_offset(4, 0, 0, 3, 1, 0), 8u);
    CHECK_EQ(rsx::swizzle_offset(7, 1, 0, 3, 1, 0), 15u);
}

TEST(RsxTextureArgbLinearAndSwizzled) {
    // 2x2 A8R8G8B8, texel k = (A=0x80+k, R=k*10, G=k*20, B=k*30)
    u8 lin[16], swz[16];
    for (u32 k = 0; k < 4; ++k) {
        const u8 t[4] = {static_cast<u8>(0x80 + k), static_cast<u8>(k * 10), static_cast<u8>(k * 20), static_cast<u8>(k * 30)};
        std::memcpy(lin + 4 * k, t, 4);
        const u32 x = k % 2, y = k / 2;
        std::memcpy(swz + 4 * rsx::swizzle_offset(x, y, 0, 1, 1, 0), t, 4);
    }
    rsx::TextureDesc d;
    d.format = rsx::TEX_A8R8G8B8 | rsx::TEX_LN;
    d.width = d.height = 2;
    d.pitch = 8;
    const auto a = rsx::decode_texture(d, lin);
    d.format = rsx::TEX_A8R8G8B8;
    const auto b = rsx::decode_texture(d, swz);
    CHECK_EQ(a.size(), std::size_t(1));
    CHECK(a[0].rgba == b[0].rgba);
    CHECK_EQ(a[0].rgba[3 * 4 + 0], 30);    // R of texel 3
    CHECK_EQ(a[0].rgba[3 * 4 + 1], 60);    // G
    CHECK_EQ(a[0].rgba[3 * 4 + 2], 90);    // B
    CHECK_EQ(a[0].rgba[3 * 4 + 3], 0x83);  // A
    // remap: force alpha to one, swap R and B sources
    d.format = rsx::TEX_A8R8G8B8 | rsx::TEX_LN;
    d.remap = (1u << 8) | (2u << 10) | (2u << 12) | (2u << 14) | (0u << 0) | (3u << 2) | (2u << 4) | (1u << 6);
    const auto c = rsx::decode_texture(d, lin);
    CHECK_EQ(c[0].rgba[3 * 4 + 0], 90);
    CHECK_EQ(c[0].rgba[3 * 4 + 2], 30);
    CHECK_EQ(c[0].rgba[3 * 4 + 3], 255);
}

TEST(RsxTextureFormatsAndMips) {
    // R5G6B5 white/red, 2x1 linear, with a 1x1 mip
    u8 px[6];
    store_be<u16>(px, 0xFFFF);
    store_be<u16>(px + 2, 0xF800);
    store_be<u16>(px + 4, 0x001F);
    rsx::TextureDesc d;
    d.format = rsx::TEX_R5G6B5 | rsx::TEX_LN;
    d.width = 2;
    d.height = 1;
    d.mipmaps = 2;
    CHECK_EQ(rsx::texture_size(d), 6u);
    const auto l = rsx::decode_texture(d, px);
    CHECK_EQ(l.size(), std::size_t(2));
    CHECK_EQ(l[0].rgba[0], 255);
    CHECK_EQ(l[0].rgba[4], 255);
    CHECK_EQ(l[0].rgba[5], 0);
    CHECK_EQ(l[1].width, 1u);
    CHECK_EQ(l[1].rgba[2], 255);  // the mip is pure blue
    CHECK_EQ(l[1].rgba[0], 0);

    // Cubemap of 1x1 B8: faces 128-byte aligned
    std::vector<u8> cube(6 * 128, 0);
    for (int f = 0; f < 6; ++f) cube[f * 128] = static_cast<u8>(40 * f);
    rsx::TextureDesc cd;
    cd.format = rsx::TEX_B8;
    cd.cubemap = true;
    CHECK_EQ(rsx::texture_size(cd), 6u * 128u);
    const auto faces = rsx::decode_texture(cd, cube.data());
    CHECK_EQ(faces.size(), std::size_t(6));
    CHECK_EQ(faces[5].rgba[0], 200);
}

TEST(RsxTextureDxt) {
    // DXT1 block: c0 = red (0xF800), c1 = blue (0x001F), all texels index 0 except texel 1 = index 1
    u8 b1[8] = {0x00, 0xF8, 0x1F, 0x00, 0x04, 0x00, 0x00, 0x00};
    rsx::TextureDesc d;
    d.format = rsx::TEX_DXT1;
    d.width = d.height = 4;
    auto l = rsx::decode_texture(d, b1);
    CHECK_EQ(l[0].rgba[0], 255);
    CHECK_EQ(l[0].rgba[2], 0);
    CHECK_EQ(l[0].rgba[4 + 2], 255);  // texel 1 is blue
    CHECK_EQ(l[0].rgba[3], 255);
    // DXT5: alpha endpoints 255/0, texel 0 index 1 (= 0), others 0 (= 255)
    u8 b5[16] = {255, 0, 0x01, 0, 0, 0, 0, 0, 0x00, 0xF8, 0x1F, 0x00, 0, 0, 0, 0};
    d.format = rsx::TEX_DXT45;
    l = rsx::decode_texture(d, b5);
    CHECK_EQ(l[0].rgba[3], 0);
    CHECK_EQ(l[0].rgba[4 + 3], 255);
    CHECK_EQ(l[0].rgba[0], 255);
}

namespace {
constexpr u32 kGeo = 0x5C000000;
rsx::ToEa geo_ea() {
    return [](u32 off, bool) { return kGeo + off; };
}
void set_reg(rsx::State& s, u32 reg, u32 v) { s.regs[reg >> 2] = v; }
}  // namespace

TEST(RsxGeometryAttributes) {
    float f;
    u8 b[4];
    f = 1.5f; std::memcpy(b, &f, 4);
    u32 fb; std::memcpy(&fb, &f, 4);
    CHECK_EQ(rsx::fetch_attribute(2, 1, (store_be<u32>(b, fb), b))[0], 1.5f);
    u8 ub[4] = {255, 0, 51, 255};
    const auto u = rsx::fetch_attribute(4, 3, ub);
    CHECK_EQ(u[0], 1.0f);
    CHECK_EQ(u[2], 0.2f);
    CHECK_EQ(u[3], 1.0f);  // missing w defaults to 1
    u8 h[2];
    store_be<u16>(h, 0x3C00);  // half 1.0
    CHECK_EQ(rsx::fetch_attribute(3, 1, h)[0], 1.0f);
    u8 cmp[4];
    store_be<u32>(cmp, (511u << 22) | (0x401u << 11) | 1023u);  // z=+1, y=-1, x=+1
    const auto c = rsx::fetch_attribute(6, 1, cmp);
    CHECK_EQ(c[0], 1.0f);
    CHECK_EQ(c[1], -1.0f);
    CHECK_EQ(c[2], 1.0f);
}

TEST(RsxGeometryQuadsIndexed) {
    mem::commit(kGeo, 0x10000);
    // 6 vertices of float2 position at offset 0x100, stride 8
    for (u32 v = 0; v < 6; ++v) {
        float xy[2] = {static_cast<float>(v), static_cast<float>(10 * v)};
        u32 w[2];
        std::memcpy(w, xy, 8);
        mem::w32(kGeo + 0x100 + 8 * v, w[0]);
        mem::w32(kGeo + 0x104 + 8 * v, w[1]);
    }
    rsx::State s;
    set_reg(s, rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, (8u << 8) | (2u << 4) | 2u);
    set_reg(s, rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 0x100);
    // indexed u16: 2 3 4 5 (one quad using vertices 2..5)
    const u16 idx[4] = {2, 3, 4, 5};
    for (int k = 0; k < 4; ++k) mem::w16(kGeo + 0x800 + 2 * k, idx[k]);
    rsx::DrawCall c;
    c.prim = rsx::Prim::Quads;
    c.indexed = true;
    c.index16 = true;
    c.index_offset = 0x800;
    c.ranges.emplace_back(0, 4);
    const rsx::Geometry g = rsx::assemble(s, c, geo_ea(), 1u);
    CHECK(g.topology == rsx::Topology::TriangleList);
    CHECK_EQ(g.vertex_count, 4u);  // only 2..5 fetched
    CHECK_EQ(g.attribute_mask, 1u);
    const std::vector<u32> want = {0, 1, 2, 2, 3, 0};
    CHECK(g.indices == want);
    CHECK_EQ(g.attributes[0][0], 2.0f);   // relative vertex 0 = absolute 2
    CHECK_EQ(g.attributes[0][13], 50.0f); // vertex 3 (abs 5) y
    CHECK_EQ(g.attributes[0][3], 1.0f);   // w default
}

TEST(RsxGeometryRangesRestartAndInline) {
    mem::commit(kGeo, 0x10000);
    rsx::State s;
    set_reg(s, rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, (4u << 8) | (1u << 4) | 2u);
    set_reg(s, rsx::NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 0x2000);
    rsx::DrawCall c;
    c.prim = rsx::Prim::TriangleStrip;
    c.ranges = {{10, 4}, {20, 3}};
    const rsx::Geometry g = rsx::assemble(s, c, geo_ea(), 1u);
    CHECK_EQ(g.vertex_count, 13u);  // 10..22
    CHECK_EQ(g.batches.size(), std::size_t(2));
    CHECK_EQ(g.batches[1].first, 4u);
    CHECK_EQ(g.indices[4], 10u);  // 20 - 10

    // Line loop closes, and restart splits segments.
    for (int k = 0; k < 5; ++k) mem::w32(kGeo + 0x900 + 4 * k, k == 2 ? 0xFFFFFFFFu : static_cast<u32>(k));
    set_reg(s, 0x1DAC, 1);
    set_reg(s, 0x1DB0, 0xFFFFFFFF);
    rsx::DrawCall l;
    l.prim = rsx::Prim::LineLoop;
    l.indexed = true;
    l.index_offset = 0x900;
    l.ranges.emplace_back(0, 5);
    const rsx::Geometry lg = rsx::assemble(s, l, geo_ea(), 1u);
    const std::vector<u32> want = {0, 1, 0, 0xFFFFFFFFu, 3, 4, 3};
    CHECK(lg.indices == want);

    // Inline array: two enabled float1 attributes, three vertices
    rsx::State si;
    set_reg(si, rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 0, (0u << 8) | (1u << 4) | 2u);
    set_reg(si, rsx::NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 8, (0u << 8) | (1u << 4) | 2u);
    rsx::DrawCall ic;
    ic.prim = rsx::Prim::Triangles;
    for (float v : {1.f, 2.f, 3.f, 4.f, 5.f, 6.f}) {
        u32 w;
        std::memcpy(&w, &v, 4);
        ic.inline_vertices.push_back(w);
    }
    const rsx::Geometry ig = rsx::assemble(si, ic, geo_ea(), (1u << 0) | (1u << 2));
    CHECK_EQ(ig.vertex_count, 3u);
    CHECK_EQ(ig.attribute_mask, 5u);
    CHECK_EQ(ig.attributes[2][4 * 2], 6.0f);
    CHECK_EQ(ig.indices.size(), std::size_t(3));
    CHECK_EQ(ig.constant_attributes[1][3], 1.0f);
}
