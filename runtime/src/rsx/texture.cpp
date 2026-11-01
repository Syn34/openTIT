#include "texture.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace opentit::rsx {

namespace {

u32 log2u(u32 v) {
    u32 r = 0;
    while ((1u << r) < v) ++r;
    return r;
}

float half_to_float(u16 h) {
    const u32 sign = (h >> 15) & 1, exp = (h >> 10) & 0x1F, man = h & 0x3FF;
    float f;
    if (exp == 0) f = std::ldexp(static_cast<float>(man), -24);
    else if (exp == 31) f = man ? NAN : INFINITY;
    else f = std::ldexp(static_cast<float>(man | 0x400), static_cast<int>(exp) - 25);
    return sign ? -f : f;
}

u8 unorm(float f) {
    if (!(f > 0.0f)) return 0;  // also NaN
    if (f >= 1.0f) return 255;
    return static_cast<u8>(f * 255.0f + 0.5f);
}

u8 expand(u32 v, u32 bits) {
    if (bits >= 8) return static_cast<u8>(v >> (bits - 8));
    return static_cast<u8>((v * 255 + ((1u << bits) - 1) / 2) / ((1u << bits) - 1));
}

float as_f32(u32 bits) {
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

// Apply TEXTURE_CONTROL1 remap to inputs in A,R,G,B order; write R,G,B,A.
void remap_store(u8* out, const u8 in[4], u32 remap) {
    u8 argb[4];
    for (int o = 0; o < 4; ++o) {
        const u32 sel = (remap >> (2 * o)) & 3;
        const u32 ctl = (remap >> (8 + 2 * o)) & 3;
        argb[o] = ctl == 0 ? 0 : ctl == 1 ? 255 : in[sel];
    }
    out[0] = argb[1];
    out[1] = argb[2];
    out[2] = argb[3];
    out[3] = argb[0];
}

// Decode one texel at `p` into A,R,G,B.
void texel(u32 fmt, const u8* p, u8 argb[4]) {
    switch (fmt) {
    case TEX_B8: argb[0] = argb[1] = argb[2] = argb[3] = p[0]; return;
    case TEX_A1R5G5B5: case TEX_D1R5G5B5: {
        const u32 v = load_be<u16>(p);
        argb[0] = fmt == TEX_D1R5G5B5 ? 255 : (v >> 15) ? 255 : 0;
        argb[1] = expand((v >> 10) & 31, 5);
        argb[2] = expand((v >> 5) & 31, 5);
        argb[3] = expand(v & 31, 5);
        return;
    }
    case TEX_A4R4G4B4: {
        const u32 v = load_be<u16>(p);
        argb[0] = expand(v >> 12, 4);
        argb[1] = expand((v >> 8) & 15, 4);
        argb[2] = expand((v >> 4) & 15, 4);
        argb[3] = expand(v & 15, 4);
        return;
    }
    case TEX_R5G6B5: {
        const u32 v = load_be<u16>(p);
        argb[0] = 255;
        argb[1] = expand(v >> 11, 5);
        argb[2] = expand((v >> 5) & 63, 6);
        argb[3] = expand(v & 31, 5);
        return;
    }
    case TEX_R6G5B5: {
        const u32 v = load_be<u16>(p);
        argb[0] = 255;
        argb[1] = expand(v >> 10, 6);
        argb[2] = expand((v >> 5) & 31, 5);
        argb[3] = expand(v & 31, 5);
        return;
    }
    case TEX_R5G5B5A1: {
        const u32 v = load_be<u16>(p);
        argb[0] = (v & 1) ? 255 : 0;
        argb[1] = expand(v >> 11, 5);
        argb[2] = expand((v >> 6) & 31, 5);
        argb[3] = expand((v >> 1) & 31, 5);
        return;
    }
    case TEX_A8R8G8B8: case TEX_D8R8G8B8:
        argb[0] = fmt == TEX_D8R8G8B8 ? 255 : p[0];
        argb[1] = p[1];
        argb[2] = p[2];
        argb[3] = p[3];
        return;
    case TEX_G8B8: case TEX_HILO8: case TEX_HILO_S8:
        argb[0] = p[0];
        argb[1] = p[0];
        argb[2] = p[0];
        argb[3] = p[1];
        if (fmt != TEX_G8B8) { argb[0] = 255; argb[1] = p[0]; argb[2] = p[1]; argb[3] = 0; }
        return;
    case TEX_B8R8_G8R8: case TEX_R8B8_R8G8:
        argb[0] = 255;
        argb[1] = argb[2] = argb[3] = p[1];
        return;
    case TEX_DEPTH24_D8: case TEX_DEPTH24_D8_FLOAT:
        argb[0] = 255;
        argb[1] = argb[2] = argb[3] = p[0];
        return;
    case TEX_DEPTH16: case TEX_DEPTH16_FLOAT: case TEX_X16:
        argb[0] = 255;
        argb[1] = argb[2] = argb[3] = p[0];
        if (fmt == TEX_DEPTH16_FLOAT) argb[1] = argb[2] = argb[3] = unorm(half_to_float(load_be<u16>(p)));
        return;
    case TEX_Y16_X16:
        argb[0] = 255;
        argb[1] = p[0];
        argb[2] = p[2];
        argb[3] = 0;
        return;
    case TEX_Y16_X16_FLOAT:
        argb[0] = 255;
        argb[1] = unorm(half_to_float(load_be<u16>(p)));
        argb[2] = unorm(half_to_float(load_be<u16>(p + 2)));
        argb[3] = 0;
        return;
    case TEX_W16_Z16_Y16_X16_FLOAT:
        argb[1] = unorm(half_to_float(load_be<u16>(p)));
        argb[2] = unorm(half_to_float(load_be<u16>(p + 2)));
        argb[3] = unorm(half_to_float(load_be<u16>(p + 4)));
        argb[0] = unorm(half_to_float(load_be<u16>(p + 6)));
        return;
    case TEX_W32_Z32_Y32_X32_FLOAT:
        argb[1] = unorm(as_f32(load_be<u32>(p)));
        argb[2] = unorm(as_f32(load_be<u32>(p + 4)));
        argb[3] = unorm(as_f32(load_be<u32>(p + 8)));
        argb[0] = unorm(as_f32(load_be<u32>(p + 12)));
        return;
    case TEX_X32_FLOAT:
        argb[0] = 255;
        argb[1] = argb[2] = argb[3] = unorm(as_f32(load_be<u32>(p)));
        return;
    default:
        argb[0] = 255; argb[1] = 255; argb[2] = 0; argb[3] = 255;  // magenta: unknown format
        return;
    }
}

// ---- DXT (block data is little endian, as on PC)
void rgb565(u16 c, u8 out[3]) {
    out[0] = expand(c >> 11, 5);
    out[1] = expand((c >> 5) & 63, 6);
    out[2] = expand(c & 31, 5);
}

void dxt_color_block(const u8* b, u8 rgba[16][4], bool dxt1) {
    const u16 c0 = static_cast<u16>(b[0] | (b[1] << 8)), c1 = static_cast<u16>(b[2] | (b[3] << 8));
    u8 pal[4][4];
    rgb565(c0, pal[0]);
    rgb565(c1, pal[1]);
    pal[0][3] = pal[1][3] = 255;
    if (c0 > c1 || !dxt1) {
        for (int k = 0; k < 3; ++k) {
            pal[2][k] = static_cast<u8>((2 * pal[0][k] + pal[1][k] + 1) / 3);
            pal[3][k] = static_cast<u8>((pal[0][k] + 2 * pal[1][k] + 1) / 3);
        }
        pal[2][3] = pal[3][3] = 255;
    } else {
        for (int k = 0; k < 3; ++k) {
            pal[2][k] = static_cast<u8>((pal[0][k] + pal[1][k]) / 2);
            pal[3][k] = 0;
        }
        pal[2][3] = 255;
        pal[3][3] = 0;
    }
    const u32 bits = static_cast<u32>(b[4]) | (b[5] << 8) | (b[6] << 16) | (static_cast<u32>(b[7]) << 24);
    for (int t = 0; t < 16; ++t) std::memcpy(rgba[t], pal[(bits >> (2 * t)) & 3], 4);
}

void dxt_block(u32 fmt, const u8* b, u8 rgba[16][4]) {
    if (fmt == TEX_DXT1) {
        dxt_color_block(b, rgba, true);
        return;
    }
    dxt_color_block(b + 8, rgba, false);
    if (fmt == TEX_DXT23) {
        for (int t = 0; t < 16; ++t) rgba[t][3] = expand((b[t / 2] >> (4 * (t & 1))) & 15, 4);
        return;
    }
    u8 a[8];
    a[0] = b[0];
    a[1] = b[1];
    if (a[0] > a[1]) {
        for (int k = 1; k < 7; ++k) a[k + 1] = static_cast<u8>(((7 - k) * a[0] + k * a[1] + 3) / 7);
    } else {
        for (int k = 1; k < 5; ++k) a[k + 1] = static_cast<u8>(((5 - k) * a[0] + k * a[1] + 2) / 5);
        a[6] = 0;
        a[7] = 255;
    }
    u64 bits = 0;
    for (int k = 0; k < 6; ++k) bits |= static_cast<u64>(b[2 + k]) << (8 * k);
    for (int t = 0; t < 16; ++t) rgba[t][3] = a[(bits >> (3 * t)) & 7];
}

u32 level_bytes(const TextureDesc& d, u32 w, u32 h, u32 dd) {
    const u32 fmt = d.base_format();
    if (is_compressed(fmt)) {
        const u32 bw = (w + 3) / 4, bh = (h + 3) / 4;
        const u32 pitch = d.linear() && d.pitch ? d.pitch : bw * texel_bytes(fmt);
        return pitch * bh * dd;
    }
    const u32 pitch = d.linear() && d.pitch ? d.pitch : w * texel_bytes(fmt);
    return pitch * h * dd;
}

}  // namespace

u32 texel_bytes(u32 f) {
    switch (f) {
    case TEX_B8: return 1;
    case TEX_A1R5G5B5: case TEX_A4R4G4B4: case TEX_R5G6B5: case TEX_G8B8: case TEX_R6G5B5: case TEX_DEPTH16:
    case TEX_DEPTH16_FLOAT: case TEX_X16: case TEX_R5G5B5A1: case TEX_HILO8: case TEX_HILO_S8: case TEX_D1R5G5B5:
        return 2;
    case TEX_DXT1: return 8;
    case TEX_DXT23: case TEX_DXT45: return 16;
    case TEX_W16_Z16_Y16_X16_FLOAT: return 8;
    case TEX_W32_Z32_Y32_X32_FLOAT: return 16;
    default: return 4;
    }
}

bool is_compressed(u32 f) { return f == TEX_DXT1 || f == TEX_DXT23 || f == TEX_DXT45; }

u32 swizzle_offset(u32 x, u32 y, u32 z, u32 lw, u32 lh, u32 ld) {
    u32 off = 0, shift = 0;
    while (lw || lh || ld) {
        if (lw) { off |= (x & 1) << shift++; x >>= 1; --lw; }
        if (lh) { off |= (y & 1) << shift++; y >>= 1; --lh; }
        if (ld) { off |= (z & 1) << shift++; z >>= 1; --ld; }
    }
    return off;
}

u32 texture_size(const TextureDesc& d) {
    u32 total = 0;
    const u32 faces = d.cubemap ? 6 : 1;
    for (u32 f = 0; f < faces; ++f) {
        u32 w = d.width, h = d.height, dd = d.depth;
        for (u32 m = 0; m < std::max<u32>(d.mipmaps, 1); ++m) {
            total += level_bytes(d, w, h, dd);
            w = std::max<u32>(w / 2, 1);
            h = std::max<u32>(h / 2, 1);
            dd = std::max<u32>(dd / 2, 1);
        }
        if (d.cubemap) total = (total + 127) & ~127u;  // faces are 128-byte aligned
    }
    return total;
}

std::vector<DecodedLevel> decode_texture(const TextureDesc& d, const u8* src) {
    std::vector<DecodedLevel> out;
    const u32 fmt = d.base_format();
    const u32 faces = d.cubemap ? 6 : 1;
    const u32 bpp = texel_bytes(fmt);
    u32 offset = 0;
    for (u32 f = 0; f < faces; ++f) {
        u32 w = std::max<u32>(d.width, 1), h = std::max<u32>(d.height, 1), dd = std::max<u32>(d.depth, 1);
        for (u32 m = 0; m < std::max<u32>(d.mipmaps, 1); ++m) {
            DecodedLevel lv;
            lv.width = w;
            lv.height = h;
            lv.depth = dd;
            lv.rgba.assign(static_cast<std::size_t>(w) * h * dd * 4, 0);
            const u8* base = src + offset;
            if (is_compressed(fmt)) {
                const u32 bw = (w + 3) / 4, bh = (h + 3) / 4;
                const u32 pitch = d.linear() && d.pitch ? d.pitch : bw * bpp;
                for (u32 z = 0; z < dd; ++z)
                    for (u32 by = 0; by < bh; ++by)
                        for (u32 bx = 0; bx < bw; ++bx) {
                            u8 block[16][4];
                            dxt_block(fmt, base + z * pitch * bh + by * pitch + bx * bpp, block);
                            for (u32 t = 0; t < 16; ++t) {
                                const u32 x = bx * 4 + t % 4, y = by * 4 + t / 4;
                                if (x >= w || y >= h) continue;
                                const u8 argb[4] = {block[t][3], block[t][0], block[t][1], block[t][2]};
                                remap_store(&lv.rgba[((static_cast<std::size_t>(z) * h + y) * w + x) * 4], argb, d.remap);
                            }
                        }
            } else {
                const bool swz = !d.linear();
                const u32 lw = log2u(w), lh = log2u(h), ld = log2u(dd);
                const u32 pitch = d.linear() && d.pitch ? d.pitch : w * bpp;
                for (u32 z = 0; z < dd; ++z)
                    for (u32 y = 0; y < h; ++y)
                        for (u32 x = 0; x < w; ++x) {
                            const u32 so = swz ? swizzle_offset(x, y, z, lw, lh, ld) * bpp
                                               : z * pitch * h + y * pitch + x * bpp;
                            u8 argb[4];
                            texel(fmt, base + so, argb);
                            remap_store(&lv.rgba[((static_cast<std::size_t>(z) * h + y) * w + x) * 4], argb, d.remap);
                        }
            }
            offset += level_bytes(d, w, h, dd);
            out.push_back(std::move(lv));
            w = std::max<u32>(w / 2, 1);
            h = std::max<u32>(h / 2, 1);
            dd = std::max<u32>(dd / 2, 1);
        }
        if (d.cubemap) offset = (offset + 127) & ~127u;
    }
    return out;
}

}  // namespace opentit::rsx
