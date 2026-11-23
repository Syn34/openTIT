// openTIT - Ape engine GUI core: resources, the object tree and globals.
//
// GUI screens are described by GUI files (compiled XML). Loading one creates
// resources (textures, bitmap fonts, ...) and a tree of GUI objects. Both are
// kept in process-wide intrusive lists:
//   * resources form a singly linked list, newest first;
//   * root objects form a sibling list, each object owns a child list.
// Objects are addressed by dotted paths of their names ("Menu.Title").
#pragma once

#include "ape/xml/ApeXml.h"

#include <cstdint>

namespace ape {

class StringTable;

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

enum GuiLoadState : uint16_t {
    kGuiLoad_None   = 0,
    kGuiLoad_Loaded = 2,
    kGuiLoad_Failed = 3,
};

// Reference counted resource shared by GUI objects. Two counts are kept: the
// object lifetime (refCount) and how many users need the data resident
// (loadCount); the data is loaded on the first load reference.
class ApeGuiResource {
public:
    // 'path' may contain "$<var>" references; see SetGuiLanguagePath().
    // 'owner' is the GUI file that created the resource.
    ApeGuiResource(const char* name, const char* path, void* owner);
    virtual ~ApeGuiResource();
    ApeGuiResource(const ApeGuiResource&) = delete;
    ApeGuiResource& operator=(const ApeGuiResource&) = delete;

    // Drops one load reference and one object reference; unloads and
    // deletes as the counts reach zero.
    virtual void Release();
    // Loads the data; returns the new GuiLoadState.
    virtual uint16_t Load() { return kGuiLoad_Failed; }
    virtual void Unload() {}

    // Takes a load reference (loads the data on the first one).
    void AddLoadRef();
    void AddRef() { ++m_refCount; }

    const char* GetPath() const { return m_path; }
    uint32_t GetNameHash() const { return m_nameHash; }
    uint32_t GetFileNameHash() const { return m_fileNameHash; }
    uint16_t GetLoadState() const { return m_loadState; }
    void* GetOwner() const { return m_owner; }
    ApeGuiResource* GetNext() const { return m_next; }

    static ApeGuiResource* First() { return s_first; }
    static ApeGuiResource* FindByName(uint32_t nameHash);

protected:
    int16_t m_refCount = 1;          // +0x04
    char m_path[0x42] = {};          // +0x06 resolved file path
    uint32_t m_fileNameHash = 0;     // +0x48 hash of the file name part of m_path
    uint32_t m_nameHash = 0;         // +0x4C
    int16_t m_loadCount = 0;         // +0x50
    uint16_t m_loadState = kGuiLoad_None; // +0x52
    void* m_owner = nullptr;         // +0x54
    ApeGuiResource* m_next = nullptr;// +0x58

private:
    static ApeGuiResource* s_first;  // 0x00F66864
};

// A texture used by GUI sprites.
class ApeGuiTexture : public ApeGuiResource {
public:
    ApeGuiTexture(const char* name, const char* path, void* owner);
    ~ApeGuiTexture() override;
    uint16_t Load() override;
    void Unload() override;

    // NOTE: the two flags are XML booleans whose names are only known by
    // hash (0x051B7FEC and 0x2E8401A3); they select sampler states.
    bool flag_51B7FEC = false;       // +0x5C bit 31
    bool flag_2E8401A3 = false;      // +0x5C bit 30
    void* texture = nullptr;         // +0x60 render texture
    void* samplers[3] = {};          // +0x64 per-filter texture views
};

// Creates a texture resource from a <texture name filename preload ...> element.
ApeGuiTexture* CreateGuiTexture(xml::XmlDocument* doc, xml::XmlElement* element, void* owner);

// Value substituted for "$<language variable>" in resource paths.
void SetGuiLanguagePath(const char* path);

// ---------------------------------------------------------------------------
// Objects
// ---------------------------------------------------------------------------

enum GuiObjectType : int32_t {
    kGuiObject_Text = 3,
};

// NOTE: only the tree/name part of the GUI object class is reconstructed;
// the PS3 class derives from a large engine base (vtable 0x00B22710).
class ApeGuiObject {
public:
    ApeGuiObject() = default;
    virtual ~ApeGuiObject() = default;
    ApeGuiObject(const ApeGuiObject&) = delete;
    ApeGuiObject& operator=(const ApeGuiObject&) = delete;

    virtual int32_t GetType() const { return 0; }
    virtual void SetText(const char* text, bool translate) {}

    char name[0x20] = {};            // +0x60
    uint32_t nameHash = 0;           // +0x80
    ApeGuiObject* parent = nullptr;  // +0x94
    ApeGuiObject* firstChild = nullptr; // +0x98
    ApeGuiObject* nextSibling = nullptr; // +0x9C
    void* owner = nullptr;           // +0xA0 GUI file that created the object
    const char* textId = nullptr;    // +0xC8 text objects: string table id
};

// Head of the root object list (0x00F66868).
ApeGuiObject*& GuiRootObjects();

// Finds a direct child of 'parent' (a root object when parent is null).
ApeGuiObject* FindGuiChild(uint32_t nameHash, ApeGuiObject* parent);
// Resolves a dotted path relative to 'scope'. A leading '.' starts at the
// root, each extra '.' of a leading ".." run goes up one level.
ApeGuiObject* FindGuiObject(const char* path, ApeGuiObject* scope);
// Appends ".<name>" for every object from the root down to 'object' at
// *cursor and advances it.
void BuildGuiObjectPath(const ApeGuiObject* object, char** cursor);

// Sets the string table used by text objects and re-applies every text.
void SetGuiStringTable(StringTable* table);
StringTable* GetGuiStringTable();

// ---------------------------------------------------------------------------
// Game hooks
// ---------------------------------------------------------------------------

// NOTE: two callbacks the game installs to filter GUI input per pad; the
// second one has no pad argument. Their exact contract is not known.
using GuiPadFilter = bool (*)(uint32_t pad, void* userData);
void SetGuiPadFilter(GuiPadFilter filter, void* userData);
using GuiFilter = bool (*)(void* userData);
void SetGuiFilter(GuiFilter filter, void* userData);

// Four opaque values the game stores for the GUI system (cleared when the
// GUI shuts down). NOTE: their meaning is unknown.
constexpr int kGuiSlotCount = 4;
void SetGuiSlot(int index, void* value);
void* GetGuiSlot(int index);

// Reads one pixel of an uncompressed bitmap as RGBA (alpha 255). Supports
// 24, 8, 4 and 1 bits per pixel; 'palette' holds BGRx entries. Rows are
// stored bottom-up unless 'topDown' is set.
void ReadBitmapPixel(uint8_t outRgba[4], const uint8_t* pixels, const uint8_t* palette, uint32_t x,
                     int32_t y, int32_t stride, int32_t height, int32_t bitsPerPixel, bool topDown);

} // namespace ape
