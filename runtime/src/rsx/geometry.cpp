#include "geometry.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "opentit/memory.h"

namespace opentit::rsx {

namespace {
constexpr u32 kRestart = 0xFFFFFFFFu;
constexpr u32 NV4097_SET_FREQUENCY_DIVIDER_OPERATION = 0x1FC0;
constexpr u32 NV4097_SET_RESTART_INDEX_ENABLE = 0x1DAC;
constexpr u32 NV4097_SET_RESTART_INDEX = 0x1DB0;
constexpr u32 kMaxVertices = 1u << 22;

float f16(u16 h) {
    const u32 sign = (h >> 15) & 1, exp = (h >> 10) & 0x1F, man = h & 0x3FF;
    float f;
    if (exp == 0) f = std::ldexp(static_cast<float>(man), -24);
    else if (exp == 31) f = man ? NAN : INFINITY;
    else f = std::ldexp(static_cast<float>(man | 0x400), static_cast<int>(exp) - 25);
    return sign ? -f : f;
}
float f32(u32 v) {
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}
s32 sext(u32 v, u32 bits) { return static_cast<s32>(v << (32 - bits)) >> (32 - bits); }
}  // namespace

u32 attribute_element_size(u32 type, u32 size) {
    switch (type) {
    case 1: case 5: return 2 * size;  // S1, S32K
    case 2: return 4 * size;          // F
    case 3: return 2 * size;          // SF
    case 4: case 7: return size;      // UB, UB256
    case 6: return 4;                 // CMP (packed xyz)
    default: return 4 * size;
    }
}

std::array<float, 4> fetch_attribute(u32 type, u32 size, const u8* p) {
    std::array<float, 4> v = {0.0f, 0.0f, 0.0f, 1.0f};
    if (type == 6) {  // CMP: x 11 bits, y 11 bits, z 10 bits, signed normalised
        const u32 w = load_be<u32>(p);
        v[0] = std::max(static_cast<float>(sext(w & 0x7FF, 11)) / 1023.0f, -1.0f);
        v[1] = std::max(static_cast<float>(sext((w >> 11) & 0x7FF, 11)) / 1023.0f, -1.0f);
        v[2] = std::max(static_cast<float>(sext(w >> 22, 10)) / 511.0f, -1.0f);
        return v;
    }
    for (u32 c = 0; c < std::min<u32>(size, 4); ++c) {
        switch (type) {
        case 1: v[c] = std::max(static_cast<float>(static_cast<s16>(load_be<u16>(p + 2 * c))) / 32767.0f, -1.0f); break;
        case 2: v[c] = f32(load_be<u32>(p + 4 * c)); break;
        case 3: v[c] = f16(load_be<u16>(p + 2 * c)); break;
        case 4: v[c] = static_cast<float>(p[c]) / 255.0f; break;
        case 5: v[c] = static_cast<float>(static_cast<s16>(load_be<u16>(p + 2 * c))); break;
        case 7: v[c] = static_cast<float>(p[c]); break;
        default: break;
        }
    }
    return v;
}

namespace {

// Convert one batch of relative indices (may contain restart markers) to the output topology.
void convert(Prim prim, const std::vector<u32>& in, std::vector<u32>& out) {
    // Split at restart markers.
    std::vector<std::vector<u32>> segs(1);
    for (u32 i : in) {
        if (i == kRestart) {
            if (!segs.back().empty()) segs.emplace_back();
        } else {
            segs.back().push_back(i);
        }
    }
    const bool strip_like = prim == Prim::LineStrip || prim == Prim::TriangleStrip || prim == Prim::TriangleFan ||
                            prim == Prim::QuadStrip || prim == Prim::Polygon || prim == Prim::LineLoop;
    bool first_seg = true;
    for (const auto& s : segs) {
        if (s.empty()) continue;
        if (strip_like && !first_seg) out.push_back(kRestart);
        first_seg = false;
        switch (prim) {
        case Prim::Quads:
            for (std::size_t k = 0; k + 3 < s.size(); k += 4) {
                const u32 q[6] = {s[k], s[k + 1], s[k + 2], s[k + 2], s[k + 3], s[k]};
                out.insert(out.end(), q, q + 6);
            }
            break;
        case Prim::QuadStrip:
            out.insert(out.end(), s.begin(), s.begin() + static_cast<std::ptrdiff_t>(s.size() & ~std::size_t(1)));
            break;
        case Prim::LineLoop:
            out.insert(out.end(), s.begin(), s.end());
            if (s.size() > 1) out.push_back(s[0]);
            break;
        case Prim::Lines:
            out.insert(out.end(), s.begin(), s.begin() + static_cast<std::ptrdiff_t>(s.size() & ~std::size_t(1)));
            break;
        case Prim::Triangles:
            out.insert(out.end(), s.begin(), s.begin() + static_cast<std::ptrdiff_t>(s.size() - s.size() % 3));
            break;
        default:
            out.insert(out.end(), s.begin(), s.end());
            break;
        }
    }
}

Topology topology_of(Prim p) {
    switch (p) {
    case Prim::Points: return Topology::PointList;
    case Prim::Lines: return Topology::LineList;
    case Prim::LineLoop: case Prim::LineStrip: return Topology::LineStrip;
    case Prim::TriangleStrip: case Prim::QuadStrip: return Topology::TriangleStrip;
    case Prim::TriangleFan: case Prim::Polygon: return Topology::TriangleFan;
    default: return Topology::TriangleList;
    }
}

}  // namespace

Geometry assemble(const State& s, const DrawCall& c, const ToEa& to_ea, u32 needed) {
    Geometry g;
    g.topology = topology_of(c.prim);
    g.constant_attributes = s.vertex_regs;

    // ---- inline vertex data: one packed record per vertex, enabled arrays in order
    if (!c.inline_vertices.empty()) {
        u32 stride = 0;
        std::array<u32, 16> at{};
        for (int a = 0; a < 16; ++a) {
            const VertexArray va = s.vertex_array(a);
            if (!va.enabled()) continue;
            at[a] = stride;
            stride += attribute_element_size(va.type, va.size);
        }
        if (!stride) return g;
        std::vector<u8> bytes(c.inline_vertices.size() * 4);
        for (std::size_t k = 0; k < c.inline_vertices.size(); ++k) store_be<u32>(&bytes[4 * k], c.inline_vertices[k]);
        g.vertex_count = static_cast<u32>(bytes.size() / stride);
        for (int a = 0; a < 16; ++a) {
            const VertexArray va = s.vertex_array(a);
            if (!va.enabled() || !(needed & (1u << a))) continue;
            g.attribute_mask |= 1u << a;
            auto& out = g.attributes[a];
            out.resize(static_cast<std::size_t>(g.vertex_count) * 4);
            for (u32 v = 0; v < g.vertex_count; ++v) {
                const auto f = fetch_attribute(va.type, va.size, &bytes[v * stride + at[a]]);
                std::memcpy(&out[v * 4], f.data(), 16);
            }
        }
        std::vector<u32> seq(g.vertex_count);
        for (u32 v = 0; v < g.vertex_count; ++v) seq[v] = v;
        convert(c.prim, seq, g.indices);
        g.batches.emplace_back(0, static_cast<u32>(g.indices.size()));
        return g;
    }

    // ---- gather the referenced vertex indices per batch
    const u32 base_index = s.reg(NV4097_SET_VERTEX_DATA_BASE_INDEX);
    const bool restart = s.reg(NV4097_SET_RESTART_INDEX_ENABLE) != 0;
    const u32 restart_index = s.reg(NV4097_SET_RESTART_INDEX);
    std::vector<std::vector<u32>> batches;
    if (!c.immediate_indices.empty()) {
        batches.push_back(c.immediate_indices);
    } else if (c.indexed) {
        const u32 isize = c.index16 ? 2 : 4;
        const u32 base = to_ea(c.index_offset, c.index_local);
        if (!base) return g;
        for (const auto& r : c.ranges) {
            std::vector<u32> b;
            b.reserve(r.second);
            for (u32 k = 0; k < r.second; ++k) {
                const u32 ea = base + (r.first + k) * isize;
                u32 i = c.index16 ? mem::r16(ea) : mem::r32(ea);
                if (restart && i == restart_index) i = kRestart;
                else if (c.index16 && !restart && i == 0xFFFF) i = 0xFFFF;  // ordinary index
                b.push_back(i == kRestart ? kRestart : i + base_index);
            }
            batches.push_back(std::move(b));
        }
    } else {
        for (const auto& r : c.ranges) {
            std::vector<u32> b(r.second);
            for (u32 k = 0; k < r.second; ++k) b[k] = r.first + k;
            batches.push_back(std::move(b));
        }
    }

    u32 lo = ~0u, hi = 0;
    for (const auto& b : batches)
        for (u32 i : b)
            if (i != kRestart) { lo = std::min(lo, i); hi = std::max(hi, i); }
    if (lo > hi) return g;
    if (hi - lo + 1 > kMaxVertices) hi = lo + kMaxVertices - 1;
    g.vertex_count = hi - lo + 1;

    // ---- fetch attributes for [lo, hi]
    const u32 base_offset = s.reg(NV4097_SET_VERTEX_DATA_BASE_OFFSET);
    const u32 freq_modulo = s.reg(NV4097_SET_FREQUENCY_DIVIDER_OPERATION);
    for (int a = 0; a < 16; ++a) {
        const VertexArray va = s.vertex_array(a);
        if (!va.enabled() || !(needed & (1u << a))) continue;
        const u32 ea = to_ea(va.offset + base_offset, va.local);
        if (!ea) continue;
        g.attribute_mask |= 1u << a;
        auto& out = g.attributes[a];
        out.resize(static_cast<std::size_t>(g.vertex_count) * 4);
        for (u32 v = 0; v < g.vertex_count; ++v) {
            u32 e = lo + v;
            if (va.frequency > 1) e = (freq_modulo >> a) & 1 ? e % va.frequency : e / va.frequency;
            const auto f = fetch_attribute(va.type, va.size, mem::ptr(ea + e * va.stride));
            std::memcpy(&out[v * 4], f.data(), 16);
        }
    }

    // ---- indices relative to lo, converted to the output topology
    for (auto& b : batches) {
        for (u32& i : b)
            if (i != kRestart) i = std::min(i, hi) - lo;
        const u32 first = static_cast<u32>(g.indices.size());
        convert(c.prim, b, g.indices);
        g.batches.emplace_back(first, static_cast<u32>(g.indices.size()) - first);
    }
    return g;
}

}  // namespace opentit::rsx
