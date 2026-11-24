// openTIT - Agl draw list: the stream of draw packets the game records for the render thread.
//
// Game code does not talk to the GPU directly. While a render target is
// being drawn it appends small packets (set camera, set world matrix, set
// material, set texture, scissor, callback, immediate-mode primitive ...)
// to a draw list; the render thread replays them. Each packet starts with a
// 12-byte header (opcode, a 24-bit tag naming the caller, one argument word
// and one spare word) followed by its payload. The list skips redundant
// state packets by remembering what it last recorded.
//
// Immediate primitives get their vertices from a separate vertex area that
// is cycled between frames (the console keeps several in GPU memory).
//
// The console list lives at 0x010DDBB4 (a ring buffer of packets).
#pragma once

#include "ape/math/ApeMatrixOps.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace agl {

class AglMaterial;
class AglRenderTarget;
class AglTexture;

enum AglDrawOp : uint8_t {
    kAglDraw_Projection = 0x00,   // payload: Matrix44
    kAglDraw_View       = 0x01,   // payload: Matrix44
    kAglDraw_World      = 0x02,   // payload: Matrix44
    kAglDraw_Material   = 0x04,   // payload: AglMaterial*
    kAglDraw_Texture    = 0x08,   // payload: AglTexture*, slot
    kAglDraw_Scissor    = 0x09,   // payload: x, y, width, height (uint16)
    kAglDraw_State      = 0x11,   // argument: state byte
    kAglDraw_Callback   = 0x12,   // payload: function, argument
    kAglDraw_Primitive  = 0x13,   // argument: vertex count; spare: first vertex
    kAglDraw_Marker     = 0x16,
};

// The vertex format of immediate primitives (24 bytes).
struct AglDrawVertex {
    float position[3];
    uint8_t color[4];
    float uv[2];
};

// A primitive being filled in by the caller.
struct AglDrawPrimitive {
    AglDrawVertex* vertices = nullptr;
    uint16_t count = 0;
    uint16_t field_6 = 0;
    uint8_t field_8 = 0;
};

class AglDrawList {
public:
    using Callback = void (*)(void* arg);

    struct PacketHeader {
        uint8_t op;
        uint32_t tag;          // 24 bits
        uint32_t argument;
        uint32_t spare;
    };

    explicit AglDrawList(size_t packetBytes = 0x40000, uint32_t vertexCapacity = 0x4000, uint8_t vertexBuffers = 2);

    // Starts a new frame: the next vertex area, and (for a ring list) an
    // empty packet stream.
    void BeginFrame();
    // Forgets the recorded state so the next packets are always written.
    void InvalidateCache();
    // Starts drawing 'view' (the view being rendered when null). 'flags'
    // bit 0 records the projection as well. Returns the packet offset
    // where the view starts (0 unless the list is a ring).
    size_t BeginView(AglRenderTarget* view, uint8_t flags, uint32_t tag);

    void Marker(uint32_t tag);
    void SetWorldMatrix(const ape::Matrix44& world, uint32_t tag);
    // Scissor rectangle, clipped to the view; null restores the full view.
    void SetScissor(const uint16_t* rect, uint32_t tag);
    void SetMaterial(AglMaterial* material, uint32_t tag);
    void SetTexture(AglTexture* texture, int slot, uint32_t tag);
    void SetState(uint8_t value, uint32_t tag);
    // A function the render thread calls at this point (draw state is
    // unknown afterwards).
    void AddCallback(Callback fn, void* arg, uint32_t tag);
    // Reserves 'count' vertices for a primitive of type 'primitive'.
    void AllocPrimitive(AglDrawPrimitive* out, uint8_t primitive, uint32_t count, uint32_t tag);

    size_t GetSize() const { return m_cursor; }
    const uint8_t* GetData() const { return m_packets.data(); }

private:
    void WriteHeader(uint8_t op, uint32_t tag, uint32_t argument, uint32_t spare = 0, uint8_t low = 0);
    void Write(const void* data, size_t bytes, size_t align);

    std::vector<uint8_t> m_packets;        // 0x00..0x10: begin, end, start, cursor
    size_t m_cursor = 0;
    bool m_ring = false;                   // 0x4C

    std::vector<std::vector<AglDrawVertex>> m_vertexBuffers;   // 0x14: one per frame in flight
    uint32_t m_vertexCapacity = 0;         // 0x34
    uint8_t m_currentBuffer = 0;           // 0x39
    uint32_t m_vertexUsed = 0;             // 0x48

    // Recorded state (0x50..0xC8).
    const AglMaterial* m_material = nullptr;
    const AglTexture* m_textures[2] = {};
    ape::Matrix44 m_world = {};
    bool m_worldValid = false;
    uint16_t m_scissor[4] = {};
    bool m_scissorValid = false;
    uint16_t m_viewRect[4] = {};           // 0xCA
    uint8_t m_state = 0;                   // 0xC8
};

// The game's draw list (0x010DDBB4).
AglDrawList& GetDrawList();

} // namespace agl
