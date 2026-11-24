// openTIT - Ape engine GUI core: resources, the object tree and globals.
#include "ape/level/ApeGui.h"

#include "ape/core/ApeString.h"
#include "platform/Platform.h"

#include <cstring>

namespace ape {

namespace {

// NOTE: hash of the "$..." path variable replaced by the language folder;
// the variable's spelling is not known.
constexpr uint32_t kLanguageVarHash = 0x243084BD;
constexpr uint32_t kPreloadHash = 0x17CFF9E9;      // "preload"
constexpr uint32_t kTextureFlagAHash = 0x051B7FEC;
constexpr uint32_t kTextureFlagBHash = 0x2E8401A3;

const char* s_languagePath = "";                    // 0x00F668A4
ApeGuiObject* s_rootObjects = nullptr;              // 0x00F66868
StringTable* s_stringTable = nullptr;               // 0x00F6687C

GuiPadFilter s_padFilter = nullptr;                 // 0x00F6686C
void* s_padFilterData = nullptr;                    // 0x00F66870
GuiFilter s_filter = nullptr;                       // 0x00F66874
void* s_filterData = nullptr;                       // 0x00F66878
void* s_slots[kGuiSlotCount] = {};                  // 0x010C2AD8

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
    if (--m_loadCount < 1 && m_loadState > kGuiLoad_None + 1) {
        Unload();
        m_loadState = kGuiLoad_None;
    }
    if (--m_refCount < 1)
        delete this;
}

void ApeGuiResource::AddLoadRef()
{
    if (++m_loadCount > 0 && m_loadState == kGuiLoad_None)
        m_loadState = Load();
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

// TODO(0x004D5EAC ApeGuiTexture::Load) needs the render texture manager
// (find by file name hash, else load m_path) and the sampler setup of
// 0x004D5C64.
uint16_t ApeGuiTexture::Load()
{
    return kGuiLoad_Failed;
}

// TODO(0x004D5F38 ApeGuiTexture::Unload) releases the three texture views
// and the texture.
void ApeGuiTexture::Unload()
{
    texture = nullptr;
    for (void*& sampler : samplers)
        sampler = nullptr;
}

// @func 0x004D5A4C CreateGuiTexture
ApeGuiTexture* CreateGuiTexture(xml::XmlDocument* doc, xml::XmlElement* element, void* owner)
{
    const char* name = element->GetString("name");
    const char* fileName = element->GetString("filename");
    bool preload = false;
    bool flagA = false;
    bool flagB = false;
    element->GetBool(kPreloadHash, &preload, false);
    element->GetBool(kTextureFlagAHash, &flagA, false);
    element->GetBool(kTextureFlagBHash, &flagB, false);
    if (!name || !fileName) {
        plat::Log(plat::LogLevel::Warning, "%s: GUI texture without name or filename\n",
                  doc ? doc->GetDocumentPath() : "");
        return nullptr;
    }

    auto* texture = new ApeGuiTexture(name, fileName, owner);
    texture->flag_51B7FEC = flagA;
    texture->flag_2E8401A3 = flagB;
    if (preload)
        texture->AddLoadRef();
    return texture;
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

} // namespace ape
