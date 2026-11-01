// Draw assembly on the CPU: RSX vertex arrays -> host float4 attributes,
// RSX primitives -> Vulkan-friendly topologies and 32-bit indices.
#pragma once

#include <array>
#include <functional>
#include <vector>

#include "rsx.h"

namespace opentit::rsx {

enum class Topology : u8 { PointList, LineList, LineStrip, TriangleList, TriangleStrip, TriangleFan };

struct Geometry {
    Topology topology = Topology::TriangleList;
    std::vector<u32> indices;                   // relative to the fetched vertex range
    std::vector<std::pair<u32, u32>> batches;   // (first index, index count), one per draw range
    u32 vertex_count = 0;                       // vertices fetched (indices < vertex_count)
    u32 attribute_mask = 0;                     // attributes present in `attributes`
    std::array<std::vector<float>, 16> attributes;  // vertex_count * 4 floats each
    std::array<std::array<float, 4>, 16> constant_attributes{};  // for attributes not in the mask
};

// Translate RSX offsets to guest EAs (0 if unmapped).
using ToEa = std::function<u32(u32 offset, bool local)>;

// Assemble a draw. `needed` is the vertex program's inputs_read mask.
Geometry assemble(const State& s, const DrawCall& c, const ToEa& to_ea, u32 needed);

// Decode one attribute element at `p` (guest memory) to float4.
std::array<float, 4> fetch_attribute(u32 type, u32 size, const u8* p);

// Byte size of one element of an array with this type/size.
u32 attribute_element_size(u32 type, u32 size);

}  // namespace opentit::rsx
