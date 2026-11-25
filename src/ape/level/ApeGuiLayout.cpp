// openTIT - GUI object placement: world matrices, screen rectangles, hit
// tests and sibling order.
//
// Every object is placed relative to its parent: the parent's matrix, then
// the anchor point inside the parent's rectangle ("anchor"), then the
// object's own transform. The object's rectangle extends from its origin
// according to its "align" bits. Root objects are placed on the screen
// (the back buffer size). Objects flagged "render3d" are projected through
// the GUI's 3D render target and end up in screen pixels as well.
#include "ape/level/ApeGuiTypes.h"

#include "agl/AglDisplay.h"
#include "agl/AglRenderTarget.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeObj.h"

#include <cmath>

namespace ape {

namespace {

// Horizontal / vertical parts of the anchor (flags bits 6-13) and align
// (flags bits 14-21) fields, as multiples of half the size.
float AnchorH(uint32_t flags) { return (float)((flags >> 6) & 3); }
float AnchorV(uint32_t flags) { return (float)((flags >> 10) & 3); }
float AlignH(uint32_t flags) { return (float)((flags >> 14) & 3); }
float AlignV(uint32_t flags) { return (float)((flags >> 18) & 3); }

Matrix34 Identity()
{
    Matrix34 m{};
    m.xAxis.x = 1.0f;
    m.yAxis.y = 1.0f;
    m.zAxis.z = 1.0f;
    return m;
}

Vec3 TransformPoint(const Matrix34& m, float x, float y, float z)
{
    Vec3 out{};
    out.x = x * m.xAxis.x + y * m.yAxis.x + z * m.zAxis.x + m.translation.x;
    out.y = x * m.xAxis.y + y * m.yAxis.y + z * m.zAxis.y + m.translation.y;
    out.z = x * m.xAxis.z + y * m.yAxis.z + z * m.zAxis.z + m.translation.z;
    return out;
}

// Projects a point of 'world' space to screen pixels relative to 'center'
// (screen fraction) with the 3D GUI render target.
void ProjectOffset(float out[2], agl::AglRenderTarget* target, const Vec3& point, const float center[2],
                   float screenW, float screenH)
{
    float screen[4];
    target->WorldToScreen(screen, point);
    out[0] = (screen[0] - center[0]) * screenW;
    out[1] = (screen[1] - center[1]) * screenH;
}

} // namespace

void GetGuiExpandSize(const ApeGuiObject* object, float out[2])
{
    if (object->parent)
        object->parent->GetLayoutSize(out);
    else
        GetGuiScreenSize(out);
}

// @func 0x004E0B30 ApeGuiObject::GetWorldMatrix
Matrix34 ApeGuiObject::GetWorldMatrix() const
{
    Matrix34 parentWorld;
    float parentSize[2];
    float parentOrigin[2];
    if (!parent) {
        parentWorld = Identity();
        GetGuiScreenSize(parentSize);
        parentOrigin[0] = parentOrigin[1] = 0.0f;
    } else {
        parent->GetLayoutSize(parentSize);
        parentOrigin[0] = parentSize[0] * -0.5f * AlignH(parent->flags);
        parentOrigin[1] = parentSize[1] * -0.5f * AlignV(parent->flags);
        parentWorld = parent->GetWorldMatrix();
    }

    // The position is in the parent's (scaled) units.
    Vec3 position = transform.translation;
    if (parent && HasFlag(kFlag_InheritScale)) {
        const Vec3 parentScale = parent->GetWorldScale();
        position.x *= parentScale.x;
        position.y *= parentScale.y;
        position.z *= parentScale.z;
    }

    Matrix34 local{};
    local.xAxis = { transform.xAxis.x, transform.xAxis.y, transform.xAxis.z, 0.0f };
    local.yAxis = { transform.yAxis.x, transform.yAxis.y, transform.yAxis.z, 0.0f };
    local.zAxis = { transform.zAxis.x, transform.zAxis.y, transform.zAxis.z, 0.0f };
    local.translation.x = parentSize[0] * 0.5f * AnchorH(flags) + parentOrigin[0] + position.x;
    local.translation.y = parentSize[1] * 0.5f * AnchorV(flags) + parentOrigin[1] + position.y;
    local.translation.z = position.z;

    Matrix34 result;
    world::Matrix34Mul(result, local, parentWorld);
    return result;
}

// @func 0x004E0648 ApeGuiObject::GetScreenRect
void ApeGuiObject::GetScreenRect(float min[2], float max[2], Matrix34& world) const
{
    float size[2];
    GetLayoutSize(size);
    min[0] = size[0] * -0.5f * AlignH(flags);
    min[1] = size[1] * -0.5f * AlignV(flags);
    max[0] = min[0] + size[0];
    max[1] = min[1] + size[1];
    world = GetWorldMatrix();

    agl::AglRenderTarget* target = GetGui3dRenderTarget();
    if (!GetRender3d() || !target)
        return;

    // Project the origin and both corners; the rectangle becomes pixel
    // offsets from the projected origin.
    float screenSize[2];
    GetGuiScreenSize(screenSize);
    float center[4];
    target->WorldToScreen(center, world.translation);
    const Vec3 minPoint = TransformPoint(world, min[0], min[1], 0.0f);
    ProjectOffset(min, target, minPoint, center, screenSize[0], screenSize[1]);
    const Vec3 maxPoint = TransformPoint(world, max[0], max[1], 0.0f);
    ProjectOffset(max, target, maxPoint, center, screenSize[0], screenSize[1]);
    world.translation.x = center[0] * screenSize[0];
    world.translation.y = center[1] * screenSize[1];
    world.translation.z = 0.0f;
}

// @func 0x004E03F0 ApeGuiObject::GetCenter
Vec3 ApeGuiObject::GetCenter() const
{
    float min[2], max[2];
    Matrix34 world;
    GetScreenRect(min, max, world);
    return TransformPoint(world, (min[0] + max[0]) * 0.5f, (min[1] + max[1]) * 0.5f, 0.0f);
}

namespace {

// True when the screen point lies inside the object's rectangle.
bool RectContains(const ApeGuiObject* object, const float point[2])
{
    float min[2], max[2];
    Matrix34 world;
    object->GetScreenRect(min, max, world);
    Vec3 p{};
    p.x = point[0];
    p.y = point[1];
    Vec3 local;
    Matrix34InvTransformPoint(local, p, world);
    return local.x >= min[0] && local.x <= max[0] && local.y >= min[1] && local.y <= max[1];
}

// Hit test of the children; the last (topmost) child hit wins.
ApeGuiObject* HitTestChildren(ApeGuiObject* object, const float point[2])
{
    ApeGuiObject* hit = nullptr;
    for (ApeGuiObject* child = object->firstChild; child; child = child->nextSibling) {
        if (ApeGuiObject* found = child->HitTest(point))
            hit = found;
    }
    return hit;
}

} // namespace

// @func 0x004E1140 ApeGuiObject::HitTest
// Hidden and fully transparent objects are skipped with their subtree.
ApeGuiObject* ApeGuiObject::HitTest(const float point[2])
{
    if (!IsVisible() || color.a == 0)
        return nullptr;
    if (ApeGuiObject* hit = HitTestChildren(this, point))
        return hit;
    return RectContains(this, point) ? this : nullptr;
}

// @func 0x004E4FB0 ApeGuiFrame::HitTest
// Frames are invisible hot spots: they can be hit while hidden (only their
// children are skipped then). An expanded frame never hits itself.
ApeGuiObject* ApeGuiFrame::HitTest(const float point[2])
{
    ApeGuiObject* hit = IsVisible() ? HitTestChildren(this, point) : nullptr;
    if (hit || HasFlag(kFlag_Expand))
        return hit;
    return RectContains(this, point) ? this : nullptr;
}

// @func 0x004E02DC ApeGuiObject::BringToFront
void ApeGuiObject::BringToFront(bool recursive)
{
    for (ApeGuiObject* object = this; object; object = object->parent) {
        if (object->nextSibling) {
            ApeGuiObject** head = object->parent ? &object->parent->firstChild : &GuiRootObjects();
            ApeGuiObject** link = head;
            while (*link != object)
                link = &(*link)->nextSibling;
            *link = object->nextSibling;
            object->nextSibling = nullptr;
            link = head;
            while (*link)
                link = &(*link)->nextSibling;
            *link = object;
        }
        if (!recursive)
            break;
    }
}

void ApeGuiObject::FillCloneDesc(GuiObjectDesc& desc, const char* newName, ApeGuiObject* newParent,
                                 void* newOwner) const
{
    desc.name = newName;
    desc.transform = transform;
    desc.scale = scale;
    desc.color = color;
    desc.blendMode = GetBlendMode();
    desc.parent = newParent;
    desc.owner = newOwner;
    desc.align = (flags >> 14) & 0xFF;
    desc.anchor = (flags >> 6) & 0xFF;
    desc.visible = HasFlag(kFlag_Visible);
    desc.flag30 = HasFlag(kFlag_30);
    desc.expand = HasFlag(kFlag_Expand);
    desc.inheritScale = HasFlag(kFlag_InheritScale);
    desc.inheritRgb = HasFlag(kFlag_InheritRgb);
    desc.inheritAlpha = HasFlag(kFlag_InheritAlpha);
    desc.render3d = HasFlag(kFlag_Render3d);
}

// @func 0x004D7634 ApeGuiObject::CloneChildren
void ApeGuiObject::CloneChildren(ApeGuiObject* newParent, void* newOwner) const
{
    for (ApeGuiObject* child = firstChild; child; child = child->nextSibling)
        child->Clone(child->name, newParent, newOwner, true);
}

// @func 0x004D3E60 FindGuiFocusAt
ApeGuiObject* FindGuiFocusAt(ApeGuiObject* object, uint32_t pad, const float point[2], float* best)
{
    ApeGuiObject* found = nullptr;
    if (!object->IsVisible())
        return nullptr;
    if (object->HasFlag(ApeGuiObject::kFlag_30)) {
        float min[2], max[2];
        Matrix34 world;
        object->GetScreenRect(min, max, world);
        Vec3 p{};
        p.x = point[0];
        p.y = point[1];
        Vec3 local;
        Matrix34InvTransformPoint(local, p, world);
        if (local.x >= min[0] && local.x <= max[0] && local.y >= min[1] && local.y <= max[1]) {
            const float distanceSq = local.x * local.x + local.y * local.y + local.z * local.z;
            if (distanceSq < *best && SendGuiEvent(kGuiEvent_CanFocus, pad, object, reinterpret_cast<uintptr_t>(found))) {
                found = object;
                *best = distanceSq;
            }
        }
    }
    for (ApeGuiObject* child = object->firstChild; child; child = child->nextSibling) {
        if (ApeGuiObject* hit = FindGuiFocusAt(child, pad, point, best))
            found = hit;
    }
    return found;
}

// @func 0x004D4178 FindGuiFocusInDirection
// Score: the angle off the direction (0..1) plus the distance in screen
// heights; candidates behind 'current' count from the opposite screen edge.
ApeGuiObject* FindGuiFocusInDirection(ApeGuiObject* object, uint32_t pad, ApeGuiObject* current, const float dir[2],
                                      float* best)
{
    ApeGuiObject* found = nullptr;
    if (!object->IsVisible())
        return nullptr;
    if (object->HasFlag(ApeGuiObject::kFlag_30) && object != current) {
        float screen[2];
        GetGuiScreenSize(screen);
        float delta[2] = { 0.0f, 0.0f };
        float along = 0.0f;
        if (current) {
            float min[2], max[2];
            Matrix34 from, to;
            current->GetScreenRect(min, max, from);
            object->GetScreenRect(min, max, to);
            delta[0] = to.translation.x - from.translation.x;
            delta[1] = to.translation.y - from.translation.y;
            along = delta[0] * dir[0] + delta[1] * dir[1];
            if (along < 0.0f) {
                auto sign = [](float v) { return (float)((v > 1e-5f) - (v < -1e-5f)); };
                delta[0] = (sign(dir[0]) * screen[0] + to.translation.x) - from.translation.x;
                delta[1] = (sign(dir[1]) * screen[1] + to.translation.y) - from.translation.y;
                along = delta[0] * dir[0] + delta[1] * dir[1];
            }
        }
        if (!current || along > 0.0f) {
            float distance = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
            float angle = 0.0f;
            if (distance > 0.0f) {
                const float cosine = std::fmin(std::fmax(along / distance, -1.0f), 1.0f);
                angle = std::acos(cosine) / 3.1415927f;
                distance /= screen[1];
            }
            const float score = angle + distance;
            if (score < *best && SendGuiEvent(kGuiEvent_CanFocus, pad, object, reinterpret_cast<uintptr_t>(found))) {
                *best = score;
                found = object;
            }
        }
    }
    for (ApeGuiObject* child = object->firstChild; child; child = child->nextSibling) {
        if (ApeGuiObject* hit = FindGuiFocusInDirection(child, pad, current, dir, best))
            found = hit;
    }
    return found;
}

} // namespace ape
