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
#include <cstring>
#include <vector>

namespace agl {

class AglMaterial;
class AglRenderTarget;
class AglTexture;

// Packet opcodes. The payloads follow the 12-byte header; the layouts of
// the state packets (0x0A..0x10) mirror the material words they stand for.
enum AglDrawOp : uint8_t {
    kAglDraw_Projection     = 0x00,   // payload: Matrix44
    kAglDraw_View           = 0x01,   // payload: Matrix44
    kAglDraw_World          = 0x02,   // payload: Matrix44
    kAglDraw_ScreenWorld    = 0x03,   // payload: Matrix44 (projected without the view)
    kAglDraw_Material       = 0x04,   // payload: AglMaterial*
    kAglDraw_MaterialShader = 0x05,   // payload: AglMaterial* (programs only, no fixed-function state)
    kAglDraw_VertexConst    = 0x06,   // payload: register (uint32), float[4]
    kAglDraw_FragmentConst  = 0x07,   // payload: constant slot (uint32), float[4]
    kAglDraw_Texture        = 0x08,   // payload: AglTexture*, slot
    kAglDraw_Scissor        = 0x09,   // payload: x, y, width, height (uint16)
    kAglDraw_Depth          = 0x0A,   // payload: AglDrawDepthPacket
    kAglDraw_Blend          = 0x0B,   // payload: AglDrawBlendPacket
    kAglDraw_Cull           = 0x0C,   // payload: culled face (uint32, 0 = none)
    kAglDraw_StencilWrite   = 0x0D,   // payload: enable << 24 | ref << 16
    kAglDraw_StencilTest    = 0x0E,   // payload: enable << 24 | ref << 16 | compare function
    kAglDraw_AlphaTest      = 0x0F,   // payload: enable << 24 | ref << 16 | compare function
    kAglDraw_LineWidth      = 0x10,   // payload: float
    kAglDraw_State          = 0x11,   // argument/payload: state byte (non-zero: depth bias on)
    kAglDraw_AppendList     = 0x12,   // payload: AglDrawList*, start offset
    kAglDraw_Primitive      = 0x13,   // argument: vertex count << 16; spare: first vertex
    kAglDraw_Marker         = 0x16,   // ends the packets of a view
    kAglDraw_End            = 0x17,   // ends the list
};

struct AglDrawDepthPacket {
    uint32_t func;    // gpu::CompareFunc
    uint32_t write;   // 1: depth writes on
};

struct AglDrawBlendPacket {
    uint16_t src;      // gpu::BlendFactor
    uint16_t dst;      // gpu::BlendFactor
    uint16_t equation; // gpu::BlendEquation
    uint16_t enable;
};

// Matrices of a replay: the projection and view last read and their
// product (console: two pointers into the packets, then the product).
struct AglDrawReplayContext {
    ape::Matrix44 projection = {};
    ape::Matrix44 view = {};
    ape::Matrix44 viewProjection = {};
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

    struct PacketHeader {
        uint8_t op;
        uint32_t tag;          // 24 bits
        uint32_t argument;
        uint32_t spare;
    };

    // 'packetBytes' of packet stream (rounded up to 16), 'vertexBuffers'
    // vertex areas of 'vertexCapacity' vertices each (one per frame in
    // flight; none for lists that only record state), 'ring' for a list
    // whose packet stream wraps around instead of restarting every frame.
    explicit AglDrawList(size_t packetBytes = 0x40000, uint32_t vertexCapacity = 0x4000, uint8_t vertexBuffers = 2,
                         bool ring = false);

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
    // Same for an affine matrix (the console reads it as four rows).
    void SetWorldMatrix(const ape::Matrix34& world, uint32_t tag);
    // Scissor rectangle, clipped to the view; null restores the full view.
    void SetScissor(const uint16_t* rect, uint32_t tag);
    void SetMaterial(AglMaterial* material, uint32_t tag);
    void SetTexture(AglTexture* texture, int slot, uint32_t tag);
    void SetState(uint8_t value, uint32_t tag);
    // Replays the packets of 'list' from 'start' at this point (the GUI
    // records into its own lists and appends them). Draw state is unknown
    // afterwards.
    void AppendList(const AglDrawList* list, uint32_t start, uint32_t tag);
    // Reserves 'count' vertices for a primitive of type 'primitive'.
    void AllocPrimitive(AglDrawPrimitive* out, uint8_t primitive, uint32_t count, uint32_t tag);
    // Closes the primitive filled in since AllocPrimitive (nothing to do;
    // the vertices are written in place).
    void EndPrimitive();
    // True when a primitive of 'vertexCount' vertices still fits in this
    // frame's vertex area (the console also checks the packet ring).
    bool HasRoomFor(uint32_t vertexCount) const { return m_vertexUsed + vertexCount < m_vertexCapacity; }

    // Render thread side: plays the packets from the read position (set by
    // SetReadPosition) up to the end of the view, issuing the GPU commands.
    void Replay(AglDrawReplayContext& context);
    void SetReadPosition(size_t offset) const { m_readCursor = offset; }

    size_t GetSize() const { return m_cursor; }
    const uint8_t* GetData() const { return m_packets.data(); }

private:
    void WriteHeader(uint8_t op, uint32_t tag, uint32_t argument, uint32_t spare = 0, uint8_t low = 0);
    void Write(const void* data, size_t bytes, size_t align);
    // Offset where 'bytes' with alignment 'align' go after 'cursor' (a
    // block that does not fit restarts at the beginning).
    size_t PlaceBlock(size_t cursor, size_t bytes, size_t align) const;
    const void* Read(size_t bytes, size_t align) const;
    template <typename T> T ReadValue(size_t align = 4) const
    {
        T value;
        std::memcpy(&value, Read(sizeof(T), align), sizeof(T));
        return value;
    }

    // Packet handlers of the replay.
    void ReadAppendPacket(const AglDrawList** list, uint32_t* start) const;
    void ReplayStatePacket();
    void ReplayLineWidthPacket() const;
    void ReplayAlphaTestPacket() const;
    void ReadBlendPacket(uint32_t* src, uint32_t* dst, uint32_t* equation, bool* enable) const;
    void ReplayStencilTestPacket() const;
    void ReplayStencilWritePacket() const;
    AglTexture* ReadTexturePacket(int* slot) const;
    void ReplayFragmentConstPacket() const;
    void ReplayVertexConstPacket() const;
    void UploadScreenMatrix(const ape::Matrix44& world, const ape::Matrix44& projection) const;
    void UploadWorldMatrix(const ape::Matrix44& world, const ape::Matrix44& viewProjection) const;

    std::vector<uint8_t> m_packets;        // 0x00..0x10: begin, end, read cursor, write cursor
    size_t m_cursor = 0;
    mutable size_t m_readCursor = 0;       // 0x0C
    bool m_ring = false;                   // 0x4C

    std::vector<std::vector<AglDrawVertex>> m_vertexBuffers;   // 0x14: one per frame in flight
    uint32_t m_vertexCapacity = 0;         // 0x34
    uint8_t m_currentBuffer = 0;           // 0x39
    uint8_t m_replayBuffer = 0;            // 0x3A vertex area the replay reads
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
// Creates the game's draw list with the configured sizes (Agl start-up).
void CreateMainDrawList(size_t packetBytes, uint32_t vertexCapacity);
// Render thread: replays the game's draw list from its read position.
void ReplayMainDrawList();

} // namespace agl
