// openTIT - GUI object base class and the simple members of the GUI classes.
#include "ape/level/ApeGuiTypes.h"

#include "ape/core/ApeString.h"
#include "ape/sys/ApeGeometry.h"
#include "agl/AglResources.h"
#include "agl/AglTexture.h"

namespace ape {

namespace {

// Engine defaults the constructors copy (0x01164BD0 identity, 0x01164B30
// unit scale, 0x01164CA0 default colour; see ApeInstances for the colour).
Matrix34 IdentityTransform()
{
    Matrix34 m{};
    m.xAxis.x = 1.0f;
    m.yAxis.y = 1.0f;
    m.zAxis.z = 1.0f;
    return m;
}
constexpr Vec3 kUnitScale = { 1.0f, 1.0f, 1.0f, 0.0f };
constexpr xml::Color8 kDefaultColor = { 0x80, 0x80, 0x80, 0x80 };
constexpr Vec3 kZAxis = { 0.0f, 0.0f, 1.0f, 0.0f };   // 0x01164B70

constexpr uint32_t kBlendShift = 22, kBlendMask = 0x3u;
constexpr uint32_t kAlignShift = 14, kAlignMask = 0xFFu;
constexpr uint32_t kAnchorShift = 6, kAnchorMask = 0xFFu;

uint32_t GetBits(uint32_t flags, uint32_t shift, uint32_t mask) { return (flags >> shift) & mask; }
uint32_t SetBits(uint32_t flags, uint32_t shift, uint32_t mask, uint32_t value)
{
    return (flags & ~(mask << shift)) | ((value & mask) << shift);
}

// Copies the axes and the position (the fourth lanes are left alone).
void CopyAffine(Matrix34& dst, const Matrix34& src)
{
    const Vec3* from[4] = { &src.xAxis, &src.yAxis, &src.zAxis, &src.translation };
    Vec3* to[4] = { &dst.xAxis, &dst.yAxis, &dst.zAxis, &dst.translation };
    for (int i = 0; i < 4; ++i) {
        to[i]->x = from[i]->x;
        to[i]->y = from[i]->y;
        to[i]->z = from[i]->z;
    }
}

// Clears the GUI slots that point at a dying object (0x010C2AD8).
void ForgetGuiSlots(ApeGuiObject* object)
{
    for (int i = 0; i < kGuiSlotCount; ++i) {
        if (GetGuiSlot(i) == object)
            SetGuiSlot(i, nullptr);
    }
}

uint8_t Modulate(uint8_t value, uint8_t by) { return (uint8_t)(((uint32_t)value * by) >> 8); }

} // namespace

// Material blend presets: 0 opaque, 1 opaque with blending on, 2 alpha
// blend, 3 additive, 4 reverse subtract. The GUI blend modes "av", "ad" and
// "su" select 2, 3 and 4 (tables 0x00AEF7F8 / 0x00AEF990).
namespace {
const int kGuiBlendPresets[4] = { 2, 3, 4, 0 };
}

namespace {
// Compile-time HashNameNoCase().
constexpr uint32_t GuiHash(const char* name)
{
    uint32_t hash = 0;
    for (; *name; ++name) {
        int c = (signed char)*name;
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        hash = hash * 31u + (uint32_t)c;
    }
    return hash;
}
static_assert(GuiHash("pingpong") == 0x152D006Au, "GuiHash must match HashNameNoCase");
} // namespace

// @func 0x004E2B60 ParseGuiAnimPlayMode
GuiAnimPlayMode ParseGuiAnimPlayMode(uint32_t nameHash)
{
    switch (nameHash) {
    case GuiHash("revloop"): return kGuiAnim_RevLoop;
    case GuiHash("revhold"): return kGuiAnim_RevHold;
    case GuiHash("pingpong"): return kGuiAnim_PingPong;
    case GuiHash("fwdloop"): return kGuiAnim_FwdLoop;
    case GuiHash("fwdhold"): return kGuiAnim_FwdHold;
    default: return kGuiAnim_FwdLoop;
    }
}

void ApplyGuiBlendMode(agl::AglMaterial* material, uint32_t mode)
{
    using agl::gpu::BlendEquation;
    using agl::gpu::BlendFactor;
    switch (kGuiBlendPresets[mode & 3]) {
    case 4:
        material->blendEquation = BlendEquation::ReverseSubtract;
        material->blendDst = BlendFactor::One;
        material->blendEnable = true;
        material->blendSrc = BlendFactor::SrcAlpha;
        break;
    case 3:
        material->blendEquation = BlendEquation::Add;
        material->blendDst = BlendFactor::One;
        material->blendEnable = true;
        material->blendSrc = BlendFactor::SrcAlpha;
        break;
    case 2:
        material->blendEquation = BlendEquation::Add;
        material->blendDst = BlendFactor::OneMinusSrcAlpha;
        material->blendEnable = true;
        material->blendSrc = BlendFactor::SrcAlpha;
        break;
    case 1:
        material->blendEquation = BlendEquation::Add;
        material->blendDst = BlendFactor::Zero;
        material->blendEnable = true;
        material->blendSrc = BlendFactor::One;
        break;
    case 0:
        material->blendEquation = BlendEquation::Add;
        material->blendDst = BlendFactor::Zero;
        material->blendEnable = false;
        material->blendSrc = BlendFactor::One;
        break;
    }
    // GUI geometry ignores the depth buffer.
    material->depthFunc = agl::gpu::CompareFunc::Always;
    material->depthTest = false;
}

// ---------------------------------------------------------------------------
// ApeGuiObject
// ---------------------------------------------------------------------------

// @func 0x004D8D48 ApeGuiObject::ApeGuiObject
// A default constructed object is not linked into the tree.
ApeGuiObject::ApeGuiObject()
    : transform(IdentityTransform()), scale(kUnitScale), color(kDefaultColor)
{
    flags = (flags & ((kAnchorMask << kAnchorShift) | (kAlignMask << kAlignShift) | (kBlendMask << kBlendShift) |
                      kFlag_Render3d | 0x1Fu)) |
            kFlag_Detached | kFlag_Dirty;
}

// @func 0x004D76D0 ApeGuiObject::ApeGuiObject(desc)
// Appends the object to its parent's children (or to the root list).
ApeGuiObject::ApeGuiObject(const GuiObjectDesc& desc)
    : scale(desc.scale), color(desc.color)
{
    CopyAffine(transform, desc.transform);
    nameHash = HashNameNoCase(desc.name);
    uint32_t f = flags & 0x1Fu;
    f |= desc.visible ? (uint32_t)kFlag_Visible : 0u;
    f |= desc.flag30 ? (uint32_t)kFlag_30 : 0u;
    f |= desc.expand ? (uint32_t)kFlag_Expand : 0u;
    f |= desc.inheritScale ? (uint32_t)kFlag_InheritScale : 0u;
    f |= desc.inheritRgb ? (uint32_t)kFlag_InheritRgb : 0u;
    f |= desc.inheritAlpha ? (uint32_t)kFlag_InheritAlpha : 0u;
    f |= kFlag_Dirty;
    f = SetBits(f, kBlendShift, kBlendMask, desc.blendMode);
    f = SetBits(f, kAlignShift, kAlignMask, desc.align);
    f = SetBits(f, kAnchorShift, kAnchorMask, desc.anchor);
    f |= desc.render3d ? (uint32_t)kFlag_Render3d : 0u;
    flags = f;
    parent = desc.parent;
    owner = desc.owner;
    StrNCopy(name, desc.name ? desc.name : "", sizeof(name) - 1);

    ApeGuiObject** link = parent ? &parent->firstChild : &GuiRootObjects();
    while (*link)
        link = &(*link)->nextSibling;
    *link = this;
}

// @func 0x004DFF1C ApeGuiObject::~ApeGuiObject
// @func 0x004E0094 ApeGuiObject::~ApeGuiObject (deleting)
ApeGuiObject::~ApeGuiObject()
{
    // Each child unlinks itself while it is destroyed.
    while (firstChild)
        delete firstChild;

    if (!HasFlag(kFlag_Detached)) {
        ApeGuiObject** link = parent ? &parent->firstChild : &GuiRootObjects();
        while (*link && *link != this)
            link = &(*link)->nextSibling;
        if (*link)
            *link = nextSibling;
        nextSibling = nullptr;
    }
    ForgetGuiSlots(this);
}

// @func 0x007882F4 ApeGuiObject::Clone
ApeGuiObject* ApeGuiObject::Clone(const char*, ApeGuiObject*, void*, bool) const
{
    return nullptr;
}

// @func 0x00761B14 ApeGuiObject::GetTransform
const Matrix34& ApeGuiObject::GetTransform() const
{
    return transform;
}

// @func 0x00761B20 ApeGuiObject::SetTransform
void ApeGuiObject::SetTransform(const Matrix34& value)
{
    CopyAffine(transform, value);
}

// @func 0x00761B84 ApeGuiObject::GetPosition
const Vec3& ApeGuiObject::GetPosition() const
{
    return transform.translation;
}

// @func 0x00761B90 ApeGuiObject::SetPosition
void ApeGuiObject::SetPosition(const Vec3& position)
{
    transform.translation.x = position.x;
    transform.translation.y = position.y;
    transform.translation.z = position.z;
}

// @func 0x004E0228 ApeGuiObject::SetRotation
void ApeGuiObject::SetRotation(float degrees)
{
    Matrix34 rotation{};
    MatrixAxisAngle(degrees * 0.017453292f, rotation, kZAxis);
    rotation.translation = transform.translation;
    CopyAffine(transform, rotation);
}

// @func 0x00761BB4 ApeGuiObject::GetScale
const Vec3& ApeGuiObject::GetScale() const
{
    return scale;
}

// @func 0x00761BC0 ApeGuiObject::SetScale
void ApeGuiObject::SetScale(const Vec3& value)
{
    scale.x = value.x;
    scale.y = value.y;
    scale.z = value.z;
}

// @func 0x00761BDC ApeGuiObject::GetSize
void ApeGuiObject::GetSize(float out[2]) const
{
    out[0] = 0.0f;
    out[1] = 0.0f;
}

// @func 0x00761BF8 ApeGuiObject::SetSize
void ApeGuiObject::SetSize(const float size[2])
{
}

// @func 0x00761BFC ApeGuiObject::GetColor
xml::Color8 ApeGuiObject::GetColor() const
{
    return color;
}

// @func 0x00761C28 ApeGuiObject::SetColor
void ApeGuiObject::SetColor(xml::Color8 value)
{
    color = value;
}

// @func 0x00761C50 ApeGuiObject::GetBlendMode
uint32_t ApeGuiObject::GetBlendMode() const
{
    return GetBits(flags, kBlendShift, kBlendMask);
}

// @func 0x00761C5C ApeGuiObject::SetBlendMode
void ApeGuiObject::SetBlendMode(uint32_t mode)
{
    flags = SetBits(flags, kBlendShift, kBlendMask, mode);
}

// @func 0x00761C6C ApeGuiObject::GetAlign
uint32_t ApeGuiObject::GetAlign() const
{
    return GetBits(flags, kAlignShift, kAlignMask);
}

// @func 0x00761C78 ApeGuiObject::SetAlign
void ApeGuiObject::SetAlign(uint32_t align)
{
    flags = SetBits(flags, kAlignShift, kAlignMask, align);
}

// @func 0x00761C88 ApeGuiObject::GetAnchor
uint32_t ApeGuiObject::GetAnchor() const
{
    return GetBits(flags, kAnchorShift, kAnchorMask);
}

// @func 0x00761C94 ApeGuiObject::SetAnchor
void ApeGuiObject::SetAnchor(uint32_t anchor)
{
    flags = SetBits(flags, kAnchorShift, kAnchorMask, anchor);
}

// @func 0x00761CA4 ApeGuiObject::IsVisible
bool ApeGuiObject::IsVisible() const
{
    return HasFlag(kFlag_Visible);
}

// @func 0x00761CBC ApeGuiObject::SetVisible
void ApeGuiObject::SetVisible(bool visible)
{
    SetFlag(kFlag_Visible, visible);
}

// @func 0x00761CD0 ApeGuiObject::GetFlag30
bool ApeGuiObject::GetFlag30() const
{
    return HasFlag(kFlag_30);
}

// @func 0x00761CE8 ApeGuiObject::SetFlag30
void ApeGuiObject::SetFlag30(bool value)
{
    SetFlag(kFlag_30, value);
}

// @func 0x00761CF8 ApeGuiObject::GetExpand
bool ApeGuiObject::GetExpand() const
{
    return HasFlag(kFlag_Expand);
}

// @func 0x00761D10 ApeGuiObject::SetExpand
void ApeGuiObject::SetExpand(bool value)
{
    SetFlag(kFlag_Expand, value);
}

// @func 0x00761D20 ApeGuiObject::GetInheritScale
bool ApeGuiObject::GetInheritScale() const
{
    return HasFlag(kFlag_InheritScale);
}

// @func 0x00761D38 ApeGuiObject::SetInheritScale
void ApeGuiObject::SetInheritScale(bool value)
{
    SetFlag(kFlag_InheritScale, value);
}

// @func 0x00761D48 ApeGuiObject::GetInheritRgb
bool ApeGuiObject::GetInheritRgb() const
{
    return HasFlag(kFlag_InheritRgb);
}

// @func 0x00761D60 ApeGuiObject::SetInheritRgb
void ApeGuiObject::SetInheritRgb(bool value)
{
    SetFlag(kFlag_InheritRgb, value);
}

// @func 0x00761D70 ApeGuiObject::GetInheritAlpha
bool ApeGuiObject::GetInheritAlpha() const
{
    return HasFlag(kFlag_InheritAlpha);
}

// @func 0x00761D88 ApeGuiObject::SetInheritAlpha
void ApeGuiObject::SetInheritAlpha(bool value)
{
    SetFlag(kFlag_InheritAlpha, value);
}

// @func 0x00761D98 ApeGuiObject::GetRender3d
bool ApeGuiObject::GetRender3d() const
{
    return HasFlag(kFlag_Render3d);
}

// @func 0x00761DB0 ApeGuiObject::SetRender3d
void ApeGuiObject::SetRender3d(bool value)
{
    SetFlag(kFlag_Render3d, value);
}

// @func 0x007882FC ApeGuiObject::GetLayoutSize
void ApeGuiObject::GetLayoutSize(float out[2]) const
{
    out[0] = layoutSize[0];
    out[1] = layoutSize[1];
}

// @func 0x004E0508 ApeGuiObject::Invalidate
void ApeGuiObject::Invalidate()
{
    flags |= kFlag_Dirty;
    for (ApeGuiObject* child = firstChild; child; child = child->nextSibling)
        child->Invalidate();
}

// @func 0x004D89CC ApeGuiObject::Tick
void ApeGuiObject::Tick(float dt)
{
    if (!IsVisible())
        return;
    for (ApeGuiObject* child = firstChild; child; child = child->nextSibling)
        child->Tick(dt);
}

// @func 0x004E05A4 ApeGuiObject::Draw
void ApeGuiObject::Draw()
{
    if (!IsVisible())
        return;
    for (ApeGuiObject* child = firstChild; child; child = child->nextSibling)
        child->Draw();
}

// @func 0x004D7E70 ApeGuiObject::GetWorldScale
Vec3 ApeGuiObject::GetWorldScale() const
{
    Vec3 result = { scale.x, scale.y, scale.z, 0.0f };
    if (HasFlag(kFlag_InheritScale) && parent) {
        const Vec3 parentScale = parent->GetWorldScale();
        result.x *= parentScale.x;
        result.y *= parentScale.y;
        result.z *= parentScale.z;
    }
    return result;
}

// @func 0x004E0FDC ApeGuiObject::GetWorldColor
xml::Color8 ApeGuiObject::GetWorldColor() const
{
    xml::Color8 result = color;
    if (parent) {
        const xml::Color8 inherited = parent->GetWorldColor();
        if (HasFlag(kFlag_InheritRgb)) {
            result.r = Modulate(result.r, inherited.r);
            result.g = Modulate(result.g, inherited.g);
            result.b = Modulate(result.b, inherited.b);
        }
        if (HasFlag(kFlag_InheritAlpha))
            result.a = Modulate(result.a, inherited.a);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

// @func 0x00794A7C ApeGuiFrame::~ApeGuiFrame
// @func 0x00796114 ApeGuiFrame::~ApeGuiFrame (deleting)
ApeGuiFrame::~ApeGuiFrame() = default;

// @func 0x007960AC ApeGuiFrame::GetType
int32_t ApeGuiFrame::GetType() const
{
    return kGuiObject_Type0;
}

// @func 0x007960B4 ApeGuiFrame::GetSize
void ApeGuiFrame::GetSize(float out[2]) const
{
    out[0] = size[0];
    out[1] = size[1];
}

// @func 0x007960C8 ApeGuiFrame::SetSize
void ApeGuiFrame::SetSize(const float value[2])
{
    size[0] = value[0];
    size[1] = value[1];
    Invalidate();
}

// ---------------------------------------------------------------------------
// Image
// ---------------------------------------------------------------------------

// @func 0x004E51D4 ApeGuiImage::~ApeGuiImage
// @func 0x004E53A8 ApeGuiImage::~ApeGuiImage (deleting)
ApeGuiImage::~ApeGuiImage()
{
    if (material)
        material->Release();
    if (texture)
        texture->Release();
}

// @func 0x004D8A80 ApeGuiImage::SetTexture
void ApeGuiImage::SetTexture(ApeGuiTexture* newTexture, const float newUv[2], const float newSt[2])
{
    if (material) {
        // Keep the material if it already draws the new texture's image.
        bool same = false;
        if (newTexture) {
            agl::AglTexture* image = newTexture->GetLoadState() == kGuiLoad_Loaded
                                         ? newTexture->texture
                                         : agl::GetDefaultTexture(0);
            same = material->textures[0] == image;
        }
        if (!same) {
            if (texture) {
                texture->Release();
                texture = nullptr;
            }
            material->Release();
            material = nullptr;
        }
    }

    texture = newTexture;
    if (newTexture) {
        uv[0] = newUv[0];
        uv[1] = newUv[1];
        st[0] = newSt[0];
        st[1] = newSt[1];
    } else {
        uv[0] = uv[1] = 0.0f;
        st[0] = st[1] = 1.0f;
    }

    if (!material) {
        const uint32_t blend = GetBlendMode();
        if (!newTexture) {
            material = agl::CreateSpriteMaterial(agl::GetDefaultTexture(1));
            SetBlendMode(blend);
        } else {
            newTexture->AddRef();
            newTexture->AddLoadRef();
            material = newTexture->materials[blend & 3];
            if (material) {
                material->AddRef();
            } else {
                agl::AglTexture* image = newTexture->GetLoadState() == kGuiLoad_Loaded
                                             ? newTexture->texture
                                             : agl::GetDefaultTexture(0);
                material = agl::CreateSpriteMaterial(image);
                SetBlendMode(blend);
            }
        }
    }
    SetFlag(kFlag_Dirty, true);
}

// @func 0x007962A0 ApeGuiImage::GetType
int32_t ApeGuiImage::GetType() const
{
    return kGuiObject_Type1;
}

// @func 0x007962A8 ApeGuiImage::SetSize
void ApeGuiImage::SetSize(const float value[2])
{
    size[0] = value[0];
    size[1] = value[1];
    Invalidate();
}

// @func 0x007962F4 ApeGuiImage::GetExpand
bool ApeGuiImage::GetExpand() const
{
    return HasFlag(kFlag_Expand);
}

// @func 0x0079630C ApeGuiImage::SetExpand
void ApeGuiImage::SetExpand(bool value)
{
    SetFlag(kFlag_Expand, value);
    Invalidate();
}

// ---------------------------------------------------------------------------
// Gradient
// ---------------------------------------------------------------------------

// @func 0x007963EC ApeGuiGradient::GetType
int32_t ApeGuiGradient::GetType() const
{
    return kGuiObject_Type2;
}

// @func 0x007963F4 ApeGuiGradient::GetSize
void ApeGuiGradient::GetSize(float out[2]) const
{
    out[0] = size[0];
    out[1] = size[1];
}

// @func 0x00796408 ApeGuiGradient::SetSize
void ApeGuiGradient::SetSize(const float value[2])
{
    size[0] = value[0];
    size[1] = value[1];
    Invalidate();
}

// @func 0x00796454 ApeGuiGradient::GetColor
xml::Color8 ApeGuiGradient::GetColor() const
{
    return color;
}

// @func 0x00796480 ApeGuiGradient::SetColor
void ApeGuiGradient::SetColor(xml::Color8 value)
{
    color = value;
}

// @func 0x004E6224 ApeGuiGradient::~ApeGuiGradient
// @func 0x004E63CC ApeGuiGradient::~ApeGuiGradient (deleting)
ApeGuiGradient::~ApeGuiGradient()
{
    if (material)
        material->Release();
}

// @func 0x004DD08C ApeGuiGradient::SetBlendMode
void ApeGuiGradient::SetBlendMode(uint32_t mode)
{
    ApeGuiObject::SetBlendMode(mode);
    if (material)
        ApplyGuiBlendMode(material, mode);
}

// ---------------------------------------------------------------------------
// Label and ticker
// ---------------------------------------------------------------------------

// @func 0x004E1604 ApeGuiLabel::~ApeGuiLabel
// @func 0x004E181C ApeGuiLabel::~ApeGuiLabel (thunk)
// @func 0x004E17D4 ApeGuiLabel::~ApeGuiLabel (deleting)
// NOTE: the font reference taken by the constructor is not released, as on
// the console; the text and glyph buffers free themselves.
ApeGuiLabel::~ApeGuiLabel() = default;

// @func 0x00795E6C ApeGuiLabel::GetType
int32_t ApeGuiLabel::GetType() const
{
    return kGuiObject_Text;
}

// @func 0x004DC6F4 ApeGuiLabel::GetText
const char* ApeGuiLabel::GetText() const
{
    return text.c_str();
}

// @func 0x007964A8 ApeGuiTicker::~ApeGuiTicker
// @func 0x007964D4 ApeGuiTicker::~ApeGuiTicker (deleting)
ApeGuiTicker::~ApeGuiTicker() = default;

// @func 0x00796528 ApeGuiTicker::GetType
int32_t ApeGuiTicker::GetType() const
{
    return kGuiObject_Type5;
}

// ---------------------------------------------------------------------------
// Tile
// ---------------------------------------------------------------------------

// @func 0x004E7B28 ApeGuiTile::~ApeGuiTile
// @func 0x004E7D10 ApeGuiTile::~ApeGuiTile (deleting)
ApeGuiTile::~ApeGuiTile()
{
    for (ApeGuiObject*& piece : pieces) {
        delete piece;
        piece = nullptr;
    }
}

// @func 0x00796530 ApeGuiTile::GetType
int32_t ApeGuiTile::GetType() const
{
    return kGuiObject_Type6;
}

// @func 0x00796598 ApeGuiTile::SetScale
void ApeGuiTile::SetScale(const Vec3& value)
{
    scale.x = value.x;
    scale.y = value.y;
    scale.z = value.z;
    Invalidate();
}

// @func 0x00796538 ApeGuiTile::GetSize
void ApeGuiTile::GetSize(float out[2]) const
{
    out[0] = size[0];
    out[1] = size[1];
}

// @func 0x0079654C ApeGuiTile::SetSize
void ApeGuiTile::SetSize(const float value[2])
{
    size[0] = value[0];
    size[1] = value[1];
    Invalidate();
}

// @func 0x004E82D8 ApeGuiTile::SetColor
void ApeGuiTile::SetColor(xml::Color8 value)
{
    color = value;
    for (ApeGuiObject* piece : pieces) {
        if (piece)
            piece->SetColor(value);
    }
}

// @func 0x004E83B8 ApeGuiTile::SetBlendMode
void ApeGuiTile::SetBlendMode(uint32_t mode)
{
    ApeGuiObject::SetBlendMode(mode);
    for (ApeGuiObject* piece : pieces) {
        if (piece)
            piece->SetBlendMode(mode);
    }
}

// @func 0x007965EC ApeGuiTile::Invalidate
void ApeGuiTile::Invalidate()
{
    layoutDirty = true;
    ApeGuiObject::Invalidate();
}

// ---------------------------------------------------------------------------
// Group
// ---------------------------------------------------------------------------

// @func 0x00794BE8 ApeGuiGroup::~ApeGuiGroup
// @func 0x007966F8 ApeGuiGroup::~ApeGuiGroup (deleting)
ApeGuiGroup::~ApeGuiGroup() = default;

// @func 0x00796690 ApeGuiGroup::GetType
int32_t ApeGuiGroup::GetType() const
{
    return kGuiObject_Type7;
}

// @func 0x00796698 ApeGuiGroup::GetSize
void ApeGuiGroup::GetSize(float out[2]) const
{
    out[0] = size[0];
    out[1] = size[1];
}

// @func 0x007966AC ApeGuiGroup::SetSize
void ApeGuiGroup::SetSize(const float value[2])
{
    size[0] = value[0];
    size[1] = value[1];
    Invalidate();
}

// ---------------------------------------------------------------------------
// Type 8 and the clip region
// ---------------------------------------------------------------------------

// @func 0x004E8F54 ApeGuiType8::~ApeGuiType8
// @func 0x004E90FC ApeGuiType8::~ApeGuiType8 (deleting)
ApeGuiType8::~ApeGuiType8()
{
    if (target)
        target->Release();
}

// @func 0x00796884 ApeGuiType8::GetType
int32_t ApeGuiType8::GetType() const
{
    return kGuiObject_Type8;
}

// @func 0x004E93AC ApeGuiType8::SetVisible
void ApeGuiType8::SetVisible(bool visible)
{
    SetFlag(kFlag_Visible, visible);
    if (target)
        target->SetVisible(visible, true);
}

// @func 0x004E9414 ApeGuiType8::Draw
// The wrapped object draws itself.
void ApeGuiType8::Draw()
{
}

// @func 0x00794D54 ApeGuiClipRegion::~ApeGuiClipRegion
// @func 0x007968F4 ApeGuiClipRegion::~ApeGuiClipRegion (deleting)
ApeGuiClipRegion::~ApeGuiClipRegion() = default;

// @func 0x0079688C ApeGuiClipRegion::GetType
int32_t ApeGuiClipRegion::GetType() const
{
    return kGuiObject_ClipRegion;
}

// @func 0x00796894 ApeGuiClipRegion::GetSize
void ApeGuiClipRegion::GetSize(float out[2]) const
{
    out[0] = size[0];
    out[1] = size[1];
}

// @func 0x007968A8 ApeGuiClipRegion::SetSize
void ApeGuiClipRegion::SetSize(const float value[2])
{
    size[0] = value[0];
    size[1] = value[1];
    Invalidate();
}

} // namespace ape
