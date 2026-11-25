// openTIT - Ape engine GUI core: resources, the object tree and globals.
#include "ape/level/ApeGui.h"

#include "agl/AglDisplay.h"
#include "agl/AglDrawList.h"
#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"
#include "agl/AglTexture.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiProperties.h"
#include "ape/level/ApeGuiTypes.h"
#include "platform/Platform.h"

#include <cstdio>
#include <cstring>

namespace ape {

namespace {

// NOTE: hash of the "$..." path variable replaced by the language folder;
// the variable's spelling is not known.
constexpr uint32_t kLanguageVarHash = 0x243084BD;
constexpr uint32_t kPreloadHash = 0x17CFF9E9;      // "preload"
constexpr uint32_t kZTestHash = 0x051B7FEC;         // "ztest"
constexpr uint32_t kTextureFlagBHash = 0x2E8401A3;
constexpr uint32_t kFontFlagHash = 0x04704F4C;
constexpr uint32_t kGlyphMapHash = 0x0001293C;      // "map"
constexpr uint32_t kScaleHash = 0x04B1112A;         // "scale"
// NOTE: font set child element; "page" is the only short name matching.
constexpr uint32_t kFontSetPageHash = 0x00255A8F;
// NOTE: per-font vertical offset in a font set (name unknown).
constexpr uint32_t kFontOffsetHash = 0x7A600EB7;

const char* s_languagePath = "";                    // 0x00F668A4
ApeGuiObject* s_rootObjects = nullptr;              // 0x00F66868
StringTable* s_stringTable = nullptr;               // 0x00F6687C

GuiPadFilter s_padFilter = nullptr;                 // 0x00F6686C
void* s_padFilterData = nullptr;                    // 0x00F66870
GuiFilter s_filter = nullptr;                       // 0x00F66874
void* s_filterData = nullptr;                       // 0x00F66878
void* s_slots[kGuiSlotCount] = {};                  // 0x010C2AD8

agl::AglRenderTarget* s_guiTarget = nullptr;        // 0x00F6684C
agl::AglRenderTarget* s_gui3dTarget = nullptr;      // 0x00F66850
agl::AglRenderTarget* s_guiDrawTarget = nullptr;    // 0x00F6688C
agl::AglDrawList* s_drawListOverride = nullptr;     // 0x00F66890

bool IsIdentifierChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

// Re-applies the text of every text object in a subtree.
// @func 0x004D573C RefreshGuiText
void RefreshGuiText(ApeGuiObject* object)
{
    if (object->GetType() == kGuiObject_Text && object->textId)
        object->SetText(TempFormat("%s", object->textId), true);
    for (ApeGuiObject* child = object->firstChild; child; child = child->nextSibling)
        RefreshGuiText(child);
}

} // namespace

// ---------------------------------------------------------------------------
// ApeGuiResource
// ---------------------------------------------------------------------------

ApeGuiResource* ApeGuiResource::s_first = nullptr;

// @func 0x004D5274 ApeGuiResource::ApeGuiResource
// @func 0x004D54D4 ApeGuiResource::ApeGuiResource (thunk)
ApeGuiResource::ApeGuiResource(const char* name, const char* path, void* owner)
    : m_nameHash(HashNameNoCase(name)), m_owner(owner)
{
    // Copy the path, expanding "$<var>" references and remembering where the
    // file name starts.
    const size_t capacity = sizeof(m_path) - 1;
    size_t length = 0;
    size_t fileStart = 0;
    while (path && *path) {
        const char c = *path;
        if (c == '$') {
            char variable[32];
            size_t n = 0;
            for (++path; IsIdentifierChar(*path); ++path) {
                if (n < sizeof(variable) - 1)
                    variable[n++] = *path;
            }
            variable[n] = '\0';
            // The character that ended the name is handled normally.
            if (HashNameNoCase(variable) == kLanguageVarHash) {
                for (const char* s = s_languagePath; *s && length < 0x3F && length < capacity; ++s)
                    m_path[length++] = *s;
            }
            continue;
        }
        if (length < capacity)
            m_path[length++] = c;
        if (c == '/' || c == '\\')
            fileStart = length;
        ++path;
    }
    m_path[length] = '\0';
    m_fileNameHash = HashNameNoCase(m_path + fileStart);

    m_next = s_first;
    s_first = this;
}

// The texture and GUI file resource destructors add nothing to this one; the
// console linker kept them as byte-identical copies.
// @func 0x004D5878 ApeGuiResource::~ApeGuiResource
// @func 0x00794FD0 ApeGuiResource::~ApeGuiResource (copy 2, texture)
// @func 0x00795724 ApeGuiResource::~ApeGuiResource (copy 3, GUI file)
// @func 0x004D58E4 ApeGuiResource::~ApeGuiResource (deleting)
// @func 0x00795790 ApeGuiResource::~ApeGuiResource (deleting, copy 3, GUI file)
ApeGuiResource::~ApeGuiResource()
{
    for (ApeGuiResource** link = &s_first; *link; link = &(*link)->m_next) {
        if (*link == this) {
            *link = m_next;
            break;
        }
    }
}

// @func 0x004D5990 ApeGuiResource::Release
void ApeGuiResource::Release()
{
    ReleaseLoadRef();
    if (--m_refCount < 1)
        delete this;
}

void ApeGuiResource::AddLoadRef()
{
    if (++m_loadCount > 0 && m_loadState == kGuiLoad_None)
        m_loadState = Load();
}

void ApeGuiResource::ReleaseLoadRef()
{
    if (--m_loadCount < 1 && m_loadState > kGuiLoad_None + 1) {
        Unload();
        m_loadState = kGuiLoad_None;
    }
}

ApeGuiResource* ApeGuiResource::FindByName(uint32_t nameHash)
{
    for (ApeGuiResource* r = s_first; r; r = r->m_next)
        if (r->m_nameHash == nameHash)
            return r;
    return nullptr;
}

void SetGuiLanguagePath(const char* path)
{
    s_languagePath = path ? path : "";
}

// ---------------------------------------------------------------------------
// ApeGuiTexture
// ---------------------------------------------------------------------------

ApeGuiTexture::ApeGuiTexture(const char* name, const char* path, void* owner)
    : ApeGuiResource(name, path, owner)
{
}

// @func 0x0079503C ApeGuiTexture::~ApeGuiTexture (deleting)
ApeGuiTexture::~ApeGuiTexture() = default;

namespace {

// One material per GUI blend mode (alpha, additive, subtractive) drawing
// 'texture'. GUI materials ignore the depth buffer unless the resource asks
// for it.
void CreateGuiMaterials(agl::AglMaterial* (&materials)[3], agl::AglTexture* texture, bool ztest,
                        bool depthTest)
{
    for (uint32_t i = 0; i < 3; ++i) {
        agl::AglMaterial* material = agl::CreateSpriteMaterial(texture);
        materials[i] = material;
        ApplyGuiBlendMode(material, i);
        material->depthFunc = ztest ? agl::gpu::CompareFunc::LEqual : agl::gpu::CompareFunc::Always;
        material->depthTest = depthTest;
    }
}

} // namespace

// @func 0x004D5C64 ApeGuiTexture::CreateMaterials
void ApeGuiTexture::CreateMaterials()
{
    if (texture)
        CreateGuiMaterials(materials, texture, ztest, flag_2E8401A3);
}

// @func 0x004D5EAC ApeGuiTexture::Load
uint16_t ApeGuiTexture::Load()
{
    // Textures already loaded by the renderer are shared by file name.
    texture = agl::FindTexture(m_fileNameHash);
    if (texture)
        texture->AddRef();
    else
        texture = agl::TextureLoadFile(m_path, false);
    if (!texture)
        return kGuiLoad_Failed;
    CreateMaterials();
    return kGuiLoad_Loaded;
}

// @func 0x004D5F38 ApeGuiTexture::Unload
void ApeGuiTexture::Unload()
{
    for (agl::AglMaterial*& material : materials) {
        if (material) {
            material->Release();
            material = nullptr;
        }
    }
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
}

// @func 0x004D5A4C CreateGuiTexture
ApeGuiTexture* CreateGuiTexture(xml::XmlDocument* doc, xml::XmlElement* element, void* owner)
{
    const char* name = element->GetString("name");
    const char* fileName = element->GetString("filename");
    bool preload = false;
    bool ztest = false;
    bool flagB = false;
    element->GetBool(kPreloadHash, &preload, false);
    element->GetBool(kZTestHash, &ztest, false);
    element->GetBool(kTextureFlagBHash, &flagB, false);
    if (!name || !fileName) {
        plat::Log(plat::LogLevel::Warning, "%s: GUI texture without name or filename\n",
                  doc ? doc->GetDocumentPath() : "");
        return nullptr;
    }

    auto* texture = new ApeGuiTexture(name, fileName, owner);
    texture->ztest = ztest;
    texture->flag_2E8401A3 = flagB;
    if (preload)
        texture->AddLoadRef();
    return texture;
}

// ---------------------------------------------------------------------------
// ApeGuiBitmapFont
// ---------------------------------------------------------------------------

ApeGuiBitmapFont::ApeGuiBitmapFont(const char* name, const char* mapPath, void* owner,
                                   ApeGuiTexture* fontTexture, uint32_t first)
    : ApeGuiResource(name, mapPath, owner), texture(fontTexture), firstChar(first)
{
    texture->AddRef();
}

// @func 0x007950DC ApeGuiBitmapFont::~ApeGuiBitmapFont
// @func 0x007951B8 ApeGuiBitmapFont::~ApeGuiBitmapFont (deleting)
ApeGuiBitmapFont::~ApeGuiBitmapFont()
{
    texture->Release();
    if (charMap)
        delete[] charMap;
}

// TODO(0x004D6768 ApeGuiBitmapFont::Load) the glyph scan of the map bitmap
// (ReadBitmapPixel, marker pixels with a channel >= 0xF0 outline the glyphs,
// up to 256 GuiGlyph records, height = smallest row pitch) is not done. The
// texture load reference and the materials are.
uint16_t ApeGuiBitmapFont::Load()
{
    texture->AddLoadRef();
    if (texture->GetLoadState() == kGuiLoad_Loaded) {
        for (uint32_t i = 0; i < 3; ++i) {
            agl::AglTexture* image = texture->texture;
            if (!image)
                image = agl::GetDefaultTexture(0);
            agl::AglMaterial* material = agl::CreateSpriteMaterial(image);
            materials[i] = material;
            ApplyGuiBlendMode(material, i);
            material->depthFunc = ztest ? agl::gpu::CompareFunc::LEqual : agl::gpu::CompareFunc::Always;
            material->depthTest = flag_2E8401A3;
        }
    }
    return glyphCount ? kGuiLoad_Loaded : kGuiLoad_Failed;
}

// @func 0x004D70B0 ApeGuiBitmapFont::Unload
void ApeGuiBitmapFont::Unload()
{
    if (glyphCount) {
        delete[] glyphs;
        glyphs = nullptr;
        glyphCount = 0;
    }
    // NOTE: the materials are released but the pointers are kept, as in the
    // original; Load overwrites them.
    for (agl::AglMaterial* material : materials)
        if (material)
            material->Release();
    // Drop the load reference taken by Load.
    texture->ReleaseLoadRef();
}

// @func 0x004D601C CreateGuiBitmapFont
ApeGuiBitmapFont* CreateGuiBitmapFont(xml::XmlDocument* doc, xml::XmlElement* element, void* owner)
{
    const char* path = doc ? doc->GetDocumentPath() : "";
    const char* name = element->GetString("name");
    const char* textureName = element->GetString("texture");
    const char* mapFileName = element->GetString("mapfilename");
    const char* firstCharText = element->GetString("firstchar");
    bool preload = false;
    bool fontFlag = false;
    bool ztest = false;
    bool flagB = false;
    element->GetBool(kPreloadHash, &preload, false);
    element->GetBool(kFontFlagHash, &fontFlag, false);
    element->GetBool(kZTestHash, &ztest, false);
    element->GetBool(kTextureFlagBHash, &flagB, false);
    if (!name) {
        plat::Log(plat::LogLevel::Warning, "%s: GUI font without name\n", path);
        return nullptr;
    }
    if (!textureName) {
        plat::Log(plat::LogLevel::Warning, "%s: GUI font '%s' without texture\n", path, name);
        return nullptr;
    }
    auto* texture = static_cast<ApeGuiTexture*>(ApeGuiResource::FindByName(HashNameNoCase(textureName)));
    if (!texture) {
        plat::Log(plat::LogLevel::Warning, "%s: GUI font '%s': unknown texture '%s'\n", path, name,
                  textureName);
        return nullptr;
    }
    if (!mapFileName) {
        plat::Log(plat::LogLevel::Warning, "%s: GUI font '%s' without mapfilename\n", path, name);
        return nullptr;
    }

    // First character: "\uXXXX", a plain ASCII character or a single byte.
    uint32_t firstChar = ' ';
    if (firstCharText) {
        const uint8_t c = static_cast<uint8_t>(firstCharText[0]);
        if (c == '\\' && (firstCharText[1] == 'u' || firstCharText[1] == 'U')) {
            if (std::sscanf(firstCharText + 2, "%x", &firstChar) != 1) {
                plat::Log(plat::LogLevel::Warning, "%s: GUI font '%s': bad firstchar\n", path, name);
                firstChar = ' ';
            }
        } else if (c < 0x80 || firstCharText[1] == 0) {
            firstChar = c;
        } else {
            plat::Log(plat::LogLevel::Warning, "%s: GUI font '%s': bad firstchar\n", path, name);
        }
    }

    // Optional code point list: glyph i draws the i-th listed code point.
    uint8_t* charMap = nullptr;
    xml::XmlElement* mapElement = element->FirstChildElement(kGlyphMapHash);
    const char* text = mapElement ? mapElement->GetText() : nullptr;
    if (text) {
        charMap = new uint8_t[0x10000];
        std::memset(charMap, 0xFF, 0x10000);
        char token[40];
        size_t length = 0;
        uint32_t glyph = 0;
        for (const char* p = text; glyph < 0xFF; ++p) {
            const char c = *p;
            const bool separator = c == 0 || c == ' ' || c == '\t' || c == '\n' || c == ',';
            if (!separator) {
                if (length + 1 < sizeof(token))
                    token[length++] = c;
                continue;
            }
            if (length) {
                token[length] = 0;
                length = 0;
                // NOTE: the original only accepts "\u" (the 'U' test checks the
                // backslash again).
                const char* digits = (token[0] == '\\' && token[1] == 'u') ? token + 2 : token;
                uint32_t codePoint = 0;
                std::sscanf(digits, "%x", &codePoint);
                if (codePoint == 0 || codePoint > 0xFFFF)
                    plat::Log(plat::LogLevel::Warning, "%s: GUI font '%s': bad map entry '%s'\n", path,
                              name, token);
                else
                    charMap[codePoint] = static_cast<uint8_t>(glyph++);
            }
            if (c == 0)
                break;
        }
    }

    auto* font = new ApeGuiBitmapFont(name, mapFileName, owner, texture, firstChar);
    font->flag_4704F4C = fontFlag;
    font->ztest = ztest;
    font->flag_2E8401A3 = flagB;
    font->charMap = charMap;
    if (preload)
        font->AddLoadRef();
    return font;
}

// ---------------------------------------------------------------------------
// ApeGuiFontSet
// ---------------------------------------------------------------------------

ApeGuiFontSet::ApeGuiFontSet(const char* name, void* owner, const float setScale[2])
    : ApeGuiResource(name, "", owner)
{
    scale[0] = setScale[0];
    scale[1] = setScale[1];
    for (float& s : fontScales)
        s = 1.0f;
}

// @func 0x007952B0 ApeGuiFontSet::~ApeGuiFontSet
// @func 0x007953AC ApeGuiFontSet::~ApeGuiFontSet (deleting)
ApeGuiFontSet::~ApeGuiFontSet()
{
    while (fontCount)
        fonts[--fontCount]->Release();
}

// @func 0x007954D0 ApeGuiFontSet::Load
uint16_t ApeGuiFontSet::Load()
{
    for (uint32_t i = 0; i < fontCount; ++i)
        fonts[i]->AddLoadRef();
    for (uint32_t i = 0; i < fontCount; ++i) {
        const uint16_t state = fonts[i]->GetLoadState();
        if (state != kGuiLoad_Loaded)
            return state;
        // Fonts after the first are scaled to the first font's line height.
        if (i != 0)
            fontScales[i] *= float(fonts[0]->GetHeight()) / float(fonts[i]->GetHeight());
    }
    return kGuiLoad_Loaded;
}

// @func 0x00795650 ApeGuiFontSet::Unload
void ApeGuiFontSet::Unload()
{
    for (uint32_t i = 0; i < fontCount; ++i)
        fonts[i]->ReleaseLoadRef();
}

void ApeGuiFontSet::AddFont(ApeGuiBitmapFont* font, float fontScale, float offset)
{
    const uint32_t index = fontCount;
    fonts[index] = font;
    font->AddRef();
    fontScales[index] = fontScale;
    fontOffsets[index] = offset;
    if (GetLoadState() != kGuiLoad_None) {
        font->AddLoadRef();
        if (fontCount != 0)
            fontScales[index] *= float(fonts[0]->GetHeight()) / float(font->GetHeight());
    }
    ++fontCount;
}

// @func 0x004D71D0 CreateGuiFontSet
ApeGuiFontSet* CreateGuiFontSet(xml::XmlDocument* doc, xml::XmlElement* element, void* owner)
{
    const char* path = doc ? doc->GetDocumentPath() : "";
    const char* name = element->GetString("name");
    // NOTE: the original passes an uninitialised default; 1,1 is assumed.
    const float kUnitScale[2] = { 1.0f, 1.0f };
    float scale[2] = { 1.0f, 1.0f };
    element->GetVec2(kScaleHash, scale, kUnitScale);
    bool preload = false;
    element->GetBool(kPreloadHash, &preload, false);
    if (!name) {
        plat::Log(plat::LogLevel::Warning, "%s: GUI font set without name\n", path);
        return nullptr;
    }

    auto* set = new ApeGuiFontSet(name, owner, scale);
    if (preload)
        set->AddLoadRef();

    for (xml::XmlElement* page = element->FirstChildElement(kFontSetPageHash); page;
         page = page->NextSiblingElement(kFontSetPageHash)) {
        const char* fontName = page->GetString("name");
        if (!fontName) {
            plat::Log(plat::LogLevel::Warning, "%s: GUI font set '%s': page without name\n", path, name);
            continue;
        }
        auto* font = static_cast<ApeGuiBitmapFont*>(ApeGuiResource::FindByName(HashNameNoCase(fontName)));
        if (!font) {
            plat::Log(plat::LogLevel::Warning, "%s: GUI font set '%s': unknown font '%s'\n", path, name,
                      fontName);
            continue;
        }
        if (set->fontCount >= ApeGuiFontSet::kMaxFonts)
            break;   // NOTE: the original does not check the limit
        float fontScale = 1.0f;
        float offset = 0.0f;
        page->GetFloat(kScaleHash, &fontScale, 1.0f);
        page->GetFloat(kFontOffsetHash, &offset, 0.0f);
        set->AddFont(font, fontScale, offset);
    }
    return set;
}

// ---------------------------------------------------------------------------
// Object tree
// ---------------------------------------------------------------------------

ApeGuiObject*& GuiRootObjects()
{
    return s_rootObjects;
}

// @func 0x004D54FC FindGuiChild
ApeGuiObject* FindGuiChild(uint32_t nameHash, ApeGuiObject* parent)
{
    for (ApeGuiObject* o = parent ? parent->firstChild : s_rootObjects; o; o = o->nextSibling)
        if (o->nameHash == nameHash)
            return o;
    return nullptr;
}

// @func 0x004D5544 FindGuiObject
ApeGuiObject* FindGuiObject(const char* path, ApeGuiObject* scope)
{
    if (!path || !*path)
        return scope;

    char token[64];
    size_t length = 0;
    const char* p = path;
    while (*p) {
        if (*p != '.') {
            if (length < sizeof(token) - 1)
                token[length++] = *p;
            ++p;
            continue;
        }
        if (p != path) {
            // End of a path component: descend and resolve the rest.
            token[length] = '\0';
            ApeGuiObject* next = FindGuiChild(HashNameNoCase(token), scope);
            return next ? FindGuiObject(p + 1, next) : nullptr;
        }
        if (p[1] == '.') {
            // "..", "...": climb one level per extra dot.
            for (++p; *p == '.'; ++p)
                scope = scope ? scope->parent : nullptr;
        } else {
            // A single leading dot restarts from the root.
            ++p;
            scope = nullptr;
        }
    }
    token[length] = '\0';
    return FindGuiChild(HashNameNoCase(token), scope);
}

// @func 0x004D3700 BuildGuiObjectPath
void BuildGuiObjectPath(const ApeGuiObject* object, char** cursor)
{
    if (object->parent)
        BuildGuiObjectPath(object->parent, cursor);
    // The caller provides a buffer large enough for the whole path.
    const size_t length = std::strlen(object->name);
    (*cursor)[0] = '.';
    std::memcpy(*cursor + 1, object->name, length + 1);
    *cursor += length + 1;
}

// @func 0x004D580C SetGuiStringTable
void SetGuiStringTable(StringTable* table)
{
    s_stringTable = table;
    for (ApeGuiObject* o = s_rootObjects; o; o = o->nextSibling)
        RefreshGuiText(o);
}

StringTable* GetGuiStringTable()
{
    return s_stringTable;
}

// ---------------------------------------------------------------------------
// Game hooks
// ---------------------------------------------------------------------------

// @func 0x004D5048 SetGuiPadFilter
void SetGuiPadFilter(GuiPadFilter filter, void* userData)
{
    s_padFilter = filter;
    s_padFilterData = userData;
}

// @func 0x004D505C SetGuiFilter
void SetGuiFilter(GuiFilter filter, void* userData)
{
    s_filter = filter;
    s_filterData = userData;
}

// @func 0x004D5714 SetGuiSlot
void SetGuiSlot(int index, void* value)
{
    if (index >= 0 && index < kGuiSlotCount)
        s_slots[index] = value;
}

// @func 0x004D5728 GetGuiSlot
void* GetGuiSlot(int index)
{
    return (index >= 0 && index < kGuiSlotCount) ? s_slots[index] : nullptr;
}

// @func 0x004D65EC ReadBitmapPixel
void ReadBitmapPixel(uint8_t outRgba[4], const uint8_t* pixels, const uint8_t* palette, uint32_t x,
                     int32_t y, int32_t stride, int32_t height, int32_t bitsPerPixel, bool topDown)
{
    if (!topDown)
        y = height - y - 1;
    const uint32_t row = uint32_t(y) * uint32_t(stride);

    const uint8_t* bgr = nullptr;
    switch (bitsPerPixel) {
    case 24:
        bgr = pixels + (row + x) * 3;
        break;
    case 8:
        bgr = palette + pixels[row + x] * 4;
        break;
    case 4: {
        // Even pixels live in the high nibble.
        const uint8_t packed = pixels[(row >> 1) + (x >> 1)];
        const uint8_t index = (x & 1) ? (packed & 0x0F) : (packed >> 4);
        bgr = palette + index * 4;
        break;
    }
    case 1: {
        // NOTE: bit 0 is the leftmost pixel here (BMP uses bit 7).
        const uint8_t packed = pixels[(row >> 3) + (x >> 3)];
        const uint8_t index = (packed & (1u << (x & 7))) ? 1 : 0;
        bgr = palette + index * 4;
        break;
    }
    default:
        break;
    }

    if (bgr) {
        outRgba[0] = bgr[2];
        outRgba[1] = bgr[1];
        outRgba[2] = bgr[0];
    } else {
        outRgba[0] = outRgba[1] = outRgba[2] = 0;
    }
    outRgba[3] = 0xFF;
}

// ---------------------------------------------------------------------------
// Rendering state
// ---------------------------------------------------------------------------

namespace {

// @func 0x004D5870 GuiPreRenderCallback
// Render callbacks of the GUI target (before / after); nothing to do.
void GuiPreRenderCallback(void*)
{
}

// @func 0x004D5874 GuiPostRenderCallback
void GuiPostRenderCallback(void*)
{
}

} // namespace

// @func 0x004D35AC InitGuiRendering
bool InitGuiRendering(const int16_t* viewport)
{
    s_guiTarget = agl::AglRenderTarget::CreatePrimary(2, false);
    StrCopyBounded(s_guiTarget->name, "RenderTarget_GUI", sizeof(s_guiTarget->name));
    s_guiTarget->SetProjectionType(agl::kAglProjection_Ortho);
    s_guiTarget->AddRenderCallback(0, GuiPreRenderCallback, nullptr, 0);
    s_guiTarget->AddRenderCallback(1, GuiPostRenderCallback, nullptr, 0);

    const agl::AglDisplaySettings& display = agl::GetDisplaySettings();
    g_guiViewport = { 0, 0, (int16_t)display.width, (int16_t)display.height };
    if (viewport)
        g_guiViewport = { viewport[0], viewport[1], viewport[2], viewport[3] };
    const uint16_t rect[4] = { (uint16_t)g_guiViewport.x, (uint16_t)g_guiViewport.y,
                               (uint16_t)g_guiViewport.width, (uint16_t)g_guiViewport.height };
    s_guiTarget->SetViewport(rect);

    s_stringTable = nullptr;
    s_drawListOverride = nullptr;
    for (void*& slot : s_slots)
        slot = nullptr;
    return true;
}

// @func 0x004D4104 GetGuiScreenSize
void GetGuiScreenSize(float out[2])
{
    const agl::AglDisplaySettings& display = agl::GetDisplaySettings();
    out[0] = (float)display.width;
    out[1] = (float)display.height;
}

// @func 0x004D3DA4 SendGuiEvent
bool SendGuiEvent(uint32_t type, uint32_t pad, ApeGuiObject* object, uint32_t param)
{
    if (!s_filter)
        return true;
    const GuiEvent event = { type, pad, s_filterData, object, param };
    return s_filter(&event);
}

agl::AglDrawList& GetGuiDrawList()
{
    return s_drawListOverride ? *s_drawListOverride : agl::GetDrawList();
}

void SetGuiDrawListOverride(agl::AglDrawList* list)
{
    s_drawListOverride = list;
}

agl::AglRenderTarget* GetGuiRenderTarget()
{
    return s_guiTarget;
}

agl::AglRenderTarget* GetGui3dRenderTarget()
{
    return s_gui3dTarget;
}

void SetGui3dRenderTarget(agl::AglRenderTarget* target)
{
    s_gui3dTarget = target;
}

agl::AglRenderTarget* GetGuiDrawTarget()
{
    return s_guiDrawTarget;
}

void SetGuiDrawTarget(agl::AglRenderTarget* target)
{
    s_guiDrawTarget = target;
}

} // namespace ape
