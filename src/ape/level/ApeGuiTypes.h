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
#include <vector>

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

// Decodes the UTF-8 character at 'cursor'; 'length' (optional) receives
// its size in bytes and the cursor moves past it when 'advance' is set.
uint32_t DecodeGuiChar(const char*& cursor, uint32_t* length, bool advance);

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

struct GuiLabelDesc : GuiObjectDesc {
    ApeGuiFontSet* font = nullptr;       // +0x80 "font"
    const char* text = "";               // +0x84 "string" (string table id or text)
    float leading = 1.0f;                // +0x88 "leading"
    float tracking = 0.0f;               // +0x8C "tracking"
    uint32_t wrapWidth = 0;              // +0x90 "wrap" (0: no wrapping)
    uint32_t field_94 = 0;               // +0x94 NOTE: 4 bits kept by the label; not read from XML
    bool fixedWidth = false;             // +0x98 "fixedwidth" (every character one space wide)
    uint32_t animMode = kGuiAnim_FwdLoop; // +0x9C "animmode" of animated glyphs
    float animRate = 3.0f;               // +0xA0 "animrate" (frames per second)
    bool layoutNow = false;              // +0xA4 lay the text out at once
};

// One laid out character of a label (0x20 bytes on the console, where the
// texture rectangle and the animation state are bit fields).
struct GuiLabelGlyph {
    float x = 0.0f, y = 0.0f;            // +0x00 position relative to the label origin
    float width = 0.0f, height = 0.0f;   // +0x08
    uint16_t texX = 0, texY = 0;         // texels of the first frame
    uint16_t texWidth = 0, texHeight = 0;
    bool untinted = false;               // the font keeps its colours (only alpha is applied)
    uint8_t frames = 0;                  // animation frames (cells to the right of the first)
    uint8_t frame = 0;                   // current frame
    bool reverse = false;                // ping-pong direction
    uint16_t line = 0;                   // text line of the character
    agl::AglMaterial* material = nullptr; // +0x18 the font material for the blend mode
    float animTime = 0.0f;               // +0x1C time since the last frame change

    // Advances the frame animation by 'dt' with one frame per 'period'.
    void Tick(float dt, float period, uint32_t mode);
};

// Reads the label attributes of an element into 'desc' (font, text,
// leading, ...); the defaults come from 'source' (a label) when cloning.
void ReadGuiLabelDesc(xml::XmlDocument* doc, xml::XmlElement* element, ApeGuiObject* source, GuiLabelDesc& desc);

// "label" (type 3, vtable 0x00B227D8, 0xF0 bytes): text drawn with a font
// set. The text may reference string table entries ("$Name"), is wrapped
// at "wrap" pixels and can be revealed character by character.
class ApeGuiLabel : public ApeGuiObject {
public:
    explicit ApeGuiLabel(const GuiLabelDesc& desc);
    ~ApeGuiLabel() override;
    ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const override;
    int32_t GetType() const override;
    void GetLayoutSize(float out[2]) const override;
    void Invalidate() override;
    void Tick(float dt) override;
    void Draw() override;
    const GuiProperty* FindProperty(const char* name) const override;
    const GuiProperty* FindProperty(const char* name, const void* type) const override;
    // Sets the text (string table references are expanded and the text is
    // wrapped); with 'layoutNow' the glyphs are built at once.
    void SetText(const char* text, bool layoutNow) override;

    // The text shown ("" when none was set).
    const char* GetText() const;
    // Advances the glyph animations (and the reveal time) by 'dt'.
    void TickGlyphs(float dt);
    // Distance between text lines in screen units.
    float GetLineSpacing() const;

    // Copies 'src' into the output, expanding "$Name" references and
    // inserting line breaks for the wrap width. Used by SetText and by
    // game classes that set the text in steps.
    struct FormatState {
        char* out = nullptr;            // write position
        size_t capacity = 0;            // size of the output buffer
        uint32_t bytes = 0;             // bytes written
        uint32_t chars = 0;             // characters written
        uint32_t lineChars = 0;         // characters on the current line
        float lineWidth = 0.0f;         // width of the current line
        char* lastBreak = nullptr;      // where the line may be broken (a space, or before a character)
        float widthAtBreak = 0.0f;      // line width at lastBreak
        uint32_t lastChar = 0;
    };
    void FormatText(const char* src, FormatState& state);
    // Makes room for a text of 'bytes' bytes (0: no text).
    void ReserveText(uint32_t bytes);
    // Makes room for 'count' glyphs (0: none); the slots are cleared when
    // they are reallocated.
    void ReserveGlyphs(uint32_t count);

protected:
    // Builds the glyph quads of the text ('force': even when hidden).
    void Layout(bool force);
    // Width of a space in screen units.
    float GetSpaceWidth(const Vec3& worldScale) const;
    // Horizontal start of each line for the alignment; returns the line count.
    uint32_t ComputeLineOffsets(float spaceWidth, float* offsets, uint32_t maxLines, const Vec3& worldScale) const;
    // Lays out one character at (x, y) and advances x; false when no font
    // has it.
    bool BuildGlyph(float spaceWidth, GuiLabelGlyph& glyph, uint32_t codePoint, float& x, float y,
                    const Vec3& worldScale, uint32_t line) const;
    // Counts the characters that have a glyph.
    void CountGlyphs();
    // Number of glyph slots that are drawn (the reveal limit).
    uint32_t GetVisibleGlyphLimit() const;
    // Labels whose layout size changes the layout (tickers) keep this set.
    void SetLayoutDirty() const { const_cast<ApeGuiLabel*>(this)->layoutDirty = true; }

public:
    ApeGuiFontSet* font = nullptr;  // +0xA8 (referenced, load referenced)
    float leading = 1.0f;           // +0xAC extra line distance in lines
    float tracking = 0.0f;          // +0xB0 "tracking" (letter spacing in spaces)
    // Bit fields of +0xB4.
    bool fixedWidth = false;        // bit 31
    bool layoutDirty = true;        // bit 30
    uint8_t field_94 = 0;           // bits 26-29
    uint8_t animMode = 0;           // bits 22-25 GuiAnimPlayMode of the glyphs
    uint16_t wrapWidth = 0;         // bits 6-21

    std::string text;               // +0xB8/+0xC0 the formatted text
    std::string textId;             // +0xC4/+0xC8 the text as set (string table id)
    std::vector<GuiLabelGlyph> glyphs; // +0xBC/+0xCC/+0xD0 capacity rounded up to 32
    uint32_t glyphCount = 0;        // +0xD4 characters with a glyph
    // +0xD8 after layout: glyphs built and lines; after drawing: glyphs
    // drawn and the last line drawn + 1.
    float progress[2] = {};
    float revealInterval = 0.0f;    // +0xE0 seconds per character (0: all shown)
    float revealTime = 0.0f;        // +0xE4
    float animPeriod = 0.0f;        // +0xE8 seconds per glyph animation frame
};

struct GuiTickerDesc : GuiLabelDesc {
    float scrollSpeed = 1.0f;            // +0xA8 "scrollspeed" (pixels per second)
    float width = 0.0f;                  // +0xAC attribute 0xB75E2EC9: visible width (0: the parent's)
    float gap = 0.0f;                    // +0xB0 attribute 0x77B9554D: space before the text repeats
    float startOffset = 0.0f;            // +0xB4 attribute 0x911A2AA2: scroll position after a reset
    bool scroll = false;                 // +0xB8 attribute 0x280D85C8: scrolling enabled
};

// "ticker" (type 5, vtable 0x00B22B78, 0x110 bytes): a single line label
// whose text scrolls to the left and wraps around. The glyphs are rebuilt
// every tick while it scrolls; drawing is the label's.
class ApeGuiTicker : public ApeGuiLabel {
public:
    explicit ApeGuiTicker(const GuiTickerDesc& desc);
    ~ApeGuiTicker() override;
    ApeGuiObject* Clone(const char* name, ApeGuiObject* parent, void* owner, bool recursive) const override;
    int32_t GetType() const override;
    void GetLayoutSize(float out[2]) const override;
    void Invalidate() override;
    void Tick(float dt) override;

    float scrollSpeed = 0.0f;   // +0xEC
    // mutable: GetLayoutSize stores the layout width here.
    mutable float width = 0.0f; // +0xF0
    float gap = 0.0f;           // +0xF4
    float scrollOffset = 0.0f;  // +0xF8
    float startOffset = 0.0f;   // +0xFC
    bool scrolling = false;     // +0x100 bit 7
    bool scrollEnabled = false; // +0x100 bit 6 (the configured value)
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
