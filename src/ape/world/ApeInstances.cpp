// openTIT - Placed world objects: render object base and instance types.
#include "ape/world/ApeInstances.h"

#include "agl/AglRenderTarget.h"
#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeRegions.h"
#include "ape/world/ApeSkeleton.h"
#include "ape/world/ApeWorldHooks.h"

#include <cstring>

namespace ape::world {

uint32_t g_renderFrameCounter = 0;
uint32_t g_renderWriteBuffer = 0;
uint32_t g_renderReadBuffer = 0;
Vec3 g_levelInstanceOffset = {0.0f, 0.0f, 0.0f, 0.0f};
uint8_t g_defaultObjectColor[4] = {0x80, 0x80, 0x80, 0x80};

namespace {

void InvalidateLightLink(AglLightLink* link)
{
    // NOTE: the console clears bit 7 of byte 0xD4 of the renderer's light
    // link state so the light selection is redone; the state itself is owned
    // by the renderer.
    (void)link;
}

void EnsureLightLink(ApeRenderObj& obj)
{
    if (obj.m_lightGroupMask != 0 && !obj.m_lightLink && g_worldRenderHooks.createLightLink) {
        obj.m_lightLink = g_worldRenderHooks.createLightLink();
        obj.OnBoundsChanged();
    }
}

void FreeLightLink(ApeRenderObj& obj)
{
    if (obj.m_lightLink) {
        if (g_worldRenderHooks.freeLightLink)
            g_worldRenderHooks.freeLightLink(obj.m_lightLink);
        obj.m_lightLink = nullptr;
    }
}

// Leaves the dynamic list (render objects only) and detaches the children.
void ShutdownInstance(ApeObj* obj)
{
    const uint32_t type = obj->GetType();
    if (type >= kObjType_FirstInstance && type < kObjType_EndRenderable)
        static_cast<ApeRenderObj*>(obj)->RemoveFromDynamicList();
    for (ApeObj* child = obj->GetFirstChild(); child;) {
        ApeObj* next = child->GetNextSibling();
        obj->DetachChild(child);
        child = next;
    }
}

// Shared body of the instance Release() implementations.
template <class T>
void ReleaseInstance(T* obj)
{
    if (!obj->DropRef())
        return;
    ShutdownInstance(obj);
    obj->ApeBase::Release();
    DeferDestroy(obj, &ApeTInstanceList<T>::Destroy);
}

// Instance chunk: create, parse the nested record (and children), then
// either offset a root instance or attach it to its parent.
template <class T>
T* LoadInstance(iff::IffReader* reader, ApeObj* parent)
{
    T* obj = ApeTInstanceList<T>::Create();
    ApeTInstanceList<T>::RunLoadCallback(obj, 0);
    reader->ParseChunks(obj);
    ApeTInstanceList<T>::RunLoadCallback(obj, 1);
    if (!parent) {
        obj->m_local.translation.x += g_levelInstanceOffset.x;
        obj->m_local.translation.y += g_levelInstanceOffset.y;
        obj->m_local.translation.z += g_levelInstanceOffset.z;
        obj->MarkDirty();
    } else {
        parent->AttachChild(obj, -1);
    }
    return obj;
}

} // namespace

// --- instance record ------------------------------------------------------------------------

void ReadInstanceRecord(iff::IffReader* reader, ApeInstanceRecord& record)
{
    std::memset(&record, 0, sizeof(record));
    uint8_t chunkVersion = 0, version = 0;
    ApeObjInstanceData& base = record.base;
    reader->ReadU8(&chunkVersion);
    reader->ReadU8(&version);
    reader->ReadVec3(base.xBasis);
    reader->ReadVec3(base.zBasis);
    reader->ReadVec3(base.position);
    reader->ReadVec3(base.scale);
    reader->ReadU32(&base.flags);
    reader->ReadU32(&base.nameHash);
    reader->ReadU32(&base.field_38);   // animHash
    reader->ReadRaw32(base.name);
    reader->ReadU32(&base.defHash);
    reader->ReadU32(&record.parentHash);
    reader->ReadU32(&record.parentPartHash);
    reader->ReadU32(&record.pvsHash);
    reader->ReadU32(&record.lightGroup);
    reader->ReadU8(&record.sortOffset);
    reader->ReadU8(&record.fogGroup);
    reader->ReadU8(&record.renderPass);
}

// --- ApeRenderObj -----------------------------------------------------------------------------

// @func 0x00580708 ApeRenderObj::ApeRenderObj
ApeRenderObj::ApeRenderObj(uint32_t type) : ApeObj(type)
{
    m_visSlot = 0xFFFF;
    InitRenderFields();
    m_flags142 |= 0x20;
}

// @func 0x00580658 ApeRenderObj::InitRenderFields
void ApeRenderObj::InitRenderFields()
{
    m_sortOffset = 0;
    m_lightLink = nullptr;
    m_visSlot = 0xFFFF;
    m_lightGroupMask = 0;
    field_128 = -1;
    m_skeleton = nullptr;
    m_frameStamp[0] = 0;
    m_frameStamp[1] = 0;
    m_flags141 = 0;
    m_lodField = 0;
    m_flags142 = uint8_t((m_flags142 & ~0x40) | 0x80);
    m_drawMode = 0;
    std::memcpy(m_color, g_defaultObjectColor, sizeof(m_color));
    m_regionMask = 0;
    field_148 = 2;
    m_shadowParamA = 0;
    m_shadowParamB = 0;
}

// @func 0x00580978 ApeRenderObj::~ApeRenderObj
// @func 0x00580AA4 ApeRenderObj::~ApeRenderObj (deleting)
ApeRenderObj::~ApeRenderObj()
{
    FreeLightLink(*this);
    if (m_skeleton) {
        ApeSkeleton::Destroy(m_skeleton);
        m_skeleton = nullptr;
    }
}

// @func 0x00583B48 ApeRenderObj::Release
void ApeRenderObj::Release()
{
    ShutdownInstance(this);
    ApeBase::Release();
}

// @func 0x0057FF98 ApeRenderObj::RemoveFromDynamicList
void ApeRenderObj::RemoveFromDynamicList()
{
    const uint32_t type = GetType();
    if (type < kObjType_FirstInstance || type >= kObjType_EndRenderable || !IsDynamic())
        return;
    if (type != kObjType_GeomInstance && type != kObjType_EmitterInstance)
        return;
    ApeTArray<ApeObj*>& list = GetDynamicObjectList();
    const int index = list.FindLast(this);
    if (index < 0)
        return;
    list.RemoveAtSwap(size_t(index));
    if (type == kObjType_EmitterInstance)
        static_cast<ApeEmitterDef*>(m_def)->m_isDynamic = 0;
}

// @func 0x00580AF0 ApeRenderObj::SetLightGroupMask
void ApeRenderObj::SetLightGroupMask(uint32_t mask)
{
    m_lightGroupMask = mask;
    if (m_lightLink)
        InvalidateLightLink(m_lightLink);
}

// @func 0x00582C10 ApeRenderObj::TrySetLightGroupMask
bool ApeRenderObj::TrySetLightGroupMask(uint32_t mask)
{
    if (GetType() != kObjType_GeomInstance)
        return true;
    if (m_lightGroupMask == 0)
        return false;
    SetLightGroupMask(mask);
    return true;
}

// @func 0x00580B18 ApeRenderObj::InitFromRecord
void ApeRenderObj::InitFromRecord(const ApeInstanceRecord& record)
{
    InitFromInstanceData(record.base);
    const uint32_t flags = record.base.flags;

    m_sortOffset = record.sortOffset;
    m_flags141 = uint8_t((m_flags141 & ~0x03) | (record.fogGroup & 1) | (record.renderPass != 0 ? 0x02 : 0));
    m_flags142 = uint8_t((m_flags142 & 0x7F) | ((flags & 0x10) ? 0x80 : 0));

    const uint32_t type = GetType();
    if (type == kObjType_EmitterInstance || type == kObjType_GeomInstance) {
        m_flags142 &= 0xE1;
        switch (flags & 0x600) {
        case 0x600: m_drawMode = kDrawMode_Fixed; break;
        case 0x400: m_drawMode = kDrawMode_Named; break;
        case 0x200: m_drawMode = kDrawMode_1; m_xformFlags |= kXform_20; break;
        default:    m_drawMode = kDrawMode_2; m_xformFlags |= kXform_20; break;
        }
    }
    if (record.pvsHash != 0) {
        m_regionMask = record.pvsHash;
        m_xformFlags |= kXform_80;
    }

    if (!(flags & 0x100) && !(flags & 0x1000)) {
        if (m_def)
            SetLightGroupMask(m_def->field_68);
    } else {
        SetLightGroupMask(record.lightGroup);
    }
    if (flags & 0x1000)
        m_xformFlags |= kXform_02;
    if (flags & 0x2000)
        m_xformFlags |= kXform_01;
    EnsureLightLink(*this);
}

// @func 0x00581B34 ApeRenderObj::CopyRenderFrom
void ApeRenderObj::CopyRenderFrom(const ApeRenderObj& other, uint32_t skipMask)
{
    CopyFrom(other, skipMask);
    if (!(skipMask & 0x40))
        m_lightGroupMask = other.m_lightGroupMask;
    field_128 = other.field_128;
    std::memcpy(m_color, other.m_color, sizeof(m_color));
    m_sortOffset = other.m_sortOffset;
    m_lodField = other.m_lodField;
    m_flags141 = uint8_t((m_flags141 & ~0x03) | (other.m_flags141 & 0x03));
    m_flags142 = uint8_t((other.m_flags142 & 0x80) | (m_flags142 & 0x61));
    m_drawMode = other.m_drawMode;
    if (m_drawMode == kDrawMode_1 || m_drawMode == kDrawMode_2)
        m_xformFlags |= kXform_20;

    if (other.m_skeleton) {
        if (!m_skeleton)
            m_skeleton = ApeSkeleton::Create(other.m_skeleton->def);
        m_skeleton->CopyPose(*other.m_skeleton);
    }

    if (other.m_lightGroupMask == 0)
        FreeLightLink(*this);
    else
        EnsureLightLink(*this);

    if (!(skipMask & kCopy_SkipTransform))
        m_regionMask = other.m_regionMask;
}

// @func 0x00582C6C ApeRenderObj::UpdateRegionMask
void ApeRenderObj::UpdateRegionMask(bool resolvePinned)
{
    if (!g_regionSet)
        return;
    ApeRegionSet& set = *g_regionSet;
    uint32_t mask;
    if (!(m_xformFlags & kXform_80)) {
        if (resolvePinned) {
            // TODO(0x00577338): the console asks the scene module for the
            // region of the object here; the bounds test is used instead.
        }
        Aabb box;
        AabbTransform(box, m_def->m_bounds, m_world);
        mask = ComputeRegionMask(set, box);
    } else {
        if (!resolvePinned)
            return;
        const ApeRegion* region = FindRegionByNameHash(set, m_regionMask);
        mask = region ? 1u << (region->id & 0x3F) : 0;
    }
    if (m_regionMask != mask) {
        m_regionMask = mask;
        OnBoundsChanged();
    }
}

// @func 0x00582ED0 ApeRenderObj::OnWorldTransformUpdated
void ApeRenderObj::OnWorldTransformUpdated(bool moved)
{
    OnBoundsChanged();
    if (moved)
        UpdateRegionMask(false);
}

// @func 0x00583C04 ApeRenderObj::SetShadowParams
void ApeRenderObj::SetShadowParams(uint32_t unused, uint32_t a, uint32_t b, bool recurse)
{
    if (GetType() == kObjType_GeomInstance) {
        m_shadowParamA = a;
        m_shadowParamB = b;
    }
    if (!recurse)
        return;
    for (ApeObj* child = m_firstChild; child; child = child->GetNextSibling()) {
        const uint32_t type = child->GetType();
        if (type >= kObjType_FirstInstance && type < kObjType_EndRenderable)
            static_cast<ApeRenderObj*>(child)->SetShadowParams(unused, a, b, recurse);
    }
}

// @func 0x00583CC8 ApeRenderObj::SetVisible
void ApeRenderObj::SetVisible(bool visible, bool recurse)
{
    bool changed = false;
    if (m_drawMode != kDrawMode_Fixed) {
        const bool wasVisible = IsVisible();
        if (visible)
            m_flags |= kObjFlag_Visible;
        else
            m_flags &= ~kObjFlag_Visible;
        changed = wasVisible != visible;
    }
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->GetNextSibling())
            child->SetVisible(visible, recurse);
    }
    if (changed)
        OnBoundsChanged();
}

// @func 0x00583DC8 ApeRenderObj::VirtualSlot12
void ApeRenderObj::VirtualSlot12(uint32_t value, bool recurse)
{
    m_sortOffset = uint8_t(value);
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->GetNextSibling())
            child->VirtualSlot12(value, recurse);
    }
}

// @func 0x00583E54 ApeRenderObj::SetLodField
void ApeRenderObj::SetLodField(uint32_t value)
{
    if (m_lodField == (value & 0x3F))
        return;
    m_lodField = uint8_t(value & 0x3F);
    OnBoundsChanged();
}

// @func 0x00583EB8 ApeRenderObj::IsDrawnThisFrame
bool ApeRenderObj::IsDrawnThisFrame() const
{
    return g_renderFrameCounter <= m_frameStamp[g_renderReadBuffer & 1];
}

// @func 0x00583EF0 ApeRenderObj::VirtualSlot13
void ApeRenderObj::VirtualSlot13(uint8_t value, bool recurse)
{
    if (m_byteA8 != value) {
        m_byteA8 = value;
        OnBoundsChanged();
    }
    if (recurse) {
        for (ApeObj* child = m_firstChild; child; child = child->GetNextSibling())
            child->VirtualSlot13(value, recurse);
    }
}

// --- instance types ---------------------------------------------------------------------------

// @func 0x005A02B0 ApeGeomInstance::Release
void ApeGeomInstance::Release()
{
    ReleaseInstance(this);
}

// TODO(0x005A22FC ApeLightInstance::Release): the console version also
// gives back the instance's slot in the active light list (0x01194EC0,
// index at offset 0x17E); that list is not ported.
void ApeLightInstance::Release()
{
    ReleaseInstance(this);
}

// @func 0x005A3F18 ApeEmitterInstance::Release
void ApeEmitterInstance::Release()
{
    ReleaseInstance(this);
}

// @func 0x005A58C8 ApeCameraInstance::Release
void ApeCameraInstance::Release()
{
    ReleaseInstance(this);
}

// --- creation and cloning ---------------------------------------------------------------------

namespace {

// Names an instance after its definition.
void NameAfterDef(ApeObj& obj, ApeDef* def)
{
    obj.SetName(def->GetName(), true);
}

// Clones the children of `source` and attaches the copies to `copy`.
void CloneChildren(ApeObj& copy, const ApeObj& source, uint32_t flags)
{
    if (!(flags & kClone_Children))
        return;
    for (ApeObj* child = source.GetFirstChild(); child; child = child->GetNextSibling()) {
        ApeObj* childCopy = child->Clone(flags);
        if (!childCopy)
            continue;
        copy.AttachChild(childCopy, -1);
        childCopy->Release();
    }
}

void FinishRootClone(ApeObj& copy, const ApeObj& source, uint32_t flags)
{
    if (!source.GetParent() && !(flags & kClone_NoProcAnim))
        RegisterProcAnims(&copy);
}

} // namespace

// @func 0x005A05E8 CreateGeomInstance
ApeGeomInstance* CreateGeomInstance(ApeDef* def, bool cloneDef)
{
    ApeGeomInstance* obj = ApeTInstanceList<ApeGeomInstance>::Create();
    bool ownsDef = false;
    if (def && cloneDef) {
        def = static_cast<ApeGeomDef*>(def)->Clone();
        ownsDef = true;
    }
    obj->SetDef(def);
    NameAfterDef(*obj, def);
    obj->m_lightGroupMask = def->field_68;
    if (obj->m_lightLink)
        InvalidateLightLink(obj->m_lightLink);
    EnsureLightLink(*obj);
    // The instance holds its own reference to the copied definition.
    if (ownsDef)
        def->Release();
    return obj;
}

// @func 0x005A59AC CreateCameraInstance
ApeCameraInstance* CreateCameraInstance(ApeDef* def)
{
    ApeCameraInstance* obj = ApeTInstanceList<ApeCameraInstance>::Create();
    obj->SetDef(def);
    return obj;
}

// @func 0x005A68EC CreatePointInstance
ApePointInstance* CreatePointInstance(ApeDef* def)
{
    ApePointInstance* obj = ApeTInstanceList<ApePointInstance>::Create();
    obj->SetDef(def);
    NameAfterDef(*obj, def);
    return obj;
}

// TODO(0x005A0394 RegisterProcAnims) walks the hierarchy with the ProcAnim
// candidate callback (0x005A9914).
void RegisterProcAnims(ApeObj* /*root*/)
{
}

// @func 0x005A089C ApeGeomInstance::Clone
ApeObj* ApeGeomInstance::Clone(uint32_t flags)
{
    ApeGeomInstance* copy;
    if (flags & kClone_Def) {
        ApeGeomDef* defCopy = static_cast<ApeGeomDef*>(m_def)->Clone();
        copy = CreateGeomInstance(defCopy, false);
        defCopy->Release();
        copy->CopyRenderFrom(*this, kCopy_SkipDef);
    } else {
        copy = CreateGeomInstance(m_def, false);
        copy->CopyRenderFrom(*this, 0);
    }
    CloneChildren(*copy, *this, flags);
    FinishRootClone(*copy, *this, flags);
    return copy;
}

// @func 0x005A5B28 ApeCameraInstance::Clone
ApeObj* ApeCameraInstance::Clone(uint32_t flags)
{
    ApeCameraInstance* copy = CreateCameraInstance(m_def);
    copy->CopyRenderFrom(*this, 0);
    CloneChildren(*copy, *this, flags);
    return copy;
}

// @func 0x005A6B28 ApePointInstance::Clone
ApeObj* ApePointInstance::Clone(uint32_t flags)
{
    ApePointInstance* copy = CreatePointInstance(m_def);
    copy->CopyFrom(*this, 0);
    CloneChildren(*copy, *this, flags);
    return copy;
}

// @func 0x005A14A0 ApeGeomInstance::SetBoundingRadius
void ApeGeomInstance::SetBoundingRadius(float radius)
{
    ApeObj::SetBoundingRadius(radius);
    const ApeGeomDef* def = static_cast<const ApeGeomDef*>(m_def);
    const float drawDist = def->m_lods[def->m_lodCount - 1].switchDist + def->GetSphere().w;
    m_drawDistSq = drawDist * drawDist;
    OnBoundsChanged();
}

namespace {
// TODO(0x00584160 DumpSkeleton) debug dump of a posed skeleton (its output
// was compiled out).
void DumpSkeleton(ApeSkeleton* /*skeleton*/, int /*depth*/)
{
}
} // namespace

// @func 0x005A0BCC ApeGeomInstance::VirtualSlot15
void ApeGeomInstance::VirtualSlot15(int count, int arg)
{
    // NOTE: at depth 0 the console only fetches the object and definition
    // names (stripped debug output).
    if (count < 1)
        return;
    ApeObj::VirtualSlot15(count - 1, arg | 1);
    if (count - 1 > 0 && GetSkeleton())
        DumpSkeleton(GetSkeleton(), count - 2);
}

// @func 0x005A2FDC ApeLightInstance::SetDef
void ApeLightInstance::SetDef(ApeDef* def)
{
    ApeObj::SetDef(def);
    if (!def) {
        m_lightType = 3;
        m_color = kWorldUnitScale;
        m_lightModel = 1;
        m_intensity = 1.0f;
        m_priority = 0x7F;
        m_atten[0] = 0.0f;
        m_atten[1] = 1.0f;
        return;
    }
    const ApeLightDef* light = static_cast<const ApeLightDef*>(def);
    m_lightType = int32_t(light->m_lightType);
    m_color = light->m_color;
    m_lightModel = light->m_lightModel;
    m_intensity = light->m_intensity * light->m_intensity;
    m_priority = light->m_priority;
    for (int i = 0; i < 3; ++i)
        m_atten[i] = light->m_atten[i];
}

// @func 0x005A30CC ApeLightInstance::VirtualSlot15
void ApeLightInstance::VirtualSlot15(int count, int arg)
{
    // NOTE: at depth 0 the console only fetches the object name (stripped
    // debug output).
    if (count < 1)
        return;
    ApeObj::VirtualSlot15(count - 1, arg | 1);
    if (count - 1 > 0 && GetSkeleton())
        DumpSkeleton(GetSkeleton(), count - 2);
}

// @func 0x005A5C28 ApeCameraInstance::SetDef
void ApeCameraInstance::SetDef(ApeDef* def)
{
    if (def == m_def)
        return;
    ApeObj::SetDef(def);
    if (!m_def)
        return;
    const ApeCameraDef* camera = static_cast<const ApeCameraDef*>(m_def);
    m_fov = camera->m_fov;
    m_local = camera->m_transform;
    MarkDirty();
}

// @func 0x005A5EA0 ApeCameraInstance::ApplyToView
void ApeCameraInstance::ApplyToView(agl::AglRenderTarget& view)
{
    UpdateWorld(false, false);
    // The world transform is orthonormal: the inverse is the transposed
    // rotation and the translation taken through it.
    const Matrix34& world = GetWorld();
    Matrix34 inverse{};
    inverse.xAxis = { world.xAxis.x, world.yAxis.x, world.zAxis.x, 0.0f };
    inverse.yAxis = { world.xAxis.y, world.yAxis.y, world.zAxis.y, 0.0f };
    inverse.zAxis = { world.xAxis.z, world.yAxis.z, world.zAxis.z, 0.0f };
    const Vec3& t = world.translation;
    inverse.translation = { -(t.x * inverse.xAxis.x + t.y * inverse.yAxis.x + t.z * inverse.zAxis.x),
                            -(t.x * inverse.xAxis.y + t.y * inverse.yAxis.y + t.z * inverse.zAxis.y),
                            -(t.x * inverse.xAxis.z + t.y * inverse.yAxis.z + t.z * inverse.zAxis.z), 0.0f };
    view.SetViewMatrix(inverse);
    view.SetFov(m_fov);
}

// @func 0x005A5DC0 ApeCameraInstance::VirtualSlot15
void ApeCameraInstance::VirtualSlot15(int count, int arg)
{
    ApeObj::VirtualSlot15(count, arg | 1);
    if (count > 0 && GetSkeleton())
        DumpSkeleton(GetSkeleton(), count - 1);
}

// @func 0x005A6808 ApePointInstance::Release
void ApePointInstance::Release()
{
    ReleaseInstance(this);
}

// @func 0x005A7A54 ApeSplineInstance::Release
void ApeSplineInstance::Release()
{
    ReleaseInstance(this);
}

// A geometry definition with a skeleton gives the instance a posed copy;
// the draw distance covers the last LOD.
// @func 0x005A0A28 ApeGeomInstance::SetDef
void ApeGeomInstance::SetDef(ApeDef* def)
{
    if (def == m_def)
        return;
    ApeObj::SetDef(def);
    ApeGeomDef* geomDef = static_cast<ApeGeomDef*>(m_def);
    if (!geomDef) {
        m_drawDistSq = 0.0f;
        m_flags142 &= ~0x40;
        OnBoundsChanged();
        return;
    }
    if (geomDef->m_skeletonDef && !GetSkeleton()) {
        ApeSkeleton* skeleton = ApeSkeleton::Create(geomDef->m_skeletonDef);
        if (m_skeleton != skeleton) {
            if (m_skeleton)
                ApeSkeleton::Destroy(m_skeleton);
            m_skeleton = skeleton;
            OnBoundsChanged();
        }
    }
    // NOTE: bit 6 mirrors bit 7 of byte 0x36 of the first LOD's mesh (a
    // renderer flag, probably "skinned"); meshes are opaque here.
    m_flags142 &= ~0x40;
    const float reach = geomDef->m_lods[geomDef->m_lodCount > 0 ? geomDef->m_lodCount - 1 : 0].switchDist +
                        geomDef->m_sphere.w;
    m_drawDistSq = reach * reach;
    OnBoundsChanged();
}

// --- spline instance queries ---------------------------------------------------------------

namespace {

Vec3 ToWorldPoint(const Vec3& p, const Matrix34& m)
{
    return {p.x * m.xAxis.x + p.y * m.yAxis.x + p.z * m.zAxis.x + m.translation.x,
            p.x * m.xAxis.y + p.y * m.yAxis.y + p.z * m.zAxis.y + m.translation.y,
            p.x * m.xAxis.z + p.y * m.yAxis.z + p.z * m.zAxis.z + m.translation.z, p.pad};
}

Vec3 ToWorldVector(const Vec3& v, const Matrix34& m)
{
    return {v.x * m.xAxis.x + v.y * m.yAxis.x + v.z * m.zAxis.x,
            v.x * m.xAxis.y + v.y * m.yAxis.y + v.z * m.zAxis.y,
            v.x * m.xAxis.z + v.y * m.yAxis.z + v.z * m.zAxis.z, v.pad};
}

} // namespace

ApeSplineDef* ApeSplineInstance::GetSplineDef() const
{
    return static_cast<ApeSplineDef*>(m_def);
}

// @func 0x005A81CC ApeSplineInstance::EvaluatePosition
int ApeSplineInstance::EvaluatePosition(float t, Vec3& outPos, uint32_t startSegment, bool reverse)
{
    const int segment = GetSplineDef()->m_spline.EvaluatePosition(t, outPos, startSegment, reverse);
    outPos = ToWorldPoint(outPos, m_world);
    return segment;
}

// @func 0x00592748 ApeSplineInstance::EvaluatePosition
int ApeSplineInstance::EvaluatePosition(float t, Vec3& outPos, bool reverse)
{
    const int segment = GetSplineDef()->m_spline.EvaluatePosition(t, outPos, reverse);
    outPos = ToWorldPoint(outPos, m_world);
    return segment;
}

// @func 0x005A8034 ApeSplineInstance::EvaluateTangent
int ApeSplineInstance::EvaluateTangent(float t, Vec3& outTangent, bool reverse)
{
    const int segment = GetSplineDef()->m_spline.EvaluateTangent(t, outTangent, reverse);
    outTangent = ToWorldVector(outTangent, m_world);
    return segment;
}

// @func 0x005A8284 ApeSplineInstance::Evaluate
int ApeSplineInstance::Evaluate(float t, Vec3& outPos, Vec3& outTangent, uint32_t startSegment, bool reverse)
{
    const int segment = GetSplineDef()->m_spline.Evaluate(t, outPos, outTangent, startSegment, reverse);
    outPos = ToWorldPoint(outPos, m_world);
    outTangent = ToWorldVector(outTangent, m_world);
    return segment;
}

// @func 0x005A80D8 ApeSplineInstance::Evaluate
int ApeSplineInstance::Evaluate(float t, Vec3& outPos, Vec3& outTangent, bool reverse)
{
    const int segment = GetSplineDef()->m_spline.Evaluate(t, outPos, outTangent, reverse);
    outPos = ToWorldPoint(outPos, m_world);
    outTangent = ToWorldVector(outTangent, m_world);
    return segment;
}

// @func 0x005927FC ApeSplineInstance::FindClosestParam
float ApeSplineInstance::FindClosestParam(const Vec3& worldPoint, uint32_t* outSegment, float* outDistance) const
{
    Vec3 local;
    Matrix34InvTransformPoint(local, worldPoint, m_world);
    return GetSplineDef()->m_spline.FindClosestParam(local, outSegment, outDistance);
}

// --- chunk handlers --------------------------------------------------------------------------

// Geometry instances are only registered by name in the "named" draw mode.
// @func 0x005A1614 LoadGeomInstanceChunk
int LoadGeomInstanceChunk(iff::IffReader* reader, void* arg)
{
    ApeGeomInstance* obj = LoadInstance<ApeGeomInstance>(reader, static_cast<ApeObj*>(arg));
    obj->m_flags142 &= ~0x20;
    if (obj->GetDrawMode() == kDrawMode_Named)
        ApeTInstanceList<ApeGeomInstance>::Register(obj);
    return 1;
}

// TODO(0x005A358C LoadLightInstanceChunk): same as the other instance types
// once the light instance construction (0x005A2BA8 family) is ported.
int LoadLightInstanceChunk(iff::IffReader* reader, void* arg)
{
    ApeLightInstance* obj = LoadInstance<ApeLightInstance>(reader, static_cast<ApeObj*>(arg));
    ApeTInstanceList<ApeLightInstance>::Register(obj);
    return 1;
}

// TODO(0x005A4918 LoadEmitterInstanceChunk): emitter instances also set up
// their particle system after loading; not ported.
int LoadEmitterInstanceChunk(iff::IffReader* reader, void* arg)
{
    ApeEmitterInstance* obj = LoadInstance<ApeEmitterInstance>(reader, static_cast<ApeObj*>(arg));
    ApeTInstanceList<ApeEmitterInstance>::Register(obj);
    return 1;
}

// TODO(0x005A600C LoadCameraInstanceChunk): not compared in detail with the
// console version yet.
int LoadCameraInstanceChunk(iff::IffReader* reader, void* arg)
{
    ApeCameraInstance* obj = LoadInstance<ApeCameraInstance>(reader, static_cast<ApeObj*>(arg));
    ApeTInstanceList<ApeCameraInstance>::Register(obj);
    return 1;
}

// @func 0x005A6C48 LoadPointInstanceChunk
int LoadPointInstanceChunk(iff::IffReader* reader, void* arg)
{
    ApePointInstance* obj = LoadInstance<ApePointInstance>(reader, static_cast<ApeObj*>(arg));
    ApeTInstanceList<ApePointInstance>::Register(obj);
    return 1;
}

// @func 0x005A83A8 LoadSplineInstanceChunk
int LoadSplineInstanceChunk(iff::IffReader* reader, void* arg)
{
    ApeSplineInstance* obj = LoadInstance<ApeSplineInstance>(reader, static_cast<ApeObj*>(arg));
    ApeTInstanceList<ApeSplineInstance>::Register(obj);
    return 1;
}

// Geometry instances without a definition get a shared placeholder
// definition with the default mesh as its only LOD.
// @func 0x005A1A08 LoadGeomInstanceRecordChunk
int LoadGeomInstanceRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeGeomInstance* obj = static_cast<ApeGeomInstance*>(arg);
    ApeInstanceRecord record;
    ReadInstanceRecord(reader, record);
    obj->InitFromRecord(record);
    if (!obj->GetDef()) {
        static ApeGeomDef* s_placeholder = nullptr;   // 0x00F889FC
        if (!s_placeholder) {
            s_placeholder = ApeTDefList<ApeGeomDef>::Create();
            ApeGeomLod& lod = s_placeholder->m_lods[s_placeholder->m_lodCount++];
            lod.mesh = g_worldRenderHooks.getDefaultMesh ? g_worldRenderHooks.getDefaultMesh() : nullptr;
            lod.switchDist = 50.0f;
        }
        obj->SetDef(s_placeholder);
    }
    return 1;
}

// Light and emitter instances.
// @func 0x005A39A8 LoadRenderInstanceRecordChunk
// @func 0x005A4CE4 LoadRenderInstanceRecordChunk (copy 2)
int LoadRenderInstanceRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeInstanceRecord record;
    ReadInstanceRecord(reader, record);
    static_cast<ApeRenderObj*>(arg)->InitFromRecord(record);
    return 1;
}

// @func 0x005A63D4 LoadCameraInstanceRecordChunk
int LoadCameraInstanceRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeRenderObj* obj = static_cast<ApeRenderObj*>(arg);
    ApeInstanceRecord record;
    ReadInstanceRecord(reader, record);
    obj->InitFromRecord(record);
    if (!obj->GetDef())
        obj->SetDef(nullptr);
    return 1;
}

// @func 0x005A7070 LoadPointInstanceRecordChunk
int LoadPointInstanceRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeObj* obj = static_cast<ApeObj*>(arg);
    ApeInstanceRecord record;
    ReadInstanceRecord(reader, record);
    obj->InitFromInstanceData(record.base);
    if (!obj->GetDef())
        obj->SetDef(ApePointDef::GetDefault());
    return 1;
}

// @func 0x005A8774 LoadSplineInstanceRecordChunk
int LoadSplineInstanceRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeObj* obj = static_cast<ApeObj*>(arg);
    ApeInstanceRecord record;
    ReadInstanceRecord(reader, record);
    obj->InitFromInstanceData(record.base);
    if (!obj->GetDef())
        obj->SetDef(ApeSplineDef::GetDefault());
    return 1;
}

// @func 0x005A9420 ParseGeomDefChunks
int ParseGeomDefChunks(iff::IffReader* reader, void* arg)
{
    ApeBase* target = static_cast<ApeBase*>(arg);
    const bool isGeomDef = target->GetType() == kObjType_GeomDef;
    if (isGeomDef)
        reader->ParseChunks(arg);
    return isGeomDef;
}

// @func 0x004A22A4 FindChildSpline
ApeSplineInstance* FindChildSpline(const ApeObj& object)
{
    for (ApeObj* child = object.GetFirstChild(); child; child = child->GetNextSibling())
        if (child->GetType() == kObjType_SplineInstance)
            return static_cast<ApeSplineInstance*>(child);
    return nullptr;
}

} // namespace ape::world
