// openTIT - ApeDebug drawing: the debug font, the primitive pools and the
// screen-space renderer of text, lines and rectangles.
//
// The console keeps two primitive pools (0x01164E50) plus pointers to the
// pool being filled (0x01164E58) and the pool being drawn (0x01164E5C). The
// render thread draws the previous frame's pool; at the end of a frame the
// pools swap and the pool that was drawn is emptied.
#include "ape/memory/ApeDebugPrim.h"

#include "agl/AglDisplay.h"
#include "agl/AglDrawList.h"
#include "agl/AglPrimitives.h"
#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"
#include "agl/AglShaderClass.h"
#include "agl/AglTexture.h"
#include "ape/world/ApeBase.h"
#include "platform/Platform.h"

#include <cstring>
#include <vector>

namespace ape {
namespace ApeDebug {

// One glyph of the bitmap font (0x10 bytes in the descriptor): its texel
// rectangle, drawn width and pen advance.
struct FontGlyph {
    int16_t u0, v0, u1, v1;
    int16_t field_8;
    int16_t width;
    int16_t advance;
    int16_t field_E;
};

// The font descriptor is stored big-endian: version, line height, three
// more metrics, the highest mapped character, the character -> glyph table
// (count + 1 entries, 0 = not mapped), four bytes and the glyph records.
struct Font {
    Font(const uint8_t* descriptor, agl::AglTexture* texture);

    agl::AglTexture* texture = nullptr;     // +0x04
    agl::AglMaterial* material = nullptr;   // +0x08
    float lineHeight = 0.0f;                // +0x0C
    float field_10 = 0.0f;
    float field_14 = 0.0f;
    float field_18 = 0.0f;
    uint16_t lastChar = 0;                  // +0x1C
    std::vector<uint16_t> charMap;          // +0x20
    uint8_t field_24[4] = {};
    std::vector<FontGlyph> glyphs;          // +0x28

    const FontGlyph& Glyph(unsigned char c) const;
};

namespace {

constexpr float kTabWidth = 30.0f;

// Debug drawing state (0x01164A88..0x01164A98, 0x01164E50..0x01164E5C).
struct DebugState {
    uint32_t field_A88 = 0;                 // NOTE: set to 0x01000000 by Init; readers not found
    Font* font = nullptr;
    agl::AglMaterial* shapeMaterial = nullptr;
    agl::AglSubMesh* sphereMesh = nullptr;
    agl::AglSubMesh* boxMesh = nullptr;
    PrimPool* pools[2] = {};
    PrimPool* current = nullptr;            // being filled
    PrimPool* previous = nullptr;           // being drawn
};
DebugState g_debug;

PrimPool g_fallbackPool;   // used when AllocPrim runs before Init

struct FontData {
    const void* descriptor = nullptr;
    const void* bmp = nullptr;
    uint32_t bmpSize = 0;
};
FontData g_fontData;

uint16_t ReadU16(const uint8_t* p)
{
    return uint16_t((p[0] << 8) | p[1]);
}

uint32_t ReadU32(const uint8_t* p)
{
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}

float ReadF32(const uint8_t* p)
{
    const uint32_t bits = ReadU32(p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

// Alpha blended, drawn from both sides.
void SetAlphaBlend(agl::AglMaterial* material)
{
    material->blendEnable = true;
    material->blendSrc = agl::gpu::BlendFactor::SrcAlpha;
    material->blendEquation = agl::gpu::BlendEquation::Add;
    material->blendDst = agl::gpu::BlendFactor::OneMinusSrcAlpha;
}

// The second eye of a frame-packed stereo frame draws the same primitives
// again, so they are only retired after the right eye.
bool CanRetirePrims()
{
    return !(agl::GetDisplaySettings().stereoOutput && agl::GetStereoMode() == agl::kAglStereo_Left);
}

void WriteVertex(agl::AglDrawVertex& v, float x, float y, float z, const uint8_t color[4], float u, float vCoord)
{
    v.position[0] = x;
    v.position[1] = y;
    v.position[2] = z;
    std::memcpy(v.color, color, 4);
    v.uv[0] = u;
    v.uv[1] = vCoord;
}

// @func 0x00569CDC ApeDebug::DrawLine
void DrawLine(const Prim& prim)
{
    agl::AglDrawList& list = agl::GetDrawList();
    if (!list.HasRoomFor(2))
        return;
    list.SetMaterial(agl::GetDebugMaterial(0), 0);
    list.SetWorldMatrix(world::kWorldIdentity, 0);
    agl::AglDrawPrimitive primitive;
    list.AllocPrimitive(&primitive, static_cast<uint8_t>(agl::gpu::PrimitiveType::Lines), 2, 0);
    const float z = -prim.line.depth;
    WriteVertex(primitive.vertices[0], prim.line.x0, prim.line.y0, z, prim.color, 0.0f, 0.0f);
    WriteVertex(primitive.vertices[1], prim.line.x1, prim.line.y1, z, prim.color, 0.0f, 0.0f);
    primitive.field_6 += 2;
    list.EndPrimitive();
}

// @func 0x00569EC8 ApeDebug::DrawRect
void DrawRect(const Prim& prim)
{
    agl::AglDrawList& list = agl::GetDrawList();
    if (!list.HasRoomFor(4))
        return;
    list.SetMaterial(agl::GetDebugMaterial(0), 0);
    list.SetWorldMatrix(world::kWorldIdentity, 0);
    const float left = prim.rect.x, top = prim.rect.y;
    const float right = left + prim.rect.width, bottom = top + prim.rect.height;
    const float corners[4][2] = { { right, top }, { left, top }, { right, bottom }, { left, bottom } };
    agl::AglDrawPrimitive primitive;
    list.AllocPrimitive(&primitive, static_cast<uint8_t>(agl::gpu::PrimitiveType::TriangleStrip), 4, 0);
    const float z = -prim.rect.depth;
    for (int i = 0; i < 4; ++i)
        WriteVertex(primitive.vertices[i], corners[i][0], corners[i][1], z, prim.color, 0.0f, 0.0f);
    primitive.field_6 += 4;
    list.EndPrimitive();
}

// Number of characters of `text` that produce a glyph (tabs only move the pen).
int CountGlyphs(const char* text)
{
    int count = 0;
    for (const char* c = text; *c; ++c) {
        if (*c != '\t')
            ++count;
    }
    return count;
}

// @func 0x00565550 ApeDebug::DrawText
// Two triangles per character, the pen starting at `origin`; tabs advance
// the pen to the next multiple of 30 units.
void DrawText(float scale, const Font& font, agl::AglDrawList& list, const float origin[3], const char* text,
              const uint8_t color[4])
{
    const int glyphCount = CountGlyphs(text);
    if (glyphCount == 0)
        return;
    agl::AglDrawPrimitive primitive;
    list.AllocPrimitive(&primitive, static_cast<uint8_t>(agl::gpu::PrimitiveType::Triangles), uint32_t(glyphCount) * 6, 0);
    agl::AglDrawVertex* out = primitive.vertices;
    const float texelU = 1.0f / float(font.texture->width);
    const float texelV = 1.0f / float(font.texture->height);
    float x = origin[0];
    const float top = origin[1];
    const float bottom = scale * font.lineHeight + top;
    constexpr float z = -1.0f;
    for (const char* c = text; *c; ++c) {
        if (*c == '\t') {
            float stop = kTabWidth;
            while (stop <= x)
                stop += kTabWidth;
            x = stop;
            continue;
        }
        const FontGlyph& glyph = font.Glyph(static_cast<unsigned char>(*c));
        const float u0 = float(glyph.u0) * texelU, u1 = float(glyph.u1) * texelU;
        const float v0 = float(glyph.v0) * texelV, v1 = float(glyph.v1) * texelV;
        const float right = scale * float(glyph.width) + x;
        WriteVertex(*out++, right, top, z, color, u1, v0);
        WriteVertex(*out++, x, top, z, color, u0, v0);
        WriteVertex(*out++, x, bottom, z, color, u0, v1);
        WriteVertex(*out++, right, top, z, color, u1, v0);
        WriteVertex(*out++, x, bottom, z, color, u0, v1);
        WriteVertex(*out++, right, bottom, z, color, u1, v1);
        x = scale * float(glyph.advance) + x;
    }
    list.EndPrimitive();
}

} // namespace

PrimPool& CurrentPool()
{
    return g_debug.current ? *g_debug.current : g_fallbackPool;
}

// @func 0x00565410 ApeDebug::Font::Font
Font::Font(const uint8_t* descriptor, agl::AglTexture* fontTexture)
{
    material = agl::CreateSpriteMaterial(fontTexture);
    texture = fontTexture;
    SetAlphaBlend(material);
    material->cullFrontFace = 0;
    lineHeight = ReadF32(descriptor + 0x04);
    field_10 = ReadF32(descriptor + 0x08);
    field_14 = ReadF32(descriptor + 0x0C);
    field_18 = ReadF32(descriptor + 0x10);
    lastChar = ReadU16(descriptor + 0x14);
    const uint8_t* map = descriptor + 0x16;
    charMap.resize(size_t(lastChar) + 1);
    for (size_t i = 0; i <= lastChar; ++i)
        charMap[i] = ReadU16(map + i * 2);
    const uint8_t* extra = map + (size_t(lastChar) + 1) * 2;
    std::memcpy(field_24, extra, sizeof(field_24));
    // NOTE: the descriptor does not store the glyph count; the highest
    // index used by the map bounds it.
    uint16_t highest = 0;
    for (uint16_t index : charMap)
        highest = index > highest ? index : highest;
    const uint8_t* records = extra + 4;
    glyphs.resize(size_t(highest) + 1);
    for (size_t i = 0; i < glyphs.size(); ++i) {
        const uint8_t* r = records + i * 0x10;
        int16_t* fields = &glyphs[i].u0;
        for (int f = 0; f < 8; ++f)
            fields[f] = static_cast<int16_t>(ReadU16(r + f * 2));
    }
}

const FontGlyph& Font::Glyph(unsigned char c) const
{
    if (c > lastChar || charMap[c] == 0)
        return glyphs[0];
    return glyphs[charMap[c]];
}

void SetFontData(const void* descriptor, const void* bmp, uint32_t bmpSize)
{
    g_fontData.descriptor = descriptor;
    g_fontData.bmp = bmp;
    g_fontData.bmpSize = bmpSize;
}

// @func 0x0056A944 ApeDebug::Init
bool Init()
{
    g_debug.font = nullptr;
    g_debug.shapeMaterial = nullptr;
    g_debug.sphereMesh = nullptr;
    g_debug.boxMesh = nullptr;
    g_debug.field_A88 = 0x01000000;
    // On the console the font image is linked into the executable.
    agl::AglTexture* texture = nullptr;
    if (g_fontData.bmp)
        texture = agl::TextureCreateFromMemory(g_fontData.bmp, g_fontData.bmpSize, "Bmp", "MEM:ApeDebugFont");
    if (!texture || !g_fontData.descriptor)
        return false;

    g_debug.font = new Font(static_cast<const uint8_t*>(g_fontData.descriptor), texture);
    for (PrimPool*& pool : g_debug.pools) {
        pool = new PrimPool();
        pool->Get();
    }
    g_debug.current = g_debug.pools[0];
    g_debug.previous = nullptr;

    agl::AglMaterial* material = agl::AglMaterial::CreateWithClass(agl::GetShaderClass(0x03DE9E33u));   // "Debug"
    g_debug.shapeMaterial = material;
    SetAlphaBlend(material);
    material->depthTest = false;
    g_debug.sphereMesh = agl::CreateIndexedSphereMesh(material, 8, 8);
    g_debug.boxMesh = agl::CreateBoxMesh(material, true);

    plat::Log(plat::LogLevel::Info, "*********************************************************\n");
    plat::Log(plat::LogLevel::Info, "APE_DEBUG enabled in APE_FINAL\n");
    plat::Log(plat::LogLevel::Info, "*********************************************************\n");
    return true;
}

// @func 0x0056AC78 ApeDebug::Shutdown
bool Shutdown()
{
    // NOTE: only the pool holders are freed; the primitive lists they
    // created stay allocated (as on the console).
    for (PrimPool*& pool : g_debug.pools) {
        delete pool;
        pool = nullptr;
    }
    g_debug.current = nullptr;
    g_debug.previous = nullptr;
    return true;
}

// @func 0x0056AD18 ApeDebug::RetireWorldPrims
void RetireWorldPrims()
{
    if (!g_debug.previous)
        return;
    const agl::AglRenderTarget* view = agl::GetRenderingView();
    // NOTE: bit 0x20 of the view's field 0x100 enables the 3D debug primitives.
    if (!view || view->pass != 0 || (view->field_100 & 0x20) == 0)
        return;
    const bool retire = CanRetirePrims();
    PrimList& list = g_debug.previous->Get();
    for (PrimList::Node* node = list.First(); node; node = node->next) {
        Prim* prim = PrimList::Data(node);
        const int32_t type = prim->type;
        if (type != kPrim_Rect && type != kPrim_Dead && type != kPrim_Line && retire && type != kPrim_Text)
            prim->type = kPrim_Dead;
    }
    agl::GetDrawList().SetState(0, 0);
}

// @func 0x0056AEFC ApeDebug::DrawScreenPrims
void DrawScreenPrims()
{
    if (!g_debug.previous)
        return;
    const agl::AglRenderTarget* view = agl::GetRenderingView();
    // NOTE: bit 0x04 of the view's field 0x100 enables the screen primitives.
    if (!view || view->pass != 2 || view->projectionType != agl::kAglProjection_Ortho ||
        (view->field_100 & 4) == 0)
        return;
    const bool retire = CanRetirePrims();
    agl::AglDrawList& list = agl::GetDrawList();
    PrimList& prims = g_debug.previous->Get();
    bool needTextState = true;
    // Oldest first, so later primitives draw on top.
    for (PrimList::Node* node = prims.Last(); node; node = node->prev) {
        Prim* prim = PrimList::Data(node);
        switch (prim->type) {
        case kPrim_Text: {
            if (needTextState) {
                list.SetWorldMatrix(world::kWorldIdentity, 0);
                list.SetMaterial(g_debug.font->material, 0);
                needTextState = false;
            }
            const uint32_t vertices = uint32_t(std::strlen(prim->text.text)) * 6;
            if (list.HasRoomFor(vertices)) {
                const float origin[3] = { prim->text.position[0], prim->text.position[1], 1.0f };
                DrawText(prim->text.scale, *g_debug.font, list, origin, prim->text.text, prim->color);
            }
            if (retire)
                prim->type = kPrim_Dead;
            break;
        }
        case kPrim_Line:
            DrawLine(*prim);
            if (retire)
                prim->type = kPrim_Dead;
            needTextState = true;
            break;
        case kPrim_Rect:
            DrawRect(*prim);
            if (retire)
                prim->type = kPrim_Dead;
            needTextState = true;
            break;
        default:
            break;
        }
    }
    list.SetState(0, 0);
}

// @func 0x0056B2B8 ApeDebug::FreeCurrent
void FreeCurrent()
{
    PrimList& list = CurrentPool().Get();
    for (PrimList::Node* node = list.First(); node;) {
        PrimList::Node* next = node->next;
        list.Free(PrimList::Data(node));
        node = next;
    }
}

// @func 0x0056B4E8 ApeDebug::SwapBuffers
void SwapBuffers()
{
    g_debug.previous = g_debug.current;
    g_debug.current = (g_debug.current == g_debug.pools[1]) ? g_debug.pools[0] : g_debug.pools[1];
}

// @func 0x0056B57C ApeDebug::Clear
void Clear()
{
    FreeCurrent();
    SwapBuffers();
    FreeCurrent();
    SwapBuffers();
}

} // namespace ApeDebug
} // namespace ape
