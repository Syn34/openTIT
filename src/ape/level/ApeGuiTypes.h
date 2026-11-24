// openTIT - The GUI object classes (one per GUI file element).
//
// Element names come from the GUI file reader (0x004DE0F8): "frame", "image",
// "gradient", "label", "ticker", "tile" and "group"; types 8 and 9 are only
// known by their element hashes (0x30CF0025, 0xAA8CFA44). Only the simple
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

// "frame" (type 0, vtable 0x00B22920, 0xB0 bytes): an empty sized box.
// TODO(0x004E4BF8 ApeGuiFrame::Clone) TODO(0x004E4E78 ApeGuiFrame::GetLayoutSize)
// TODO(0x004E4FB0 ApeGuiFrame::VirtualSlot44)
class ApeGuiFrame : public ApeGuiObject {
public:
    explicit ApeGuiFrame(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    ~ApeGuiFrame() override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;

    float size[2] = {};   // +0xA8
};

// "image" (type 1, vtable 0x00B229E8).
// TODO(0x004E51D4 ApeGuiImage::~ApeGuiImage) releases its texture.
class ApeGuiImage : public ApeGuiObject {
public:
    explicit ApeGuiImage(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    int32_t GetType() const override;
    void SetSize(const float size[2]) override;
    bool GetFlag29() const override;
    void SetFlag29(bool value) override;

    float size[2] = {};   // +0xC0
};

// "gradient" (type 2, vtable 0x00B22AB0).
class ApeGuiGradient : public ApeGuiObject {
public:
    explicit ApeGuiGradient(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;
    xml::Color8 GetColor() const override;
    void SetColor(xml::Color8 color) override;
    void SetBlendMode(uint32_t mode) override;

    float size[2] = {};   // +0xA8
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

// "group" (type 7, vtable 0x00B22D08, 0xB0 bytes).
class ApeGuiGroup : public ApeGuiObject {
public:
    explicit ApeGuiGroup(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    ~ApeGuiGroup() override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;

    float size[2] = {};   // +0xA8
};

// Type 8 (element hash 0x30CF0025, vtable 0x00B22DD0): wraps another object
// (+0xA8) whose visibility follows the GUI object.
class ApeGuiType8 : public ApeGuiObject {
public:
    // The wrapped object's interface as far as it is used here.
    class Target {
    public:
        virtual ~Target() = default;
        // NOTE: console slot 10 called with (visible, true).
        virtual void SetVisible(bool visible, bool flag) = 0;
    };

    explicit ApeGuiType8(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    int32_t GetType() const override;
    void SetVisible(bool visible) override;
    void Update() override;

    Target* target = nullptr;   // +0xA8
};

// Type 9 (element hash 0xAA8CFA44, vtable 0x00B22E98).
class ApeGuiType9 : public ApeGuiObject {
public:
    explicit ApeGuiType9(const GuiObjectDesc& desc) : ApeGuiObject(desc) {}
    ~ApeGuiType9() override;
    int32_t GetType() const override;
    void GetSize(float out[2]) const override;
    void SetSize(const float size[2]) override;

    float size[2] = {};   // +0xA8
};

} // namespace ape
