// openTIT - Ape engine GUI core: resources, the object tree and globals.
//
// GUI screens are described by GUI files (compiled XML). Loading one creates
// resources (textures, bitmap fonts, ...) and a tree of GUI objects. Both are
// kept in process-wide intrusive lists:
//   * resources form a singly linked list, newest first;
//   * root objects form a sibling list, each object owns a child list.
// Objects are addressed by dotted paths of their names ("Menu.Title").
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

#include <cstdint>

namespace agl {
class AglDrawList;
class AglMaterial;
class AglRenderTarget;
class AglTexture;
}

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
    // Drops a load reference (unloads the data on the last one).
    void ReleaseLoadRef();
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

    // Builds 'materials' (one per GUI blend mode) for the loaded texture.
    void CreateMaterials();

    bool ztest = false;              // +0x5C bit 31: depth compare LEqual instead of Always
    // NOTE: XML boolean known only by hash (0x2E8401A3); enables the depth test.
    bool flag_2E8401A3 = false;      // +0x5C bit 30
    agl::AglTexture* texture = nullptr;      // +0x60
    agl::AglMaterial* materials[3] = {};     // +0x64 alpha, additive, subtractive
};

// Creates a texture resource from a <texture name filename preload ...> element.
ApeGuiTexture* CreateGuiTexture(xml::XmlDocument* doc, xml::XmlElement* element, void* owner);

// One glyph of a bitmap font, as found by scanning the font bitmap.
struct GuiGlyph {
    uint8_t top = 0;                 // +0 rows above the baseline
    uint8_t bottom = 0;              // +1 rows below the baseline
    uint8_t width = 0;               // +2
    uint8_t cells = 0;               // +3 character cells covered (wide glyphs span several)
    uint16_t x = 0;                  // +4 position in the texture
    uint16_t y = 0;                  // +6
};

// A font whose glyphs are cut out of a texture. The glyph rectangles come
// from a separate map bitmap ('path') in which marker pixels outline each
// glyph.
class ApeGuiBitmapFont : public ApeGuiResource {
public:
    ApeGuiBitmapFont(const char* name, const char* mapPath, void* owner, ApeGuiTexture* texture,
                     uint32_t firstChar);
    ~ApeGuiBitmapFont() override;
    uint16_t Load() override;
    void Unload() override;

    uint16_t GetHeight() const { return height; }

    ApeGuiTexture* texture = nullptr;        // +0x5C (referenced)
    uint32_t firstChar = ' ';                // +0x60 character of glyph 0 without a char map
    uint8_t glyphCount = 0;                  // +0x64
    bool ztest = false;                      // +0x65 bit 6, see ApeGuiTexture
    bool flag_2E8401A3 = false;              // +0x65 bit 5
    // NOTE: boolean known only by hash (0x04704F4C).
    bool flag_4704F4C = false;               // +0x65 bit 7
    uint16_t height = 0;                     // +0x66 line height in pixels
    GuiGlyph* glyphs = nullptr;              // +0x68
    uint8_t* charMap = nullptr;              // +0x6C 64K entries: glyph index per code point, 0xFF none
    agl::AglMaterial* materials[3] = {};     // +0x70
};

// Creates a bitmap font from a <font name texture mapfilename firstchar ...>
// element; the texture must already exist. An optional <map> child lists
// the code points of the glyphs in order (hex, optionally "\u" prefixed).
ApeGuiBitmapFont* CreateGuiBitmapFont(xml::XmlDocument* doc, xml::XmlElement* element, void* owner);

// Up to 16 bitmap fonts used together (one per script / glyph range), each
// with its own scale and extra letter spacing.
class ApeGuiFontSet : public ApeGuiResource {
public:
    static constexpr uint32_t kMaxFonts = 16;

    ApeGuiFontSet(const char* name, void* owner, const float scale[2]);
    ~ApeGuiFontSet() override;
    uint16_t Load() override;
    void Unload() override;

    // Adds 'font' (referenced). Scales after the first are relative to the
    // first font's height once the fonts are loaded.
    void AddFont(ApeGuiBitmapFont* font, float fontScale, float tracking);
    // The first loaded font that has a glyph for 'codePoint' (inlined at
    // every use on the console).
    bool FindGlyph(uint32_t codePoint, uint32_t& fontIndex, uint32_t& glyphIndex) const;
    // Height of a text line: the first font's height times the vertical
    // scale (0 without fonts).
    float GetLineHeight() const;

    float scale[2] = {};                     // +0x5C
    uint32_t fontCount = 0;                  // +0x64
    ApeGuiBitmapFont* fonts[kMaxFonts] = {}; // +0x68
    float fontScales[kMaxFonts] = {};        // +0xA8
    float fontTracking[kMaxFonts] = {};      // +0xE8 added to the label's tracking
};

// Creates a font set from a <fontset name scale preload> element with
// <page name scale tracking> children naming existing fonts.
ApeGuiFontSet* CreateGuiFontSet(xml::XmlDocument* doc, xml::XmlElement* element, void* owner);

// Value substituted for "$<language variable>" in resource paths.
void SetGuiLanguagePath(const char* path);

// ---------------------------------------------------------------------------
// Objects
// ---------------------------------------------------------------------------

enum GuiObjectType : int32_t {
    kGuiObject_Type0 = 0,   // NOTE: class names unknown except text; see ApeGuiObject.cpp
    kGuiObject_Type1 = 1,
    kGuiObject_Type2 = 2,
    kGuiObject_Text  = 3,
    kGuiObject_Type5 = 5,
    kGuiObject_Type6 = 6,
    kGuiObject_Type7 = 7,
    kGuiObject_Type8 = 8,
    kGuiObject_ClipRegion = 9,
};

// Horizontal (bits 0-1) and vertical (bits 4-5) parts of the "anchor" and
// "align" attributes: L/C/R and T/M/B.
enum GuiAlign : uint32_t {
    kGuiAlign_Left = 0x00, kGuiAlign_Center = 0x01, kGuiAlign_Right = 0x02,
    kGuiAlign_Top = 0x00, kGuiAlign_Middle = 0x10, kGuiAlign_Bottom = 0x20,
};

// "blend" attribute: "av", "ad", "su".
enum GuiBlendMode : uint32_t {
    kGuiBlend_Average = 0, kGuiBlend_Add = 1, kGuiBlend_Subtract = 2,
};

class ApeGuiObject;
class GuiProperty;

// Construction description filled by the GUI file reader (a stack struct on
// the console).
struct GuiObjectDesc {
    const char* name = nullptr;          // +0x00
    Matrix34 transform{};                // +0x10 axes and position
    Vec3 scale{};                        // +0x50
    xml::Color8 color{};                 // +0x60
    uint32_t blendMode = kGuiBlend_Average; // +0x64
    ApeGuiObject* parent = nullptr;      // +0x68
    void* owner = nullptr;               // +0x6C GUI file
    uint32_t align = 0;                  // +0x70 GuiAlign bits
    uint32_t anchor = 0;                 // +0x74 GuiAlign bits
    bool visible = true;                 // +0x78 "visible"
    bool flag30 = false;                 // +0x79 attribute 0xFFD1EC91
    bool expand = false;                 // +0x7A attribute 0x7ABBD29A ("expand" property)
    bool inheritScale = true;            // +0x7B "inheritscale"
    bool inheritRgb = true;              // +0x7C "inheritrgb"
    bool inheritAlpha = true;            // +0x7D "inheritalpha"
    bool render3d = false;               // +0x7E "render3d"
};

// Base of every GUI object (vtable 0x00B22710, 0xA8 bytes). Objects form a
// tree: each has a parent, a first child and a next sibling; root objects
// are chained from GuiRootObjects().
class ApeGuiObject {
public:
    // Flags word (+0x88).
    enum Flags : uint32_t {
        kFlag_Render3d     = 1u << 5,
        kFlag_Detached     = 1u << 24,   // not linked into the tree (default construction)
        kFlag_Dirty        = 1u << 25,   // set by Invalidate()
        kFlag_InheritAlpha = 1u << 26,
        kFlag_InheritRgb   = 1u << 27,
        kFlag_InheritScale = 1u << 28,
        kFlag_Expand       = 1u << 29,   // fill the parent ("expand"; XML attribute 0x7ABBD29A)
        kFlag_30           = 1u << 30,   // attribute 0xFFD1EC91: can take the pad focus
        kFlag_Visible      = 1u << 31,
    };

    ApeGuiObject();
    explicit ApeGuiObject(const GuiObjectDesc& desc);
    virtual ~ApeGuiObject();
    ApeGuiObject(const ApeGuiObject&) = delete;
    ApeGuiObject& operator=(const ApeGuiObject&) = delete;

    // Creates a copy named 'name' under 'parent' (a root object when null),
    // owned by 'owner'; with 'recursive' the children are copied as well.
    // The base class cannot be copied and returns null.
    virtual ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const;
    virtual int32_t GetType() const = 0;

    virtual const Matrix34& GetTransform() const;
    virtual void SetTransform(const Matrix34& transform);
    virtual const Vec3& GetPosition() const;
    virtual void SetPosition(const Vec3& position);
    // Rotation about the Z axis in degrees (keeps the position).
    virtual void SetRotation(float degrees);
    virtual const Vec3& GetScale() const;
    virtual void SetScale(const Vec3& scale);
    // Size of the object's content; zero for the base class.
    virtual void GetSize(float out[2]) const;
    virtual void SetSize(const float size[2]);
    virtual xml::Color8 GetColor() const;
    virtual void SetColor(xml::Color8 color);
    virtual uint32_t GetBlendMode() const;
    virtual void SetBlendMode(uint32_t mode);
    virtual uint32_t GetAlign() const;
    virtual void SetAlign(uint32_t align);
    virtual uint32_t GetAnchor() const;
    virtual void SetAnchor(uint32_t anchor);
    virtual bool IsVisible() const;
    virtual void SetVisible(bool visible);
    virtual bool GetFlag30() const;
    virtual void SetFlag30(bool value);
    virtual bool GetExpand() const;
    virtual void SetExpand(bool value);
    virtual bool GetInheritScale() const;
    virtual void SetInheritScale(bool value);
    virtual bool GetInheritRgb() const;
    virtual void SetInheritRgb(bool value);
    virtual bool GetInheritAlpha() const;
    virtual void SetInheritAlpha(bool value);
    virtual bool GetRender3d() const;
    virtual void SetRender3d(bool value);
    // Size used to place the object in its parent (alignment/anchoring), in
    // screen units (the world scale applied).
    virtual void GetLayoutSize(float out[2]) const;
    // Centre of the object's rectangle in screen space.
    virtual Vec3 GetCenter() const;
    // Marks the object and its subtree dirty.
    virtual void Invalidate();
    // Advances animations by 'dt' seconds (base: the visible subtree).
    virtual void Tick(float dt);
    // Records the object's geometry into the GUI draw list (base: the
    // visible subtree).
    virtual void Draw();
    // The object's rectangle relative to its origin ('min', 'max') and the
    // matrix placing that origin on the screen.
    virtual void GetScreenRect(float min[2], float max[2], Matrix34& world) const;
    // The matrix placing the object's origin: the parent's matrix, the
    // anchor point in the parent and the object's own transform.
    virtual Matrix34 GetWorldMatrix() const;
    // Scale including the inherited parent scale (when kFlag_InheritScale).
    virtual Vec3 GetWorldScale() const;
    // Colour modulated by the parent's world colour (inherit rgb / alpha).
    virtual xml::Color8 GetWorldColor() const;
    // Topmost visible object of the subtree containing the screen point.
    virtual ApeGuiObject* HitTest(const float point[2]);
    // Animatable property by name (optionally of a given value type).
    virtual const GuiProperty* FindProperty(const char* name) const;
    virtual const GuiProperty* FindProperty(const char* name, const void* type) const;

    // Text objects only (console slot 47 of the label classes).
    virtual void SetText(const char* text, bool layoutNow) {}

    // Moves the object to the end of its sibling list so it draws last (on
    // top); with 'recursive' its ancestors are moved as well.
    void BringToFront(bool recursive);
    // Fills a construction description that copies this object.
    void FillCloneDesc(GuiObjectDesc& desc, const char* name, ApeGuiObject* parent, void* owner) const;
    // Copies every child of this object under 'parent'.
    void CloneChildren(ApeGuiObject* parent, void* owner) const;

    bool HasFlag(uint32_t flag) const { return (flags & flag) != 0; }
    void SetFlag(uint32_t flag, bool on) { flags = on ? (flags | flag) : (flags & ~flag); }

    Matrix34 transform{};            // +0x10 (translation row = position)
    Vec3 scale{};                    // +0x50
    char name[0x20] = {};            // +0x60
    uint32_t nameHash = 0;           // +0x80
    xml::Color8 color{};             // +0x84
    // mutable: GetLayoutSize caches its result and clears kFlag_Dirty.
    mutable uint32_t flags = 0;      // +0x88 Flags; bits 22-23 blend, 14-21 align, 6-13 anchor
    mutable float layoutSize[2] = {}; // +0x8C cached layout size
    ApeGuiObject* parent = nullptr;  // +0x94
    ApeGuiObject* firstChild = nullptr; // +0x98
    ApeGuiObject* nextSibling = nullptr; // +0x9C
    void* owner = nullptr;           // +0xA0 GUI file that created the object
    uint32_t field_A4 = 0;           // +0xA4
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

// Pad input the game reports to the GUI each frame (filled by the pad
// filter callback).
struct GuiPadInput {
    void* userData = nullptr;   // +0x00 the filter's user data
    uint32_t buttons = 0;       // +0x04 GuiPadButton bits held
    uint32_t edges = 0;         // +0x08 bits that changed this frame
    uint32_t flags = 0;         // +0x0C kGuiPadInput_Cursor: 'cursor' is valid
    float cursor[2] = {};       // +0x10 pointer position, -1..1 on both axes
};
enum GuiPadButton : uint32_t {
    kGuiPad_Up = 0x01, kGuiPad_Down = 0x02, kGuiPad_Left = 0x04, kGuiPad_Right = 0x08,
    kGuiPad_Button10 = 0x10, kGuiPad_Button20 = 0x20,   // NOTE: GUI events 7 and 8
    kGuiPad_Cancel = 0x40, kGuiPad_Accept = 0x80,
    kGuiPad_Start = 0x4000,
};
constexpr uint32_t kGuiPadInput_Cursor = 0x30000;
// Returns false when the pad has nothing for the GUI this frame.
using GuiPadFilter = bool (*)(uint32_t pad, GuiPadInput* input);
void SetGuiPadFilter(GuiPadFilter filter, void* userData);
// Directional focus navigation: enabled flag (0x00F66880) and the direction
// bits it reacts to (0x00F66888).
void SetGuiNavigation(bool enabled, uint32_t buttonMask);
// Per frame GUI update: pad focus and button events, script players and
// the object animations.
void UpdateGui(float dt);
// GUI events are reported to one handler the game installs. The event
// carries a type, a pad index, the handler's user data, the object
// concerned and one argument. Types sent by the GUI: 1 focus gained, 2
// focus lost, 3 may-take-focus query, 4 focus moved by direction, 5 accept,
// 6 cancel, 7 and 8 (two more buttons), 9 start, 10/11 right/left; script
// message steps send any of these.
struct GuiEvent {
    uint32_t type;      // +0x00
    uint32_t pad;       // +0x04
    void* userData;     // +0x08
    ApeGuiObject* object; // +0x0C
    uintptr_t param;    // +0x10 (a script player for script messages)
};
using GuiFilter = bool (*)(const GuiEvent* event);
void SetGuiFilter(GuiFilter filter, void* userData);
// GUI event 3 asks whether an object may take the focus of a pad (the
// argument is the best candidate so far).
constexpr uint32_t kGuiEvent_CanFocus = 3;
// Focus search among the visible focusable objects of a subtree (the last
// match in drawing order wins): the object under 'point' closest to its
// origin, or the object best placed in direction 'dir' from 'current'
// (angle and distance; the screen wraps around). 'best' holds the best
// score so far.
ApeGuiObject* FindGuiFocusAt(ApeGuiObject* object, uint32_t pad, const float point[2], float* best);
ApeGuiObject* FindGuiFocusInDirection(ApeGuiObject* object, uint32_t pad, ApeGuiObject* current, const float dir[2],
                                      float* best);
// Sends an event to the handler; true when none is installed.
bool SendGuiEvent(uint32_t type, uint32_t pad, ApeGuiObject* object, uintptr_t param);

// Four opaque values the game stores for the GUI system (cleared when the
// GUI shuts down). NOTE: their meaning is unknown.
constexpr int kGuiSlotCount = 4;
void SetGuiSlot(int index, void* value);
void* GetGuiSlot(int index);

// ---------------------------------------------------------------------------
// Rendering state
// ---------------------------------------------------------------------------

// Creates the GUI render target ("RenderTarget_GUI", orthographic). The
// viewport is the whole back buffer unless 'viewport' (x, y, w, h) is given.
bool InitGuiRendering(const int16_t* viewport);
// Destroys every GUI animation, object and resource.
void DestroyGuiObjects();
// Shuts the GUI down (objects and the render target).
void ShutdownGui();
// Records the GUI into the draw list ('draw' false: nothing): the screen
// fade quad, then every root object.
void DrawGui(bool draw);
// Sets the colour of a quad covering the screen behind the GUI (alpha 0
// removes it).
void SetGuiFadeColor(xml::Color8 color);
// Size of the GUI screen in pixels (the back buffer).
void GetGuiScreenSize(float out[2]);
// The draw list GUI objects record into: the override when one is set,
// otherwise the game's list.
agl::AglDrawList& GetGuiDrawList();
void SetGuiDrawListOverride(agl::AglDrawList* list);
// The GUI's own render target (0x00F6684C).
agl::AglRenderTarget* GetGuiRenderTarget();
// Render target that "render3d" objects are projected with (0x00F66850).
agl::AglRenderTarget* GetGui3dRenderTarget();
void SetGui3dRenderTarget(agl::AglRenderTarget* target);
// Render target the GUI is being drawn into (0x00F6688C).
agl::AglRenderTarget* GetGuiDrawTarget();
void SetGuiDrawTarget(agl::AglRenderTarget* target);

// Reads one pixel of an uncompressed bitmap as RGBA (alpha 255). Supports
// 24, 8, 4 and 1 bits per pixel; 'palette' holds BGRx entries. Rows are
// stored bottom-up unless 'topDown' is set.
void ReadBitmapPixel(uint8_t outRgba[4], const uint8_t* pixels, const uint8_t* palette, uint32_t x,
                     int32_t y, int32_t stride, int32_t height, int32_t bitsPerPixel, bool topDown);

} // namespace ape
