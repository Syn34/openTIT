// openTIT - Agl draw list: the stream of draw packets the game records for the render thread.
#include "agl/AglDrawList.h"

#include <memory>

#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"

#include <cstring>

namespace agl {

namespace {

constexpr size_t kHeaderBytes = 12;

ape::Matrix44 IdentityMatrix()
{
    ape::Matrix44 m = {};
    m.rows[0].x = m.rows[1].y = m.rows[2].z = m.rows[3].w = 1.0f;
    return m;
}

bool SameMatrix(const ape::Matrix44& a, const ape::Matrix44& b)
{
    constexpr float kTolerance = 1e-5f;
    const float* x = &a.rows[0].x;
    const float* y = &b.rows[0].x;
    // The console compares the 3x4 part (Matrix34Equal).
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 3; ++c) {
            const float d = x[r * 4 + c] - y[r * 4 + c];
            if (d > kTolerance || d < -kTolerance)
                return false;
        }
    return true;
}

ape::Matrix44 ToMatrix44(const ape::Matrix34& m)
{
    ape::Matrix44 r;
    r.rows[0] = { m.xAxis.x, m.xAxis.y, m.xAxis.z, 0.0f };
    r.rows[1] = { m.yAxis.x, m.yAxis.y, m.yAxis.z, 0.0f };
    r.rows[2] = { m.zAxis.x, m.zAxis.y, m.zAxis.z, 0.0f };
    r.rows[3] = { m.translation.x, m.translation.y, m.translation.z, 1.0f };
    return r;
}

} // namespace

// @func 0x0050EB08 AglDrawList::AglDrawList
AglDrawList::AglDrawList(size_t packetBytes, uint32_t vertexCapacity, uint8_t vertexBuffers, bool ring)
    : m_packets((packetBytes + 0xF) & ~size_t(0xF)), m_ring(ring), m_vertexCapacity(vertexCapacity)
{
    // NOTE: a list without vertex areas still gets one here so that the
    // vertex accessors never index an empty vector; it is never written.
    m_vertexBuffers.resize(vertexBuffers ? vertexBuffers : 1);
    for (auto& buffer : m_vertexBuffers)
        buffer.resize(vertexCapacity);
    // Nothing recorded yet: the first packets always go out.
    InvalidateCache();
}

namespace {
std::unique_ptr<AglDrawList> g_mainDrawList;   // 0x010DDBB4
}

void CreateMainDrawList(size_t packetBytes, uint32_t vertexCapacity)
{
    g_mainDrawList = std::make_unique<AglDrawList>(packetBytes, vertexCapacity, 2, false);
}

AglDrawList& GetDrawList()
{
    // Code that runs without the Agl start-up (tools, tests) gets a list
    // of the default size.
    if (!g_mainDrawList)
        g_mainDrawList = std::make_unique<AglDrawList>();
    return *g_mainDrawList;
}

size_t AglDrawList::PlaceBlock(size_t cursor, size_t bytes, size_t align) const
{
    const size_t at = (cursor + align - 1) & ~(align - 1);
    // Blocks never straddle the end: one that does not fit restarts at the
    // beginning (the console list is a ring).
    return at + bytes > m_packets.size() ? 0 : at;
}

void AglDrawList::Write(const void* data, size_t bytes, size_t align)
{
    const size_t at = PlaceBlock(m_cursor, bytes, align);
    std::memcpy(m_packets.data() + at, data, bytes);
    m_cursor = at + bytes;
}

const void* AglDrawList::Read(size_t bytes, size_t align) const
{
    const size_t at = PlaceBlock(m_readCursor, bytes, align);
    m_readCursor = at + bytes;
    return m_packets.data() + at;
}

void AglDrawList::WriteHeader(uint8_t op, uint32_t tag, uint32_t argument, uint32_t spare, uint8_t low)
{
    uint8_t header[kHeaderBytes];
    const uint32_t word0 = (uint32_t(op) << 24) | (tag & 0xFF0000) | low;
    const uint32_t word1 = argument;
    const uint32_t words[3] = { word0, word1, spare };
    std::memcpy(header, words, sizeof(header));
    Write(header, sizeof(header), 4);
}

// @func 0x0050FF64 AglDrawList::BeginFrame
void AglDrawList::BeginFrame()
{
    m_currentBuffer = static_cast<uint8_t>((m_currentBuffer + 1) % m_vertexBuffers.size());
    m_replayBuffer = m_currentBuffer;
    m_vertexUsed = 0;
    if (m_ring) {
        m_cursor = 0;
        m_readCursor = 0;
    }
}

void AglDrawList::InvalidateCache()
{
    m_material = nullptr;
    m_textures[0] = m_textures[1] = nullptr;
    m_world = {};
    m_world.rows[0].x = 100000.0f;   // never equal to a real matrix
    m_worldValid = false;
    m_scissorValid = false;
    m_state = 0;
}

// @func 0x00521A5C AglDrawList::BeginView
size_t AglDrawList::BeginView(AglRenderTarget* view, uint8_t flags, uint32_t tag)
{
    if (!view)
        view = GetRenderingView();
    InvalidateCache();
    if (flags == 0 || !view)
        return 0;

    const size_t start = m_ring ? m_cursor : 0;
    view->UpdateMatrices(true);
    if (flags & 1) {
        WriteHeader(kAglDraw_Projection, 0, flags & 1);
        Write(&view->projection, sizeof(view->projection), 16);
    }
    WriteHeader(kAglDraw_View, 0xFF0000, 0xFFFF);
    const ape::Matrix44 viewMatrix = ToMatrix44(view->view);
    Write(&viewMatrix, sizeof(viewMatrix), 16);
    SetWorldMatrix(IdentityMatrix(), tag);

    const AglRenderSurface* surface = view->surface;
    m_viewRect[0] = 0;
    m_viewRect[1] = 0;
    m_viewRect[2] = static_cast<uint16_t>(surface ? surface->width : 0);
    m_viewRect[3] = static_cast<uint16_t>(surface ? surface->height : 0);
    SetScissor(nullptr, tag);
    return start;
}

// @func 0x005220E8 AglDrawList::Marker
void AglDrawList::Marker(uint32_t tag)
{
    WriteHeader(kAglDraw_Marker, tag, tag & 0xFFFF);
}

// @func 0x005218D8 AglDrawList::SetWorldMatrix
void AglDrawList::SetWorldMatrix(const ape::Matrix44& world, uint32_t tag)
{
    if (m_worldValid && SameMatrix(m_world, world))
        return;
    m_world = world;
    m_worldValid = true;
    WriteHeader(kAglDraw_World, tag, tag & 0xFFFF);
    Write(&world, sizeof(world), 16);
}

void AglDrawList::SetWorldMatrix(const ape::Matrix34& world, uint32_t tag)
{
    SetWorldMatrix(ToMatrix44(world), tag);
}

// @func 0x0052173C AglDrawList::SetScissor
void AglDrawList::SetScissor(const uint16_t* rect, uint32_t tag)
{
    const uint32_t viewLeft = m_viewRect[0];
    const uint32_t viewTop = m_viewRect[1];
    const uint32_t viewRight = viewLeft + m_viewRect[2];
    const uint32_t viewBottom = viewTop + m_viewRect[3];
    uint32_t left = viewLeft, top = viewTop, right = viewRight, bottom = viewBottom;
    if (rect) {
        auto clamp = [](uint32_t v, uint32_t lo, uint32_t hi) { return v < lo ? lo : (v > hi ? hi : v); };
        left = clamp(rect[0], viewLeft, viewRight);
        right = clamp(uint32_t(rect[0]) + rect[2], viewLeft, viewRight);
        top = clamp(rect[1], viewTop, viewBottom);
        bottom = clamp(uint32_t(rect[1]) + rect[3], viewTop, viewBottom);
    }
    const uint16_t scissor[4] = { static_cast<uint16_t>(left), static_cast<uint16_t>(top),
                                  static_cast<uint16_t>(right - left), static_cast<uint16_t>(bottom - top) };
    if (m_scissorValid && std::memcmp(scissor, m_scissor, sizeof(scissor)) == 0)
        return;
    std::memcpy(m_scissor, scissor, sizeof(scissor));
    m_scissorValid = true;
    WriteHeader(kAglDraw_Scissor, 0, tag & 0xFFFF);
    Write(scissor, sizeof(scissor), 4);
}

// @func 0x00523310 AglDrawList::SetMaterial
void AglDrawList::SetMaterial(AglMaterial* material, uint32_t tag)
{
    if (material == m_material)
        return;
    m_material = material;
    // A material binds its first two textures.
    m_textures[0] = material->textures[0];
    m_textures[1] = material->textures[1];
    WriteHeader(kAglDraw_Material, tag, tag & 0xFFFF);
    Write(&material, sizeof(material), 4);
}

// @func 0x005233F8 AglDrawList::SetTexture
void AglDrawList::SetTexture(AglTexture* texture, int slot, uint32_t tag)
{
    if (texture == m_textures[slot])
        return;
    m_textures[slot] = texture;
    m_material = nullptr;
    WriteHeader(kAglDraw_Texture, tag, tag & 0xFFFF);
    Write(&texture, sizeof(texture), 4);
    const int32_t slotValue = slot;
    Write(&slotValue, sizeof(slotValue), 4);
}

// @func 0x005234DC AglDrawList::SetState
void AglDrawList::SetState(uint8_t value, uint32_t tag)
{
    if (value == m_state)
        return;
    m_state = value;
    WriteHeader(kAglDraw_State, tag, value);
    Write(&value, 1, 4);
}

// @func 0x00523584 AglDrawList::AppendList
void AglDrawList::AppendList(const AglDrawList* list, uint32_t start, uint32_t tag)
{
    WriteHeader(kAglDraw_AppendList, tag, tag & 0xFFFF);
    Write(&list, sizeof(list), 4);
    Write(&start, sizeof(start), 4);
    // The appended packets may change any state.
    InvalidateCache();
}

// @func 0x005236A0 AglDrawList::AllocPrimitive
void AglDrawList::AllocPrimitive(AglDrawPrimitive* out, uint8_t primitive, uint32_t count, uint32_t tag)
{
    std::vector<AglDrawVertex>& buffer = m_vertexBuffers[m_currentBuffer];
    if (m_vertexUsed + count > buffer.size())
        buffer.resize(m_vertexUsed + count);
    WriteHeader(kAglDraw_Primitive, tag, (count << 16) | (tag & 0xFFFF), m_vertexUsed, primitive);
    out->vertices = buffer.data() + m_vertexUsed;
    out->count = static_cast<uint16_t>(count);
    out->field_6 = 0;
    out->field_8 = 0;
    m_vertexUsed += count;
}

// @func 0x00523748 AglDrawList::EndPrimitive
void AglDrawList::EndPrimitive()
{
}

} // namespace agl
