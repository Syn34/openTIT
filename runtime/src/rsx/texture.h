// RSX texture formats -> RGBA8 on the CPU.
//
// The first renderer keeps the GPU side simple: every sampled texture is
// decoded here into tightly packed RGBA8 (R first in memory), mip by mip,
// cube face by face. Float formats are clamped to [0,1] in the process; a
// later pass can upload those natively.
#pragma once

#include <vector>

#include "common/types.h"

namespace opentit::rsx {

enum : u32 {
    TEX_B8 = 0x81, TEX_A1R5G5B5 = 0x82, TEX_A4R4G4B4 = 0x83, TEX_R5G6B5 = 0x84, TEX_A8R8G8B8 = 0x85,
    TEX_DXT1 = 0x86, TEX_DXT23 = 0x87, TEX_DXT45 = 0x88, TEX_G8B8 = 0x8B, TEX_B8R8_G8R8 = 0x8D,
    TEX_R8B8_R8G8 = 0x8E, TEX_R6G5B5 = 0x8F, TEX_DEPTH24_D8 = 0x90, TEX_DEPTH24_D8_FLOAT = 0x91,
    TEX_DEPTH16 = 0x92, TEX_DEPTH16_FLOAT = 0x93, TEX_X16 = 0x94, TEX_Y16_X16 = 0x95, TEX_R5G5B5A1 = 0x97,
    TEX_HILO8 = 0x98, TEX_HILO_S8 = 0x99, TEX_W16_Z16_Y16_X16_FLOAT = 0x9A, TEX_W32_Z32_Y32_X32_FLOAT = 0x9B,
    TEX_X32_FLOAT = 0x9C, TEX_D1R5G5B5 = 0x9D, TEX_D8R8G8B8 = 0x9E, TEX_Y16_X16_FLOAT = 0x9F,
    TEX_LN = 0x20,  // linear (pitch) layout; otherwise swizzled
    TEX_UN = 0x40,  // unnormalised coordinates
};

struct TextureDesc {
    u32 format = 0;           // raw format byte (with LN/UN flags)
    u32 width = 1, height = 1, depth = 1;
    u32 mipmaps = 1;
    bool cubemap = false;
    u32 pitch = 0;            // linear layout row pitch (0 = tightly packed)
    u32 remap = 0xAAE4;       // TEXTURE_CONTROL1 channel remap
    u32 base_format() const { return format & ~(TEX_LN | TEX_UN); }
    bool linear() const { return (format & TEX_LN) != 0; }
};

// Bytes per texel (per 4x4 block for DXT).
u32 texel_bytes(u32 base_format);
bool is_compressed(u32 base_format);

// Total guest bytes the texture occupies (all mips, faces, slices).
u32 texture_size(const TextureDesc& d);

struct DecodedLevel {
    u32 width = 0, height = 0, depth = 0;
    std::vector<u8> rgba;  // width*height*depth*4
};

// Decode every level (and every face for cubemaps, as face-major: face 0's
// mips, then face 1's...). `src` points at guest memory (big endian).
std::vector<DecodedLevel> decode_texture(const TextureDesc& d, const u8* src);

// Morton offset of (x, y, z) in a swizzled texture of log2 dimensions.
u32 swizzle_offset(u32 x, u32 y, u32 z, u32 log2w, u32 log2h, u32 log2d);

}  // namespace opentit::rsx
