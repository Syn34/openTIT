// openTIT - ApeObj: base class of every placed world object (instances).
#include "ape/world/ApeObj.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeRenderNode.h"
#include "ape/world/ApeSkeleton.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ape::world {

bool g_dirtyRootListLocked = false;

ApeTArray<ApeObj*>& GetDirtyRootList()
{
    static ApeTArray<ApeObj*> s_list(64);
    return s_list;
}

ApeTArray<ApeObj*>& GetDynamicObjectList()
{
    static ApeTArray<ApeObj*> s_list(64);
    return s_list;
}

// --- matrix helpers ------------------------------------------------------------

void Matrix34Mul(Matrix34& out, const Matrix34& a, const Matrix34& b)
{
    auto row = [&b](const Vec3& r) {
        return Vec3{r.x * b.xAxis.x + r.y * b.yAxis.x + r.z * b.zAxis.x,
                    r.x * b.xAxis.y + r.y * b.yAxis.y + r.z * b.zAxis.y,
                    r.x * b.xAxis.z + r.y * b.yAxis.z + r.z * b.zAxis.z, 0.0f};
    };
    const Vec3 x = row(a.xAxis), y = row(a.yAxis), z = row(a.zAxis);
    Vec3 t = row(a.translation);
    t.x += b.translation.x;
    t.y += b.translation.y;
    t.z += b.translation.z;
    out.xAxis = x;
    out.yAxis = y;
    out.zAxis = z;
    out.translation = t;
}

void Matrix34FromBasis(Matrix34& out, const float xBasis[3], const float zBasis[3], const float position[3])
{
    out.xAxis = {xBasis[0], xBasis[1], xBasis[2], 0.0f};
    out.zAxis = {zBasis[0], zBasis[1], zBasis[2], 0.0f};
    out.yAxis = {zBasis[1] * xBasis[2] - zBasis[2] * xBasis[1],
                 zBasis[2] * xBasis[0] - zBasis[0] * xBasis[2],
                 zBasis[0] * xBasis[1] - zBasis[1] * xBasis[0], 0.0f};
    out.translation = {position[0], position[1], position[2], 0.0f};
}

namespace {

bool DiffersFrom(const Vec3& a, const Vec3& b)
{
    return std::fabs(a.x - b.x) >= kScaleEpsilon || std::fabs(a.y - b.y) >= kScaleEpsilon ||
           std::fabs(a.z - b.z) >= kScaleEpsilon;
}

// v * rotation(m), no translation.
Vec3 RotateVector(const Vec3& v, const Matrix34& m)
{
    return {v.x * m.xAxis.x + v.y * m.yAxis.x + v.z * m.zAxis.x,
            v.x * m.xAxis.y + v.y * m.yAxis.y + v.z * m.zAxis.y,
            v.x * m.xAxis.z + v.y * m.yAxis.z + v.z * m.zAxis.z, 0.0f};
}

Vec3 TransformPoint(const Vec3& p, const Matrix34& m)
{
    Vec3 r = RotateVector(p, m);
    r.x += m.translation.x;
    r.y += m.translation.y;
    r.z += m.translation.z;
    return r;
}

// Inverse of a rigid transform (transposed rotation).
void InvertRigid(Matrix34& out, const Matrix34& m)
{
    const Vec3 x = m.xAxis, y = m.yAxis, z = m.zAxis, t = m.translation;
    out.xAxis = {x.x, y.x, z.x, 0.0f};
    out.yAxis = {x.y, y.y, z.y, 0.0f};
    out.zAxis = {x.z, y.z, z.z, 0.0f};
    out.translation = {-(t.x * x.x + t.y * x.y + t.z * x.z),
                       -(t.x * y.x + t.y * y.y + t.z * y.z),
                       -(t.x * z.x + t.y * z.y + t.z * z.z), 0.0f};
}

// Integer parts of a position, used to decide whether an object "moved".
struct TruncatedPos {
    int x, y, z;
    bool operator!=(const TruncatedPos& o) const { return x != o.x || y != o.y || z != o.z; }
};
TruncatedPos Truncate(const Vec3& v)
{
    return {int(v.x), int(v.y), int(v.z)};
}

} // namespace

// --- construction ------------------------------------------------------------------

// The constructor is inlined into every instance constructor on the console
// (see the render object constructor 0x00580708).
ApeObj::ApeObj(uint32_t type) : ApeBase(type)
{
    m_name[0] = '\0';
    Reset();
}

// @func 0x0057DCF0 ApeObj::~ApeObj
// @func 0x0057DDDC ApeObj::~ApeObj (deleting)
ApeObj::~ApeObj()
{
    if (m_flags & kObjFlag_InDirtyList) {
        ApeTArray<ApeObj*>& list = GetDirtyRootList();
        for (int i = int(list.Count()) - 1; i >= 0; --i) {
            if (list[size_t(i)] == this) {
                list.RemoveAtSwap(size_t(i));
                m_flags &= ~kObjFlag_InDirtyList;
                break;
            }
        }
    }
    ReleaseRenderNodes();
    SetDef(nullptr);
}

// @func 0x0057D688 ApeObj::Reset
void ApeObj::Reset()
{
    Matrix34Copy(m_local, kWorldIdentity);
    Matrix34Copy(m_world, kWorldIdentity);
    m_scale = kWorldUnitScale;
    m_renderNode = kNoRenderNode;
    // Keeps the low byte, drops dirty/queued/scale state, becomes visible.
    m_flags = uint16_t((m_flags & 0x00FF) | kObjFlag_Visible);
    m_byteA8 = 0xFF;
    m_xformFlags = 0;
    m_pivot = &kWorldIdentity;
    m_nameHash = 0;
    m_def = nullptr;
    m_worldBounds = kWorldEmptySphere;
    m_root = this;
    m_firstChild = nullptr;
    m_nextSibling = nullptr;
    m_parent = nullptr;
    m_parentBone = 0;
    SetName("ApeObj::Unknown", true);
}

// @func 0x0057D368 ApeObj::SetName
void ApeObj::SetName(const char* name, bool updateHash)
{
    StrCopyBounded(m_name, name, sizeof(m_name));
    if (updateHash)
        m_nameHash = HashNameNoCase(name);
}

// @func 0x0057D3E8 ApeObj::InitFromInstanceData
bool ApeObj::InitFromInstanceData(const ApeObjInstanceData& data)
{
    m_nameHash = data.nameHash;
    StrCopyBounded(m_name, data.name, sizeof(m_name));
    Matrix34FromBasis(m_local, data.xBasis, data.zBasis, data.position);
    MarkDirty();

    const Vec3 scale = {data.scale[0], data.scale[1], data.scale[2], 0.0f};
    SetScale(scale, false);

    uint16_t flags = uint16_t(m_flags & ~(kObjFlag_Visible | kObjFlag_4000 | kObjFlag_C0));
    if (data.flags & kInstanceFlag_Visible)
        flags |= kObjFlag_Visible;
    if (data.flags & kInstanceFlag_02)
        flags |= kObjFlag_4000;
    flags |= uint16_t(data.flags & kInstanceFlag_C0);
    m_flags = flags;

    ApeDef* def = FindDef(data.defHash, GetType());
    if (!def)
        std::fprintf(stderr, "Definition not found for instance: (%s)\n", GetName());
    SetDef(def);
    return true;
}

// @func 0x0057DEEC ApeObj::CopyFrom
void ApeObj::CopyFrom(const ApeObj& other, uint32_t skipMask)
{
    if (!(skipMask & kCopy_SkipTags))
        ShareTags(other);

    if (!(skipMask & kCopy_SkipTransform)) {
        Matrix34Copy(m_local, other.m_local);
        Matrix34Copy(m_world, other.m_world);
        m_scale = other.m_scale;
        m_pivot = &kWorldIdentity;
    }

    if (!(skipMask & kCopy_SkipFlags)) {
        const uint16_t copied = kObjFlag_Visible | kObjFlag_4000 | kObjFlag_HasScale | kObjFlag_1000 |
                                kObjFlag_0100 | kObjFlag_C0;
        m_flags = uint16_t((m_flags & ~copied) | (other.m_flags & copied) | kObjFlag_Dirty);
        m_byteA8 = other.m_byteA8;
        const uint8_t xformCopied = kXform_IgnoreParent | kXform_ScaleTranslation | kXform_InheritScale | kXform_01;
        m_xformFlags = uint8_t((m_xformFlags & ~xformCopied) | (other.m_xformFlags & xformCopied));
    }

    if (!(skipMask & kCopy_SkipName)) {
        StrCopy(m_name, other.m_name);
        m_nameHash = other.m_nameHash;
    }

    if (!(skipMask & kCopy_SkipDef))
        SetDef(other.m_def);
}

// --- dirty tracking ------------------------------------------------------------------

// @func 0x0057D000 ApeObj::MarkDirty
void ApeObj::MarkDirty()
{
    if (m_flags & kObjFlag_Dirty)
        return;
    ApeObj* root = m_root;
    if (!(root->m_flags & kObjFlag_Dirty)) {
        if (!(root->m_flags & kObjFlag_InDirtyList) && !root->m_parent && !g_dirtyRootListLocked) {
            GetDirtyRootList().PushBack(root);
            root->m_flags |= kObjFlag_InDirtyList;
        }
        root->m_flags |= kObjFlag_Dirty;
    }
    m_flags |= kObjFlag_Dirty;
}

// @func 0x0057D100 ApeObj::SetScale
void ApeObj::SetScale(const Vec3& scale, bool recurse)
{
    if (!DiffersFrom(m_scale, scale))
        return;
    m_scale.x = scale.x;
    m_scale.y = scale.y;
    m_scale.z = scale.z;
    if (DiffersFrom(scale, kWorldUnitScale))
        m_flags |= kObjFlag_HasScale;
    else
        m_flags &= ~kObjFlag_HasScale;
    MarkDirty();
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
            child->SetScale(scale, recurse);
    }
    OnBoundsChanged();
}

// @func 0x0057FDB4 ApeObj::SetFlag4000
void ApeObj::SetFlag4000(bool value, bool recurse)
{
    if (value)
        m_flags |= kObjFlag_4000;
    else
        m_flags &= ~kObjFlag_4000;
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
            child->SetFlag4000(value, true);
    }
}

// --- definition ----------------------------------------------------------------------

// @func 0x0057D8AC ApeObj::SetDef
void ApeObj::SetDef(ApeDef* def)
{
    if (m_def)
        m_def->Release();
    m_def = def;
    if (!def) {
        m_localBounds = kWorldEmptySphere;
    } else {
        def->AddRef();
        m_localBounds = def->m_sphere;
        MarkDirty();
    }
    OnBoundsChanged();
}

// @func 0x0058049C ApeObj::SetBoundingRadius
void ApeObj::SetBoundingRadius(float radius)
{
    ApeDef* def = m_def;
    const Vec3 sphereCenter = {def->m_sphere.x, def->m_sphere.y, def->m_sphere.z, 0.0f};
    Vec3 boxCenter;
    AabbCenter(def->m_bounds, boxCenter);
    def->m_sphere = {sphereCenter.x, sphereCenter.y, sphereCenter.z, radius};
    m_localBounds = def->m_sphere;
    def->m_bounds.min = {boxCenter.x - radius, boxCenter.y - radius, boxCenter.z - radius, def->m_bounds.min.pad};
    def->m_bounds.max = {boxCenter.x + radius, boxCenter.y + radius, boxCenter.z + radius, def->m_bounds.max.pad};
    MarkDirty();
    UpdateWorld(false, false);
}

// --- world transform ---------------------------------------------------------------

// @func 0x0057E14C ApeObj::UpdateWorldBounds
const Sphere& ApeObj::UpdateWorldBounds()
{
    const float largest = std::fmax(std::fmax(m_scale.x, m_scale.y), m_scale.z);
    const Vec3 center = {m_localBounds.x * largest, m_localBounds.y * largest, m_localBounds.z * largest, 0.0f};
    const Vec3 world = TransformPoint(center, m_world);
    m_worldBounds = {world.x, world.y, world.z, m_localBounds.w * largest};
    return m_worldBounds;
}

// @func 0x0057E260 ApeObj::UpdateWorldFromParent
void ApeObj::UpdateWorldFromParent(const Vec3& parentScale)
{
    // Frame of the parent, or of the parent's bone when attached to one.
    Matrix34 parentFrame = m_parent->m_world;
    if (m_parentBone != -1) {
        Matrix34 bone = m_parent->GetSkeleton()->model[m_parentBone];
        bone.translation.x *= parentScale.x;
        bone.translation.y *= parentScale.y;
        bone.translation.z *= parentScale.z;
        Matrix34Mul(parentFrame, bone, m_parent->m_world);
    }

    Matrix34 local;
    Matrix34Mul(local, *m_pivot, m_local);

    if (m_xformFlags & kXform_IgnoreParent) {
        // Follow the parent's position only.
        local.translation = TransformPoint(local.translation, parentFrame);
        Matrix34Copy(parentFrame, kWorldIdentity);
    } else if (m_xformFlags & kXform_InheritScale) {
        Vec3 scale;
        Matrix34GetScale(parentFrame, scale);
        Matrix34NormalizeAxes(parentFrame);
        SetScale(scale, false);
    } else {
        local.translation.x *= parentScale.x;
        local.translation.y *= parentScale.y;
        local.translation.z *= parentScale.z;
        if (m_xformFlags & kXform_ScaleTranslation)
            Matrix34NormalizeAxes(parentFrame);
    }

    Matrix34Mul(m_world, local, parentFrame);
    m_flags &= ~kObjFlag_Dirty;
    UpdateWorldBounds();

    for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling) {
        const Vec3 childScale = {m_scale.x * parentScale.x, m_scale.y * parentScale.y, m_scale.z * parentScale.z, 0.0f};
        child->UpdateWorldFromParent(childScale);
    }

    if (m_flags & kObjFlag_HasScale)
        Matrix34ScaleAxes(m_world, m_scale);
    OnWorldTransformUpdated(true);
}

// @func 0x0057EE00 ApeObj::UpdateWorld
void ApeObj::UpdateWorld(bool unused, bool forceSkeletonBounds)
{
    if (!(m_flags & kObjFlag_Dirty))
        return;

    const TruncatedPos oldPos = Truncate(m_world.translation);
    if (m_flags & kObjFlag_4000) {
        // The local matrix holds the inverse transform (camera style).
        Matrix34 inverse;
        InvertRigid(inverse, m_local);
        Matrix34Mul(m_world, *m_pivot, inverse);
    } else {
        Matrix34Mul(m_world, *m_pivot, m_local);
    }
    m_flags &= ~kObjFlag_Dirty;
    const bool moved = Truncate(m_world.translation) != oldPos;

    UpdateWorldBounds();

    if (ApeSkeleton* skeleton = GetSkeleton()) {
        skeleton->UpdateModel();
        if (m_flags & kObjFlag_0100) {
            // Bounds of the animated bone positions, refreshed every
            // g_skeletonBoundsInterval frames.
            bool refresh = true;
            if (g_skeletonBoundsInterval != 0) {
                const int counter = int(skeleton->lodCounter) - 1;
                if (counter < 0) {
                    skeleton->lodCounter = uint8_t(counter + g_skeletonBoundsInterval);
                } else {
                    skeleton->lodCounter = uint8_t(counter);
                    refresh = forceSkeletonBounds;
                }
            }
            Sphere sphere;
            if (refresh) {
                skeleton->UpdateModel();
                skeleton->UpdateModel();
                Aabb box;
                AabbFromPoint(box, skeleton->model[0].translation);
                for (uint32_t i = 1; i < skeleton->numBones; ++i)
                    AabbAddPoint(box, skeleton->model[i].translation);
                SphereFromAabb(sphere, box);
                skeleton->bounds = sphere;
            } else {
                sphere = skeleton->bounds;
            }
            SphereTransform(sphere, m_world, false);
            m_worldBounds.w = std::fmax(sphere.w, m_worldBounds.w);
            m_worldBounds.x = sphere.x;
            m_worldBounds.y = sphere.y;
            m_worldBounds.z = sphere.z;
        } else {
            // Centre the bounds on the root bone.
            const Vec3 offset = RotateVector(skeleton->bones[0].local->translation, m_world);
            Vec3 center = {m_worldBounds.x, m_worldBounds.y, m_worldBounds.z, 0.0f};
            Vec3Accumulate(center, offset);
            m_worldBounds.x = center.x;
            m_worldBounds.y = center.y;
            m_worldBounds.z = center.z;
        }
    }

    for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
        child->UpdateWorldFromParent(m_scale);

    if (m_flags & kObjFlag_HasScale)
        Matrix34ScaleAxes(m_world, m_scale);
    OnWorldTransformUpdated(moved);
    m_flags &= ~kObjFlag_Dirty;
}

// --- hierarchy -------------------------------------------------------------------------

// Visitor used by AttachChild/DetachChild to propagate the hierarchy root.
// @func 0x0057F5CC ApeObj::SetRootVisitor
static bool SetRootVisitor(ApeObj* obj, void* root)
{
    obj->SetRoot(static_cast<ApeObj*>(root));
    return true;
}

// @func 0x0057F5D8 ApeObj::ForEachInHierarchy
bool ApeObj::ForEachInHierarchy(VisitFn fn, void* arg, bool includeSelf)
{
    if (includeSelf && !fn(this, arg))
        return false;
    if (!m_firstChild)
        return true;

    constexpr int kMaxPending = 10;
    ApeObj* pending[kMaxPending];
    int top = 0;
    pending[0] = m_firstChild;
    while (top >= 0) {
        ApeObj* obj = pending[top--];
        if (!obj)
            return true;
        if (!fn(obj, arg))
            return false;
        if (obj->m_nextSibling && top < kMaxPending - 1)
            pending[++top] = obj->m_nextSibling;
        if (obj->m_firstChild && top < kMaxPending - 1)
            pending[++top] = obj->m_firstChild;
    }
    return true;
}

// @func 0x0057F794 ApeObj::AttachChild
void ApeObj::AttachChild(ApeObj* child, int16_t parentBone)
{
    if (m_refCount <= 0 || child->m_refCount <= 0 || child->m_parent)
        return;

    child->m_parent = this;
    child->m_parentBone = parentBone;

    // Push the child at the front of the child list; the list links hold
    // references.
    ApeObj* oldFirst = m_firstChild;
    if (child->m_nextSibling)
        child->m_nextSibling->Release();
    child->m_nextSibling = oldFirst;
    if (oldFirst)
        oldFirst->AddRef();
    if (m_firstChild)
        m_firstChild->Release();
    m_firstChild = child;
    child->AddRef();

    MarkDirty();

    // The new root is the topmost ancestor below any hierarchy root object.
    ApeObj* root = this;
    while (root->m_parent && root->m_parent->GetType() != kObjType_HierarchyRoot)
        root = root->m_parent;
    child->ForEachInHierarchy(&SetRootVisitor, root, true);
}

// @func 0x0057F9C0 ApeObj::DetachChild
void ApeObj::DetachChild(ApeObj* child)
{
    if (child->m_parent != this) {
        // NOTE: the console only fetches both names here (a stripped warning).
        return;
    }

    auto unlink = [child](ApeObj*& link) {
        child->AddRef();   // keeps the child alive while the links change
        ApeObj* next = child->m_nextSibling;
        if (link)
            link->Release();
        link = next;
        if (next)
            next->AddRef();
        child->m_parent = nullptr;
        child->m_parentBone = 0;
        if (child->m_nextSibling)
            child->m_nextSibling->Release();
        child->m_nextSibling = nullptr;
        child->Release();
    };

    if (m_firstChild == child) {
        unlink(m_firstChild);
    } else {
        for (ApeObj* sibling = m_firstChild; sibling; sibling = sibling->m_nextSibling) {
            if (sibling->m_nextSibling == child) {
                unlink(sibling->m_nextSibling);
                break;
            }
        }
    }
    child->ForEachInHierarchy(&SetRootVisitor, child, true);
}

// --- virtual defaults ------------------------------------------------------------------

// @func 0x0057FC40 ApeObj::OnBoundsChanged
// @func 0x007A9964 ApeObj::OnBoundsChanged (copy 2)
void ApeObj::OnBoundsChanged()
{
    if (ApeRenderNode* node = GetRenderNode())
        GetRenderNodeBounds(node)->sphere = m_worldBounds;
}

// @func 0x0057FCD4 ApeObj::SetVisible
void ApeObj::SetVisible(bool visible, bool recurse)
{
    const bool wasVisible = (m_flags & kObjFlag_Visible) != 0;
    if (visible)
        m_flags |= kObjFlag_Visible;
    else
        m_flags &= ~kObjFlag_Visible;
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
            child->SetVisible(visible, recurse);
    }
    if (wasVisible != visible)
        OnBoundsChanged();
}

// @func 0x0057FE40 ApeObj::VirtualSlot12
void ApeObj::VirtualSlot12(uint32_t value, bool recurse)
{
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
            child->VirtualSlot12(value, recurse);
    }
}

// @func 0x0057FEC8 ApeObj::VirtualSlot13
void ApeObj::VirtualSlot13(uint8_t value, bool recurse)
{
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
            child->VirtualSlot13(value, recurse);
    }
}

// @func 0x00580094 ApeObj::VirtualSlot15
void ApeObj::VirtualSlot15(int count, int arg)
{
    // NOTE: the console also fetches the object and definition names here
    // without using them (stripped debug output).
    if (m_def && count > 0)
        m_def->VirtualSlot6(0);
    for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
        child->VirtualSlot15(count, arg);
}

// --- render nodes / lists ----------------------------------------------------------------

// @func 0x0057A428 ApeObj::GetRenderNode
// @func 0x0057FF50 ApeObj::GetRenderNode (copy 2)
ApeRenderNode* ApeObj::GetRenderNode() const
{
    return GetRenderNodeOf(*this);
}

// @func 0x0057DA68 ApeObj::ReleaseRenderNodes
void ApeObj::ReleaseRenderNodes()
{
    if (m_renderNode != kNoRenderNode)
        ReleaseRenderNode(m_renderNode);
    for (ApeObj* child = m_firstChild; child; child = child->m_nextSibling)
        child->ReleaseRenderNodes();
}

} // namespace ape::world
