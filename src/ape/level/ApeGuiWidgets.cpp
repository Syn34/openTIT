// openTIT - The frame, image, gradient, group and clip region GUI classes:
// construction, copying, layout and drawing.
#include "ape/level/ApeGuiTypes.h"

#include "agl/AglDrawList.h"
#include "agl/AglRenderTarget.h"
#include "agl/AglResources.h"
#include "agl/AglTexture.h"

#include <cmath>

namespace ape {

namespace {

constexpr float kAutoSpacing = 3.4028235e38f;   // FLT_MAX: group spacing chosen from the size
constexpr uint32_t kGuiDrawTag = 0x475549;      // 'GUI' (draw list packet tag)

// The four corners of a GUI quad in triangle strip order, as fractions of
// the size with y pointing down (0x00AEF99C / 0x00AEF9BC), and the gradient
// colour of each (0x00AEF9DC).
constexpr float kQuadCorners[4][2] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };
constexpr int kGradientCornerColor[4] = { ApeGuiGradient::kTopLeft, ApeGuiGradient::kBottomLeft,
                                          ApeGuiGradient::kTopRight, ApeGuiGradient::kBottomRight };

uint8_t Modulate(uint8_t value, uint8_t by) { return (uint8_t)(((uint32_t)value * by) >> 8); }

float AlignH(uint32_t flags) { return (float)((flags >> 14) & 3); }
float AlignV(uint32_t flags) { return (float)((flags >> 18) & 3); }

// Layout size of the sized classes: the size times the world scale, or
// the parent's size when the object expands.
void ScaledLayoutSize(const ApeGuiObject* object, const float size[2], float out[2])
{
    if (object->HasFlag(ApeGuiObject::kFlag_Expand)) {
        GetGuiExpandSize(object, out);
        return;
    }
    const Vec3 scale = object->GetWorldScale();
    out[0] = size[0] * scale.x;
    out[1] = size[1] * scale.y;
}

// Starts a quad covering the object's rectangle: sets the material and the
// matrix and reserves four vertices. 'origin' and 'size' receive the
// rectangle; the caller fills the vertices and ends the primitive.
agl::AglDrawVertex* BeginGuiQuad(const ApeGuiObject* object, agl::AglMaterial* material, float origin[2],
                                 float size[2])
{
    agl::AglDrawList& list = GetGuiDrawList();
    list.SetMaterial(material, kGuiDrawTag);
    list.SetWorldMatrix(object->GetWorldMatrix(), kGuiDrawTag);
    object->GetLayoutSize(size);
    origin[0] = size[0] * -0.5f * AlignH(object->flags);
    origin[1] = size[1] * -0.5f * AlignV(object->flags);
    agl::AglDrawPrimitive primitive;
    list.AllocPrimitive(&primitive, (uint8_t)agl::gpu::PrimitiveType::TriangleStrip, 4, kGuiDrawTag);
    return primitive.vertices;
}

void SetQuadPosition(agl::AglDrawVertex& vertex, int corner, const float origin[2], const float size[2])
{
    vertex.position[0] = origin[0] + size[0] * kQuadCorners[corner][0];
    vertex.position[1] = origin[1] + size[1] * kQuadCorners[corner][1];
    vertex.position[2] = 0.0f;
}

agl::AglTexture* ImageOf(const ApeGuiTexture* texture)
{
    return texture->GetLoadState() == kGuiLoad_Loaded ? texture->texture : agl::GetDefaultTexture(0);
}

} // namespace

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

ApeGuiFrame::ApeGuiFrame(const GuiFrameDesc& desc) : ApeGuiObject(desc)
{
    size[0] = desc.size[0];
    size[1] = desc.size[1];
}

// @func 0x004E4BF8 ApeGuiFrame::Clone
ApeGuiObject* ApeGuiFrame::Clone(const char* newName, ApeGuiObject* newParent, void* newOwner,
                                 bool recursive) const
{
    GuiFrameDesc desc;
    FillCloneDesc(desc, newName, newParent, newOwner);
    desc.size[0] = size[0];
    desc.size[1] = size[1];
    auto* copy = new ApeGuiFrame(desc);
    if (recursive)
        CloneChildren(copy, newOwner);
    return copy;
}

// @func 0x004E4E78 ApeGuiFrame::GetLayoutSize
void ApeGuiFrame::GetLayoutSize(float out[2]) const
{
    ScaledLayoutSize(this, size, out);
}

// ---------------------------------------------------------------------------
// Image
// ---------------------------------------------------------------------------

ApeGuiImage::ApeGuiImage(const GuiImageDesc& desc) : ApeGuiObject(desc)
{
    material = desc.material;
    uv[0] = desc.uv[0];
    uv[1] = desc.uv[1];
    st[0] = desc.st[0];
    st[1] = desc.st[1];
    size[0] = desc.size[0];
    size[1] = desc.size[1];
    if (material)
        material->AddRef();
    else
        SetTexture(desc.texture, uv, st);
}

// @func 0x004E53F0 ApeGuiImage::Clone
ApeGuiObject* ApeGuiImage::Clone(const char* newName, ApeGuiObject* newParent, void* newOwner,
                                 bool recursive) const
{
    GuiImageDesc desc;
    FillCloneDesc(desc, newName, newParent, newOwner);
    desc.texture = texture;
    // A material without a texture resource is shared as it is.
    desc.material = texture ? nullptr : material;
    desc.uv[0] = uv[0];
    desc.uv[1] = uv[1];
    desc.st[0] = st[0];
    desc.st[1] = st[1];
    desc.size[0] = size[0];
    desc.size[1] = size[1];
    auto* copy = new ApeGuiImage(desc);
    if (recursive)
        CloneChildren(copy, newOwner);
    return copy;
}

// @func 0x004E59C8 ApeGuiImage::GetSize
// Without an explicit size the image has its texture's size (in texels,
// times the texture coordinate extent).
void ApeGuiImage::GetSize(float out[2]) const
{
    if (size[0] >= 0.0f && size[1] >= 0.0f) {
        out[0] = size[0];
        out[1] = size[1];
        return;
    }
    out[0] = out[1] = 0.0f;
    if (material && material->textures[0]) {
        const agl::AglTexture* image = material->textures[0];
        out[0] = (float)image->width * std::fabs(st[0]);
        out[1] = (float)image->height * std::fabs(st[1]);
    }
}

// @func 0x004E57A0 ApeGuiImage::SetBlendMode
// Images of a texture resource use the resource's material for the mode.
void ApeGuiImage::SetBlendMode(uint32_t mode)
{
    ApeGuiObject::SetBlendMode(mode);
    if (material && texture) {
        material->Release();
        material = texture->materials[GetBlendMode()];
        if (material) {
            material->AddRef();
            return;
        }
        material = agl::CreateSpriteMaterial(ImageOf(texture));
    }
    if (material)
        ApplyGuiBlendMode(material, mode);
}

// @func 0x004E5AC8 ApeGuiImage::GetLayoutSize
void ApeGuiImage::GetLayoutSize(float out[2]) const
{
    if (HasFlag(kFlag_Dirty)) {
        if (HasFlag(kFlag_Expand)) {
            GetGuiExpandSize(this, layoutSize);
        } else if (size[0] < 0.0f || size[1] < 0.0f) {
            layoutSize[0] = layoutSize[1] = 0.0f;
            if (material && material->textures[0]) {
                const agl::AglTexture* image = material->textures[0];
                const Vec3 scale = GetWorldScale();
                layoutSize[0] = (float)image->width * std::fabs(st[0]) * scale.x;
                layoutSize[1] = (float)image->height * std::fabs(st[1]) * scale.y;
            }
        } else {
            const Vec3 scale = GetWorldScale();
            layoutSize[0] = size[0] * scale.x;
            layoutSize[1] = size[1] * scale.y;
        }
        flags &= ~kFlag_Dirty;
    }
    out[0] = layoutSize[0];
    out[1] = layoutSize[1];
}

// @func 0x004E5D48 ApeGuiImage::Draw
void ApeGuiImage::Draw()
{
    if (IsVisible() && material) {
        float origin[2], quadSize[2];
        agl::AglDrawVertex* vertices = BeginGuiQuad(this, material, origin, quadSize);
        const xml::Color8 tint = GetWorldColor();
        for (int i = 0; i < 4; ++i) {
            agl::AglDrawVertex& v = vertices[i];
            SetQuadPosition(v, i, origin, quadSize);
            v.color[0] = tint.r;
            v.color[1] = tint.g;
            v.color[2] = tint.b;
            v.color[3] = tint.a;
            v.uv[0] = uv[0] + st[0] * kQuadCorners[i][0];
            v.uv[1] = uv[1] + st[1] * kQuadCorners[i][1];
        }
        GetGuiDrawList().EndPrimitive();
    }
    ApeGuiObject::Draw();
}

// ---------------------------------------------------------------------------
// Gradient
// ---------------------------------------------------------------------------

ApeGuiGradient::ApeGuiGradient(const GuiGradientDesc& desc) : ApeGuiObject(desc)
{
    size[0] = desc.size[0];
    size[1] = desc.size[1];
    for (int i = 0; i < 4; ++i)
        colors[i] = desc.colors[i];
    material = agl::CreateSpriteMaterial(agl::GetDefaultTexture(1));
    SetBlendMode(GetBlendMode());
}

// @func 0x004E6414 ApeGuiGradient::Clone
ApeGuiObject* ApeGuiGradient::Clone(const char* newName, ApeGuiObject* newParent, void* newOwner,
                                    bool recursive) const
{
    GuiGradientDesc desc;
    FillCloneDesc(desc, newName, newParent, newOwner);
    desc.size[0] = size[0];
    desc.size[1] = size[1];
    for (int i = 0; i < 4; ++i)
        desc.colors[i] = colors[i];
    auto* copy = new ApeGuiGradient(desc);
    if (recursive)
        CloneChildren(copy, newOwner);
    return copy;
}

// @func 0x004E687C ApeGuiGradient::GetLayoutSize
void ApeGuiGradient::GetLayoutSize(float out[2]) const
{
    if (HasFlag(kFlag_Dirty)) {
        ScaledLayoutSize(this, size, layoutSize);
        flags &= ~kFlag_Dirty;
    }
    out[0] = layoutSize[0];
    out[1] = layoutSize[1];
}

// @func 0x004E69D0 ApeGuiGradient::Draw
void ApeGuiGradient::Draw()
{
    if (IsVisible() && material) {
        float origin[2], quadSize[2];
        agl::AglDrawVertex* vertices = BeginGuiQuad(this, material, origin, quadSize);
        const xml::Color8 tint = GetWorldColor();
        for (int i = 0; i < 4; ++i) {
            agl::AglDrawVertex& v = vertices[i];
            SetQuadPosition(v, i, origin, quadSize);
            const xml::Color8& corner = colors[kGradientCornerColor[i]];
            v.color[0] = Modulate(corner.r, tint.r);
            v.color[1] = Modulate(corner.g, tint.g);
            v.color[2] = Modulate(corner.b, tint.b);
            v.color[3] = Modulate(corner.a, tint.a);
            v.uv[0] = kQuadCorners[i][0];
            v.uv[1] = kQuadCorners[i][1];
        }
        GetGuiDrawList().EndPrimitive();
    }
    ApeGuiObject::Draw();
}

// ---------------------------------------------------------------------------
// Group
// ---------------------------------------------------------------------------

ApeGuiGroup::ApeGuiGroup(const GuiGroupDesc& desc) : ApeGuiObject(desc)
{
    size[0] = desc.size[0];
    size[1] = desc.size[1];
    offset[0] = desc.offset[0];
    offset[1] = desc.offset[1];
    flow = (uint8_t)desc.flow;
    arrange = (uint8_t)desc.arrange;
    spacing = desc.spacing;
}

// @func 0x004E8460 ApeGuiGroup::Clone
ApeGuiObject* ApeGuiGroup::Clone(const char* newName, ApeGuiObject* newParent, void* newOwner,
                                 bool recursive) const
{
    GuiGroupDesc desc;
    FillCloneDesc(desc, newName, newParent, newOwner);
    desc.size[0] = size[0];
    desc.size[1] = size[1];
    desc.offset[0] = offset[0];
    desc.offset[1] = offset[1];
    desc.flow = flow;
    desc.arrange = arrange;
    desc.spacing = spacing;
    auto* copy = new ApeGuiGroup(desc);
    if (recursive)
        CloneChildren(copy, newOwner);
    return copy;
}

// @func 0x004E8E3C ApeGuiGroup::GetLayoutSize
void ApeGuiGroup::GetLayoutSize(float out[2]) const
{
    ScaledLayoutSize(this, size, out);
}

// @func 0x004E87A0 ApeGuiGroup::Tick
// Re-arranges the visible children whenever the group or one of them
// changed: each child is anchored to the group's middle along the flow axis
// and placed one after the other along it.
void ApeGuiGroup::Tick(float dt)
{
    if (IsVisible()) {
        bool dirty = HasFlag(kFlag_Dirty);
        for (ApeGuiObject* child = firstChild; child && !dirty; child = child->nextSibling)
            dirty = child->HasFlag(kFlag_Dirty);

        if (dirty) {
            float groupSize[2];
            GetLayoutSize(groupSize);
            float total[2] = { 0.0f, 0.0f };
            uint32_t count = 0;
            for (ApeGuiObject* child = firstChild; child; child = child->nextSibling) {
                if (!child->IsVisible())
                    continue;
                ++count;
                float childSize[2];
                child->GetLayoutSize(childSize);
                total[0] += childSize[0];
                total[1] += childSize[1];
            }

            const int axis = flow == kGuiGroup_Row ? 0 : 1;
            const bool centered = arrange == kGuiGroup_Center;
            float gap = spacing;
            if (spacing == kAutoSpacing) {
                gap = 0.0f;
                if (groupSize[axis] >= 1e-5f) {
                    float screenGap;
                    if (centered)
                        screenGap = count > 1 ? groupSize[axis] / (float)(count - 1) : 0.0f;
                    else
                        screenGap = (groupSize[axis] - total[axis]) / (float)(count + 1);
                    // Back to unscaled units.
                    float unscaled[2];
                    GetSize(unscaled);
                    gap = screenGap * (unscaled[axis] / groupSize[axis]);
                }
            }

            float cursor = 0.0f;
            if (count) {
                if (centered) {
                    cursor = gap * (float)(count - 1) * -0.5f;
                } else {
                    const float align = axis == 0 ? AlignH(flags) : AlignV(flags);
                    cursor = (gap * (float)(count - 1) + total[axis]) * (align * -0.5f);
                }
            }

            for (ApeGuiObject* child = firstChild; child; child = child->nextSibling) {
                if (!child->IsVisible())
                    continue;
                uint32_t anchor = child->GetAnchor();
                const uint32_t align = child->GetAlign();
                float childSize[2];
                child->GetLayoutSize(childSize);
                Vec3 position = child->GetPosition();
                if (flow == kGuiGroup_Column)
                    anchor = (anchor & ~0x30u) | kGuiAlign_Middle;
                else
                    anchor = (anchor & ~0x03u) | kGuiAlign_Center;
                child->SetAnchor(anchor);

                int alignSteps = (int)((align >> (axis * 4)) & 3);
                if (centered)
                    alignSteps -= 1;
                const float placed = (float)alignSteps * childSize[axis] * 0.5f + cursor + offset[axis];
                if (axis == 0)
                    position.x = placed;
                else
                    position.y = placed;
                child->SetPosition(position);
                cursor += centered ? gap : childSize[axis] + gap;
            }
            flags &= ~kFlag_Dirty;
        }
    }
    ApeGuiObject::Tick(dt);
}

// ---------------------------------------------------------------------------
// Clip region
// ---------------------------------------------------------------------------

ApeGuiClipRegion::ApeGuiClipRegion(const GuiClipRegionDesc& desc) : ApeGuiObject(desc)
{
    size[0] = desc.size[0];
    size[1] = desc.size[1];
    projectedSize[0] = desc.projectedSize[0];
    projectedSize[1] = desc.projectedSize[1];
}

// @func 0x004E9424 ApeGuiClipRegion::Clone
// NOTE: the projected size is not copied (the copy keeps zero), as on the
// console.
ApeGuiObject* ApeGuiClipRegion::Clone(const char* newName, ApeGuiObject* newParent, void* newOwner,
                                      bool recursive) const
{
    GuiClipRegionDesc desc;
    FillCloneDesc(desc, newName, newParent, newOwner);
    desc.size[0] = size[0];
    desc.size[1] = size[1];
    auto* copy = new ApeGuiClipRegion(desc);
    if (recursive)
        CloneChildren(copy, newOwner);
    return copy;
}

// @func 0x004E971C ApeGuiClipRegion::GetLayoutSize
void ApeGuiClipRegion::GetLayoutSize(float out[2]) const
{
    ScaledLayoutSize(this, size, out);
}

// @func 0x004E9860 ApeGuiClipRegion::Draw
// Draws the children with the scissor rectangle set to the region. Under a
// perspective render target the rectangle is the projection of the
// region's projected size; otherwise the screen rectangle is mapped into
// the target's viewport (render3d regions are skipped then).
void ApeGuiClipRegion::Draw()
{
    if (!IsVisible())
        return;
    agl::AglDrawList& list = GetGuiDrawList();
    const Matrix34 world = GetWorldMatrix();
    agl::AglRenderTarget* target = GetGuiDrawTarget();
    // NOTE: the console assumes a draw target is set.
    const bool projected = target && target->projectionType == agl::kAglProjection_Perspective;
    if (!projected && GetRender3d())
        return;

    float regionSize[2];
    GetLayoutSize(regionSize);
    float screen[2];
    GetGuiScreenSize(screen);
    uint16_t rect[4];
    if (!projected) {
        const float left = std::fmax(regionSize[0] * -0.5f * AlignH(flags) + world.translation.x, 0.0f);
        const float top = std::fmax(regionSize[1] * -0.5f * AlignV(flags) + world.translation.y, 0.0f);
        rect[0] = (uint16_t)(int)left;
        rect[1] = (uint16_t)(int)top;
        rect[2] = (uint16_t)(int)regionSize[0];
        rect[3] = (uint16_t)(int)regionSize[1];
        if (target) {
            const uint16_t* vp = target->viewport;
            rect[0] = (uint16_t)(int)((float)vp[0] + (float)rect[0] * (float)vp[2] / screen[0]);
            rect[1] = (uint16_t)(int)((float)vp[1] + (float)rect[1] * (float)vp[3] / screen[1]);
            rect[2] = (uint16_t)(int)((float)rect[2] * (float)vp[2] / screen[0]);
            rect[3] = (uint16_t)(int)((float)rect[3] * (float)vp[3] / screen[1]);
        }
    } else {
        const float left = projectedSize[0] * -0.5f * AlignH(flags) + world.translation.x;
        const float top = projectedSize[1] * 0.5f * AlignV(flags) + world.translation.y;
        Vec3 topLeft{};
        topLeft.x = left;
        topLeft.y = top;
        topLeft.z = world.translation.z;
        Vec3 bottomRight = topLeft;
        bottomRight.x = left + projectedSize[0];
        bottomRight.y = top - projectedSize[1];
        float a[4], b[4];
        target->WorldToScreen(a, topLeft);
        target->WorldToScreen(b, bottomRight);
        const float ax = a[0] * screen[0], ay = a[1] * screen[1];
        const float bx = b[0] * screen[0], by = b[1] * screen[1];
        if (ax < 0.0f || ay < 0.0f || bx < 0.0f || b[1] < 0.0f)
            return;
        rect[0] = (uint16_t)(int)ax;
        rect[1] = (uint16_t)(int)ay;
        rect[2] = (uint16_t)(int)(bx - ax);
        rect[3] = (uint16_t)(int)(by - ay);
    }

    list.SetScissor(rect, kGuiDrawTag);
    ApeGuiObject::Draw();
    list.SetScissor(nullptr, kGuiDrawTag);
}

} // namespace ape
