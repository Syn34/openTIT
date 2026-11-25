// openTIT - The GUI object classes (one per GUI file element).
//
// Element names come from the GUI file reader (0x004DE0F8): "frame", "image",
// "gradient", "label", "ticker", "tile", "group" and "clipregion"; type 8 is
// only known by its element hash (0x30CF0025). Only the simple
// members are reconstructed so far; the drawing, layout and cloning code of
// each class is listed as TODO next to the class.
#pragma once

#include "ape/level/ApeGui.h"

#include <string>

namespace agl {
class AglMaterial;
}

namespace ape {

// Applies the blend/depth state of a GUI blend mode to a material (inlined
// into the SetBlendMode overrides on the console).
void ApplyGuiBlendMode(agl::AglMaterial* material, uint32_t mode);

// Playback modes of GUI animations.
enum GuiAnimPlayMode : uint32_t {
    kGuiAnim_FwdLoop = 0,
    kGuiAnim_FwdHold = 1,
    kGuiAnim_PingPong = 2,
    kGuiAnim_RevLoop = 3,
    kGuiAnim_RevHold = 4,
};

// Mode named by 'nameHash' ("fwdloop", "fwdhold", "pingpong", "revloop",
// "revhold"); unknown names play forward looping.
GuiAnimPlayMode ParseGuiAnimPlayMode(uint32_t nameHash);

// Construction descriptions of the classes: the common part plus the
// element's own attributes (stack structs on the console).
struct GuiFrameDesc : GuiObjectDesc {
    float size[2] = {};                  // +0x80 "size"
};

struct GuiImageDesc : GuiObjectDesc {
    ApeGuiTexture* texture = nullptr;    // +0x80 "texture"
    // Used instead of the texture when set (the pieces of a tile share
    // their material).
    agl::AglMaterial* material = nullptr; // +0x84
    float uv[2] = {};                    // +0x88 "uv"
    float st[2] = { 1.0f, 1.0f };        // +0x90 "st"
    float size[2] = { -1.0f, -1.0f };    // +0x98 "size"
};

struct GuiGradientDesc : GuiObjectDesc {
    float size[2] = {};                  // +0x80 "size"
    xml::Color8 colors[4] = {};          // +0x88 "colortl", "colortr", "colorbl", "colorbr"
};

// Direction children of a group are stacked in.
enum GuiGroupFlow : uint8_t {
    kGuiGroup_Column = 0,   // top to bottom
    kGuiGroup_Row = 1,      // left to right
};
// How a group spaces its children.
enum GuiGroupArrange : uint8_t {
    kGuiGroup_Center = 0,   // child centres are 'spacing' apart
    kGuiGroup_Padding = 1,  // 'spacing' is the gap between children
};

struct GuiGroupDesc : GuiObjectDesc {
    float size[2] = {};                  // +0x80 "size"
    float offset[2] = {};                // +0x88 "offset"
    uint32_t flow = kGuiGroup_Column;    // +0x90 attribute 0xFBA47770 ("column"/"row")
    uint32_t arrange = kGuiGroup_Center; // +0x94 "arrange" ("center"/"padding")
    float spacing = 3.4028235e38f;       // +0x98 "spacing"; FLT_MAX: spread over the size
};

struct GuiClipRegionDesc : GuiObjectDesc {
    float size[2] = {};                  // +0x80 "size"
    // NOTE: attribute 0x76A60DC1; the rectangle used when the region is
    // projected by a perspective render target.
    float projectedSize[2] = {};         // +0x88
};

// Size the "expand" flag gives an object: its parent's layout size, or the
// screen for root objects.
void GetGuiExpandSize(const ApeGuiObject* object, float out[2]);

// "frame" (type 0, vtable 0x00B22920, 0xB0 bytes): an empty sized box.
class ApeGuiFrame : public ApeGuiObject {
public:
    explicit ApeGuiFrame(const GuiFrameDesc& desc);
    ~ApeGuiFrame() override;
    ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;
    void GetLayoutSize(float out[2]) const override;
    ApeGuiObject* HitTest(const float point[2]) override;

    float size[2] = {};   // +0xA8
};

// "image" (type 1, vtable 0x00B229E8, 0xD0 bytes).
class ApeGuiImage : public ApeGuiObject {
public:
    explicit ApeGuiImage(const GuiImageDesc& desc);
    ~ApeGuiImage() override;
    // Shows 'texture' (null: default texture 1) with the texture
    // coordinates 'uv' (offset) and 'st' (size); null coordinates use the
    // defaults. The material is shared with the texture resource when it has
    // one for the current blend mode.
    void SetTexture(ApeGuiTexture* texture, const float uv[2], const float st[2]);
    ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;
    void SetBlendMode(uint32_t mode) override;
    bool GetExpand() const override;
    void SetExpand(bool value) override;
    void GetLayoutSize(float out[2]) const override;
    void Draw() override;
    const GuiProperty* FindProperty(const char* name) const override;
    const GuiProperty* FindProperty(const char* name, const void* type) const override;

    agl::AglMaterial* material = nullptr;   // +0xA8
    ApeGuiTexture* texture = nullptr;       // +0xAC
    float uv[2] = {};     // +0xB0 "uv"
    float st[2] = { 1.0f, 1.0f };   // +0xB8 "st"
    float size[2] = { -1.0f, -1.0f };   // +0xC0 (-1: the texture's size)
};

// "gradient" (type 2, vtable 0x00B22AB0, 0xD0 bytes): a box with one
// colour per corner.
class ApeGuiGradient : public ApeGuiObject {
public:
    enum Corner { kTopLeft, kTopRight, kBottomLeft, kBottomRight };

    explicit ApeGuiGradient(const GuiGradientDesc& desc);
    ~ApeGuiGradient() override;
    ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;
    xml::Color8 GetColor() const override;
    void SetColor(xml::Color8 color) override;
    void SetBlendMode(uint32_t mode) override;
    void GetLayoutSize(float out[2]) const override;
    void Draw() override;

    float size[2] = {};   // +0xA8
    xml::Color8 colors[4] = {};   // +0xB0 Corner order
    agl::AglMaterial* material = nullptr;   // +0xC0
};

// "label" (type 3, vtable 0x00B227D8): text from the string table.
class ApeGuiLabel : public ApeGuiObject {
public:
    explicit ApeGuiLabel(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    ~ApeGuiLabel() override;
    int32_t GetType() const override;
    // The text shown ("" when none was set).
    const char* GetText() const;
    const GuiProperty* FindProperty(const char* name) const override;
    const GuiProperty* FindProperty(const char* name, const void* type) const override;

    float tracking = 0.0f;  // +0xB0 "tracking" (letter spacing)

    // Owned text buffers (capacity/data pairs on the console).
    std::string text;       // +0xB8/+0xC0 displayed text
    std::string idBuffer;   // +0xC4/+0xC8 string table id
    std::string field_CC;   // +0xCC/+0xD0
};

// "ticker" (type 5, vtable 0x00B22B78): a scrolling label.
class ApeGuiTicker : public ApeGuiLabel {
public:
    explicit ApeGuiTicker(const GuiObjectDesc& desc) : ApeGuiLabel(desc) {}
    ~ApeGuiTicker() override;
    int32_t GetType() const override;
};

// "tile" (type 6, vtable 0x00B22C40).
class ApeGuiTile : public ApeGuiObject {
public:
    explicit ApeGuiTile(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    ~ApeGuiTile() override;
    int32_t GetType() const override;
    void SetScale(const Vec3& scale) override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;
    void SetColor(xml::Color8 color) override;
    void SetBlendMode(uint32_t mode) override;
    void Invalidate() override;

    // The nine pieces of the tiled box (corners, edges, centre).
    ApeGuiObject* pieces[9] = {};   // +0xCC
    float size[2] = {};     // +0xF0
    bool layoutDirty = false; // +0xF8 set by Invalidate
};

// "group" (type 7, vtable 0x00B22D08, 0xC0 bytes): stacks its visible
// children in a row or a column (laid out in Tick when anything changed).
class ApeGuiGroup : public ApeGuiObject {
public:
    explicit ApeGuiGroup(const GuiGroupDesc& desc);
    ~ApeGuiGroup() override;
    ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;
    void GetLayoutSize(float out[2]) const override;
    void Tick(float dt) override;

    float size[2] = {};     // +0xA8
    float offset[2] = {};   // +0xB0 added to every child position
    uint8_t flow = kGuiGroup_Column;      // +0xB8 GuiGroupFlow
    uint8_t arrange = kGuiGroup_Center;   // +0xB9 GuiGroupArrange
    float spacing = 3.4028235e38f;        // +0xBC
};

// Type 8 (element hash 0x30CF0025, vtable 0x00B22DD0): wraps another object
// (+0xA8) whose visibility follows the GUI object.
class ApeGuiType8 : public ApeGuiObject {
public:
    // The wrapped object's interface as far as it is used here.
    class Target {
    public:
        virtual ~Target() = default;
        // Console slot 2: drops the reference held by the GUI object.
        virtual void Release() = 0;
        // NOTE: console slot 10 called with (visible, true).
        virtual void SetVisible(bool visible, bool flag) = 0;
    };

    explicit ApeGuiType8(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    ~ApeGuiType8() override;
    int32_t GetType() const override;
    void SetVisible(bool visible) override;
    void Draw() override;

    Target* target = nullptr;   // +0xA8
};

// "clipregion" (type 9, vtable 0x00B22E98, 0xC0 bytes): clips its
// children to its rectangle with the scissor test.
class ApeGuiClipRegion : public ApeGuiObject {
public:
    explicit ApeGuiClipRegion(const GuiClipRegionDesc& desc);
    ~ApeGuiClipRegion() override;
    ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;
    void GetLayoutSize(float out[2]) const override;
    void Draw() override;

    float size[2] = {};           // +0xA8
    float projectedSize[2] = {};  // +0xB0
};

} // namespace ape
