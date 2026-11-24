// openTIT - The world definition types and their level chunk handlers.
#include "ape/world/ApeDefTypes.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeBounds.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"
#include "ape/world/ApeWorldHooks.h"

#include <cmath>
#include <cstring>

namespace ape::world {

ApeWorldRenderHooks g_worldRenderHooks;
Vec3 g_lodCameraPosition = {0.0f, 0.0f, 0.0f, 0.0f};
float g_defaultMinNearFadeAlpha = 0.0f;
float g_defaultLightFadeDist = 0.0f;
float g_defaultLightFadeRange = 0.0f;

namespace {

constexpr float kEpsilon = 1e-5f;
constexpr float kAlphaStep = 1.0f / 255.0f;

float Clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

Vec3 ToVec3(const float v[3])
{
    return {v[0], v[1], v[2], 0.0f};
}

void ReleaseMesh(AglMesh*& mesh)
{
    if (mesh && g_worldRenderHooks.releaseMesh)
        g_worldRenderHooks.releaseMesh(mesh);
    mesh = nullptr;
}

// Shared tail of the definition chunk handlers: load callbacks around the
// nested chunks, then registration by name.
template <class T>
void ParseAndRegister(iff::IffReader* reader, T* def)
{
    ApeTDefList<T>::RunLoadCallback(def, 0);
    reader->ParseChunks(def);
    ApeTDefList<T>::RunLoadCallback(def, 1);
    ApeTDefList<T>::Register(def);
}

} // namespace

// --- ApeGeomDef ---------------------------------------------------------------------------

ApeGeomDef::ApeGeomDef() : ApeDef(kObjType_GeomDef) {}

// @func 0x00585678 ApeGeomDef::~ApeGeomDef
// @func 0x00585848 ApeGeomDef::~ApeGeomDef (deleting)
ApeGeomDef::~ApeGeomDef()
{
    for (uint32_t i = 0; i < m_lodCount; ++i)
        ReleaseMesh(m_lods[i].mesh);
    // A shadow mesh stored past the visible LODs is owned as well.
    if (m_shadowLod != 0 && m_shadowLod != m_lodCount - 1u)
        ReleaseMesh(m_lods[m_shadowLod].mesh);
    if (m_skeletonDef) {
        ApeSkeletonDef* skeleton = m_skeletonDef;
        m_skeletonDef = nullptr;
        skeleton->Release();
    }
}

// @func 0x00585890 ApeGeomDef::Release
void ApeGeomDef::Release()
{
    if (DropRef())
        DeferDestroy(this, &ApeTDefList<ApeGeomDef>::DestroyErased);
}

// @func 0x00585480 ApeGeomDef::SetFromRecord
void ApeGeomDef::SetFromRecord(ApeGeomDefRecord& record)
{
    m_bounds.min = ToVec3(record.bboxMin);
    m_bounds.max = ToVec3(record.bboxMax);
    SphereFromAabb(m_sphere, m_bounds);
    StrCopy(m_name, record.name);
    m_nameHash = record.nameHash;
    m_animHash = record.animHash;
    field_68 = record.lightGroup;

    if (record.nearZFadeRange == 0.0f)
        record.nearZFadeRange = 1.0f;
    m_nearZFadeRange = Clamp(record.nearZFadeRange, kEpsilon, 100.0f);
    m_nearZFadeDist = record.nearZFadeDist;
    m_shadowType = record.shadowType;
    m_shadowExtrusionLength = record.shadowExtrusionLength;
    m_shadowLod = record.shadowLod;
    m_shadowZCullDist = std::fabs(record.shadowZCullDist) < kEpsilon ? 50.0f : record.shadowZCullDist;

    const float minAlpha = record.minNearFadeAlpha >= 0.0f ? record.minNearFadeAlpha : g_defaultMinNearFadeAlpha;
    m_minNearFadeAlpha = Clamp01(minAlpha);
}

// @func 0x00585B98 ApeGeomDef::SelectLod
ApeGeomLod* ApeGeomDef::SelectLod(ApeDrawParams& params)
{
    const float dx = params.position.x - g_lodCameraPosition.x;
    const float dy = params.position.y - g_lodCameraPosition.y;
    const float dz = params.position.z - g_lodCameraPosition.z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz) - m_sphere.w;

    for (uint32_t i = 0; i < m_lodCount; ++i) {
        if (!(m_lods[i].switchDist - distance > 0.0f))
            continue;

        if (m_nearZFadeRange > kEpsilon) {
            // Fade in close to the camera.
            if (m_nearZFadeDist > 0.0f && distance < m_nearZFadeDist) {
                const float fade = 1.0f - (m_nearZFadeDist - distance) / m_nearZFadeRange;
                const float alpha = std::fmax(m_minNearFadeAlpha, std::fmin(fade, 1.0f));
                if (alpha <= kAlphaStep)
                    return nullptr;
                params.alpha = uint8_t(int(float(params.alpha) * alpha));
            }
            // Fade out towards the end of the last LOD.
            const float farDist = m_lods[m_lodCount - 1].switchDist;
            if (farDist - m_nearZFadeRange < distance) {
                const float fade = (farDist - distance) / m_nearZFadeRange;
                if (fade <= kAlphaStep)
                    return nullptr;
                params.alpha = uint8_t(int(float(params.alpha) * Clamp01(fade)));
            }
        }
        return params.alpha != 0 ? &m_lods[i] : nullptr;
    }
    return nullptr;
}

// @func 0x00585DEC ApeGeomDef::Draw
int ApeGeomDef::VirtualSlot5(void* drawParams)
{
    ApeGeomLod* lod = SelectLod(*static_cast<ApeDrawParams*>(drawParams));
    if (lod && g_worldRenderHooks.drawMesh)
        g_worldRenderHooks.drawMesh(lod->mesh, drawParams);
    return lod != nullptr;
}

// @func 0x00585E3C ApeGeomDef::VirtualSlot6
void ApeGeomDef::VirtualSlot6(int value)
{
    if (value < 2)
        return;
    for (uint32_t i = 0; i < m_lodCount; ++i)
        if (g_worldRenderHooks.meshSlot6)
            g_worldRenderHooks.meshSlot6(m_lods[i].mesh, value - 1);
}

// @func 0x005858D4 ApeGeomDef::Clone
ApeGeomDef* ApeGeomDef::Clone() const
{
    ApeGeomDef* copy = ApeTDefList<ApeGeomDef>::Create();
    copy->ApeDef::CopyFrom(*this);

    auto cloneLod = [copy, this](uint32_t i) {
        copy->m_lods[i].mesh = g_worldRenderHooks.cloneMesh ? g_worldRenderHooks.cloneMesh(m_lods[i].mesh) : nullptr;
        copy->m_lods[i].switchDist = m_lods[i].switchDist;
    };
    copy->m_lodCount = m_lodCount;
    for (uint32_t i = 0; i < m_lodCount; ++i)
        cloneLod(i);

    copy->m_nearZFadeRange = m_nearZFadeRange;
    copy->m_nearZFadeDist = m_nearZFadeDist;
    if (copy->m_skeletonDef != m_skeletonDef) {
        ApeSkeletonDef* old = copy->m_skeletonDef;
        copy->m_skeletonDef = m_skeletonDef;
        if (m_skeletonDef)
            ++m_skeletonDef->refCount;
        if (old)
            old->Release();
    }
    copy->m_shadowType = m_shadowType;
    copy->m_shadowExtrusionLength = m_shadowExtrusionLength;
    copy->m_shadowLod = m_shadowLod;
    copy->m_shadowZCullDist = m_shadowZCullDist;
    copy->m_minNearFadeAlpha = m_minNearFadeAlpha;
    if (m_shadowLod != 0 && m_shadowLod != m_lodCount - 1u)
        cloneLod(m_shadowLod);
    return copy;
}

// Geometry definition chunk: the nested chunks fill in the record, the
// LODs and the skeleton. Definitions without any LOD are dropped.
// @func 0x0059B788 LoadGeomDefChunk
int LoadGeomDefChunk(iff::IffReader* reader, void* arg)
{
    ApeGeomDef* def = ApeTDefList<ApeGeomDef>::Create();
    ApeTDefList<ApeGeomDef>::RunLoadCallback(def, 0);
    reader->ParseChunks(def);

    if (def->m_shadowType != 0) {
        for (uint32_t i = 0; i < def->m_lodCount; ++i)
            if (g_worldRenderHooks.enableMeshShadows)
                g_worldRenderHooks.enableMeshShadows(def->m_lods[i].mesh);
        // A dedicated shadow mesh is stored as the last LOD.
        if (def->m_lodCount > 1 && def->m_shadowLod == def->m_lodCount - 1u)
            --def->m_lodCount;
        if (def->m_shadowLod == 0 && def->m_lodCount != 0)
            def->m_shadowLod = uint8_t(def->m_lodCount - 1);
    }
    ApeTDefList<ApeGeomDef>::RunLoadCallback(def, 1);

    if (def->m_lodCount == 0)
        def->Release();
    else
        ApeTDefList<ApeGeomDef>::Register(def);
    return 1;
}

// @func 0x0059B9CC LoadGeomDefRecordChunk
int LoadGeomDefRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeGeomDefRecord record;
    std::memset(&record, 0, sizeof(record));
    uint8_t version = 0;
    reader->ReadU8(&version);
    reader->ReadVec3(record.bboxMin);
    reader->ReadVec3(record.bboxMax);
    reader->ReadU32(&record.nameHash);
    reader->ReadU32(&record.animHash);
    reader->ReadU32(&record.lightGroup);
    reader->ReadRaw32(record.name);
    reader->ReadU32(&record.collisionHash);
    reader->ReadFloat(&record.nearZFadeRange);
    reader->ReadFloat(&record.nearZFadeDist);
    reader->ReadU8(&record.shadowType);
    reader->ReadU8(&record.shadowExtrusionLength);
    reader->ReadU8(&record.shadowLod);
    reader->ReadFloat(&record.shadowZCullDist);
    reader->ReadFloat(&record.minNearFadeAlpha);
    static_cast<ApeGeomDef*>(arg)->SetFromRecord(record);
    return 1;
}

// One LOD: the mesh (by hash) and its switch distance.
// @func 0x0059BC0C LoadGeomDefLodChunk
int LoadGeomDefLodChunk(iff::IffReader* reader, void* arg)
{
    ApeGeomDef* def = static_cast<ApeGeomDef*>(arg);
    uint8_t version = 0;
    uint32_t meshHash = 0;
    float switchDist = 0.0f;
    reader->ReadU8(&version);
    reader->ReadU32(&meshHash);
    reader->ReadFloat(&switchDist);

    AglMesh* mesh = g_worldRenderHooks.findMesh ? g_worldRenderHooks.findMesh(meshHash) : nullptr;
    if (mesh) {
        if (g_worldRenderHooks.setMeshOwnerName)
            g_worldRenderHooks.setMeshOwnerName(mesh, def->GetName());
    } else if (g_worldRenderHooks.getDefaultMesh) {
        mesh = g_worldRenderHooks.getDefaultMesh();
    }
    ApeGeomLod& lod = def->m_lods[def->m_lodCount++];
    lod.mesh = mesh;
    lod.switchDist = switchDist;
    return 1;
}

// --- ApeLightDef --------------------------------------------------------------------------

ApeLightDef::ApeLightDef() : ApeDef(kObjType_LightDef)
{
    Reset();
}

// @func 0x00586148 ApeLightDef::Reset
void ApeLightDef::Reset()
{
    ResetBase();
    m_color = kWorldUnitScale;
    m_lightType = 0;
    m_lightModel = 1;
    m_intensity = 1.0f;
    m_priority = 0x7F;
    m_atten[0] = 0.0f;
    m_atten[1] = 1.0f;
}

// @func 0x005863F8 ApeLightDef::Release
void ApeLightDef::Release()
{
    if (DropRef())
        DeferDestroy(this, &ApeTDefList<ApeLightDef>::DestroyErased);
}

// @func 0x00585EF0 ApeLightDef::SetFromRecord
void ApeLightDef::SetFromRecord(const ApeLightDefRecord& record)
{
    m_sphere = {0.0f, 0.0f, 0.0f, record.radius};
    StrCopy(m_name, record.name);
    m_nameHash = record.nameHash;
    m_animHash = record.animHash;
    field_68 = record.lightGroup;
    m_lightType = record.type;
    m_color = {record.color[0] * kAlphaStep, record.color[1] * kAlphaStep, record.color[2] * kAlphaStep, m_color.pad};
    m_intensity = std::sqrt(record.intensity);
    m_priority = record.priority;
    m_lightModel = record.lightModel;

    if (record.lightModel == 0) {
        m_atten[0] = record.atten[0];
        m_atten[1] = record.atten[1];
        m_atten[2] = record.atten[2];
    } else {
        // Start/end attenuation: the end is kept past the start and bounds
        // the light.
        m_atten[0] = record.atten[0];
        m_atten[1] = std::fmax(record.atten[1], record.atten[0] + 0.01f);
        m_sphere.w = m_atten[1];
    }

    m_fadeDist = std::fabs(record.fadeDist) >= kEpsilon ? record.fadeDist : g_defaultLightFadeDist;
    float fadeRange = std::fabs(record.fadeRange) >= kEpsilon ? record.fadeRange : g_defaultLightFadeRange;
    if (fadeRange <= kEpsilon)
        fadeRange = kEpsilon;
    m_fadeRange = Clamp(fadeRange, kEpsilon, 100.0f);
    AabbFromSphere(m_bounds, m_sphere);
}

// @func 0x0059BEE4 LoadLightDefChunk
int LoadLightDefChunk(iff::IffReader* reader, void* arg)
{
    ParseAndRegister(reader, ApeTDefList<ApeLightDef>::Create());
    return 1;
}

// @func 0x0059C294 LoadLightDefRecordChunk
int LoadLightDefRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeLightDefRecord record;
    std::memset(&record, 0, sizeof(record));
    uint8_t version = 0;
    reader->ReadU8(&version);
    reader->ReadU8(&record.type);
    reader->ReadU8(&record.lightModel);
    reader->ReadU32(&record.lightGroup);
    reader->ReadRaw4(record.color);
    reader->ReadFloat(&record.intensity);
    if (record.lightModel == 0) {
        reader->ReadFloat(&record.atten[0]);   // lightModel1.constantAtten
        reader->ReadFloat(&record.atten[1]);   // lightModel1.linearAtten
        reader->ReadFloat(&record.atten[2]);   // lightModel1.quadraticAtten
    } else {
        reader->ReadFloat(&record.atten[0]);   // lightModel2.startAtten
        reader->ReadFloat(&record.atten[1]);   // lightModel2.endAtten
    }
    reader->ReadU32(&record.nameHash);
    reader->ReadU32(&record.animHash);
    reader->ReadRaw32(record.name);
    reader->ReadFloat(&record.radius);
    reader->ReadU8(&record.priority);
    reader->ReadFloat(&record.fadeDist);
    reader->ReadFloat(&record.fadeRange);
    static_cast<ApeLightDef*>(arg)->SetFromRecord(record);
    return 1;
}

// --- ApeCameraDef -------------------------------------------------------------------------

ApeCameraDef::ApeCameraDef() : ApeDef(kObjType_CameraDef)
{
    Reset();
}

// @func 0x0059C7B0 ApeCameraDef::Reset
void ApeCameraDef::Reset()
{
    ResetBase();
    Matrix34Copy(m_transform, kWorldIdentity);
    m_fov = 0.0f;
}

// @func 0x0059CA84 ApeCameraDef::Release
void ApeCameraDef::Release()
{
    if (DropRef())
        DeferDestroy(this, &ApeTDefList<ApeCameraDef>::DestroyErased);
}

// @func 0x0059CACC LoadCameraDefChunk
int LoadCameraDefChunk(iff::IffReader* reader, void* arg)
{
    ParseAndRegister(reader, ApeTDefList<ApeCameraDef>::Create());
    return 1;
}

// @func 0x0059CF98 LoadCameraDefRecordChunk
int LoadCameraDefRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeCameraDef* def = static_cast<ApeCameraDef*>(arg);
    uint8_t version = 0;
    float xBasis[3] = {}, zBasis[3] = {}, position[3] = {};
    float fov = 0.0f;
    uint32_t nameHash = 0, animHash = 0;
    char name[32] = {};
    reader->ReadU8(&version);
    reader->ReadVec3(xBasis);
    reader->ReadVec3(zBasis);
    reader->ReadVec3(position);
    reader->ReadFloat(&fov);
    reader->ReadU32(&nameHash);
    reader->ReadU32(&animHash);
    reader->ReadRaw32(name);

    def->m_fov = fov;
    StrCopy(def->m_name, name);
    def->m_nameHash = nameHash;
    def->m_animHash = animHash;
    Matrix34FromBasis(def->m_transform, xBasis, zBasis, position);
    return 1;
}

// --- ApeEmitterDef ------------------------------------------------------------------------

ApeEmitterDef::ApeEmitterDef() : ApeDef(kObjType_EmitterDef) {}

// The particle members are torn down by the destructor (0x0059D360, not
// ported) once the deferred destruction runs.
// @func 0x0059D52C ApeEmitterDef::Release
void ApeEmitterDef::Release()
{
    if (DropRef())
        DeferDestroy(this, &ApeTDefList<ApeEmitterDef>::DestroyErased);
}

// @func 0x0059DCC0 LoadEmitterDefRecordChunk
int LoadEmitterDefRecordChunk(iff::IffReader* reader, void* arg)
{
    ApeEmitterDef* def = static_cast<ApeEmitterDef*>(arg);
    uint8_t version = 0;
    float bboxMin[3] = {}, bboxMax[3] = {};
    uint32_t nameHash = 0, animHash = 0, lightGroup = 0;
    char name[32] = {};
    reader->ReadU8(&version);
    reader->ReadVec3(bboxMin);
    reader->ReadVec3(bboxMax);
    reader->ReadU32(&nameHash);
    reader->ReadU32(&animHash);
    reader->ReadU32(&lightGroup);
    reader->ReadRaw32(name);

    def->m_bounds.min = ToVec3(bboxMin);
    def->m_bounds.max = ToVec3(bboxMax);
    SphereFromAabb(def->m_sphere, def->m_bounds);
    StrCopy(def->m_name, name);
    def->m_nameHash = nameHash;
    def->m_animHash = animHash;
    def->field_68 = lightGroup;
    return 1;
}

// --- ApePointDef --------------------------------------------------------------------------

ApePointDef::ApePointDef() : ApeDef(kObjType_PointDef) {}

// @func 0x0059E1C0 ApePointDef::Release
void ApePointDef::Release()
{
    if (DropRef())
        DeferDestroy(this, &ApeTDefList<ApePointDef>::DestroyErased);
}

// @func 0x0059E9B0 ApePointDef::GetDefault
ApePointDef* ApePointDef::GetDefault()
{
    static ApePointDef* s_default = nullptr;   // 0x00F88A00
    if (!s_default)
        s_default = ApeTDefList<ApePointDef>::Create();
    return s_default;
}

// @func 0x0059E208 LoadPointDefChunk
int LoadPointDefChunk(iff::IffReader* reader, void* arg)
{
    ParseAndRegister(reader, ApeTDefList<ApePointDef>::Create());
    return 1;
}

// The bounds of a point are kept around the origin; the radius reaches the
// farther box corner.
// @func 0x0059E678 LoadPointDefRecordChunk
int LoadPointDefRecordChunk(iff::IffReader* reader, void* arg)
{
    ApePointDef* def = static_cast<ApePointDef*>(arg);
    uint8_t version = 0;
    uint32_t nameHash = 0;
    char name[32] = {};
    float bboxMin[3] = {}, bboxMax[3] = {};
    reader->ReadU8(&version);
    reader->ReadU32(&nameHash);
    reader->ReadRaw32(name);
    reader->ReadVec3(bboxMin);
    reader->ReadVec3(bboxMax);

    def->m_bounds.min = ToVec3(bboxMin);
    def->m_bounds.max = ToVec3(bboxMax);
    const float minSq = bboxMin[0] * bboxMin[0] + bboxMin[1] * bboxMin[1] + bboxMin[2] * bboxMin[2];
    const float maxSq = bboxMax[0] * bboxMax[0] + bboxMax[1] * bboxMax[1] + bboxMax[2] * bboxMax[2];
    const float farSq = minSq > maxSq ? minSq : maxSq;
    def->m_sphere.w = farSq > 0.0f ? std::sqrt(farSq) : 0.0f;
    StrCopy(def->m_name, name);
    def->m_nameHash = nameHash;
    return 1;
}

// --- ApeSplineDef -------------------------------------------------------------------------

// @func 0x0059ECF8 ApeSplineDef::ApeSplineDef
ApeSplineDef::ApeSplineDef() : ApeDef(kObjType_SplineDef) {}

// @func 0x0059EF6C ApeSplineDef::~ApeSplineDef
// @func 0x0059F030 ApeSplineDef::~ApeSplineDef (deleting)
ApeSplineDef::~ApeSplineDef() = default;

// @func 0x0059F204 ApeSplineDef::Release
void ApeSplineDef::Release()
{
    if (DropRef())
        DeferDestroy(this, &ApeTDefList<ApeSplineDef>::DestroyErased);
}

// @func 0x0059F174 ApeSplineDef::InitFromHeader
void ApeSplineDef::InitFromHeader(const ApeSplineDefHeader& header)
{
    StrNCopy(m_name, header.name, 31);
    m_nameHash = HashNameNoCase(m_name);
    m_spline.Init(header.numPoints, (header.flags & 1) != 0, (header.flags & 2) != 0);
}

// @func 0x0059F110 ApeSplineDef::UpdateBounds
void ApeSplineDef::UpdateBounds(bool skipLength)
{
    if (!skipLength)
        m_spline.ComputeLength();
    m_spline.GetBounds(m_bounds, m_sphere);
}

void ApeSplineDef::FinishLoad()
{
    const uint32_t count = m_spline.GetNumPoints();
    if (count > 1) {
        // The level data stores both control points of a segment as absolute
        // positions with the segment's first point: inTangent holds the
        // outgoing handle, outTangent the next point's incoming one.
        for (int i = int(count) - 1; i >= 0; --i) {
            SplinePoint& p = m_spline.GetPoint(uint32_t(i));
            p.outTangent = {p.inTangent.x - p.position.x, p.inTangent.y - p.position.y,
                            p.inTangent.z - p.position.z, p.outTangent.pad};
            if (i > 0) {
                const SplinePoint& prev = m_spline.GetPoint(uint32_t(i - 1));
                p.inTangent = {prev.outTangent.x - p.position.x, prev.outTangent.y - p.position.y,
                               prev.outTangent.z - p.position.z, p.inTangent.pad};
            }
        }
        m_spline.GetPoint(0).inTangent = kWorldZero;
        m_spline.GetPoint(count - 1).outTangent = kWorldZero;

        if (!m_spline.GetFlag()) {
            // Make the two handles of every interior point collinear.
            for (uint32_t i = 1; i + 1 < count; ++i) {
                SplinePoint& p = m_spline.GetPoint(i);
                const Vec3 d = {p.outTangent.x - p.inTangent.x, p.outTangent.y - p.inTangent.y,
                                p.outTangent.z - p.inTangent.z, 0.0f};
                const float lengthSq = d.x * d.x + d.y * d.y + d.z * d.z;
                if (lengthSq < 1e-6f)
                    continue;
                const float inv = 1.0f / std::sqrt(lengthSq);
                const Vec3 dir = {d.x * inv, d.y * inv, d.z * inv, 0.0f};
                const float along = dir.x * p.outTangent.x + dir.y * p.outTangent.y + dir.z * p.outTangent.z;
                const Vec3 perp = {p.outTangent.x - dir.x * along, p.outTangent.y - dir.y * along,
                                   p.outTangent.z - dir.z * along, 0.0f};
                p.inTangent.x -= perp.x;
                p.inTangent.y -= perp.y;
                p.inTangent.z -= perp.z;
                p.outTangent.x -= perp.x;
                p.outTangent.y -= perp.y;
                p.outTangent.z -= perp.z;
            }
        }
    }
    m_spline.ComputeLength();
    m_spline.GetBounds(m_bounds, m_sphere);
}

// @func 0x005A0000 ApeSplineDef::GetDefault
ApeSplineDef* ApeSplineDef::GetDefault()
{
    static ApeSplineDef* s_default = nullptr;   // 0x00F88A04
    if (!s_default)
        s_default = ApeTDefList<ApeSplineDef>::Create();
    return s_default;
}

// Spline definition container: the header chunk creates (or, on a reload,
// reuses) the definition, the point chunk fills it.
// @func 0x0059F5D8 LoadSplineDefChunk
int LoadSplineDefChunk(iff::IffReader* reader, void* arg)
{
    ApeSplineDef* def = nullptr;
    reader->ParseChunks(&def);
    if (!def)
        return 1;
    def->FinishLoad();
    ApeTDefList<ApeSplineDef>::Register(def);
    return 1;
}

// @func 0x0059F8E4 LoadSplineDefHeaderChunk
int LoadSplineDefHeaderChunk(iff::IffReader* reader, void* arg)
{
    ApeSplineDef*& def = *static_cast<ApeSplineDef**>(arg);
    ApeSplineDefHeader header;
    std::memset(&header, 0, sizeof(header));
    uint8_t version = 0;
    reader->ReadU8(&version);
    reader->ReadRaw32(header.name);
    reader->ReadU16(&header.flags);
    reader->ReadU16(&header.numPoints);

    // NOTE: reader flag 0x04 marks a reload; an existing definition of the
    // same name is then refilled in place.
    def = ApeTDefList<ApeSplineDef>::Find(HashNameNoCase(header.name));
    if ((reader->flags & 0x04) && def)
        return 1;

    def = ApeTDefList<ApeSplineDef>::Create();
    ApeTDefList<ApeSplineDef>::RunLoadCallback(def, 0);
    def->InitFromHeader(header);
    ApeTDefList<ApeSplineDef>::RunLoadCallback(def, 1);
    return 1;
}

// Points: position and the two control points of the following segment.
// @func 0x0059FBD0 LoadSplineDefPointsChunk
int LoadSplineDefPointsChunk(iff::IffReader* reader, void* arg)
{
    ApeSplineDef* def = *static_cast<ApeSplineDef**>(arg);
    ApeSpline& spline = def->m_spline;
    uint32_t count = reader->GetChunkSize() / 36;

    const uint32_t stored = spline.GetNumPoints();
    const bool closed = spline.IsClosed();
    const uint32_t real = closed ? stored - 1 : stored;
    if (real != count) {
        if (count > stored)
            count = stored;
        // NOTE: like the console, the closing point is added to the count
        // passed to Init(), which adds another one for closed splines.
        spline.Init(closed ? count + 1 : count, closed, spline.GetFlag());
    }

    for (uint32_t i = 0; i < count; ++i) {
        float position[3], control1[3], control2[3];
        reader->ReadVec3(position);
        reader->ReadVec3(control1);
        reader->ReadVec3(control2);
        SplinePoint& p = spline.GetPoint(i);
        p.position = ToVec3(position);
        p.inTangent = ToVec3(control1);
        p.outTangent = ToVec3(control2);
    }
    if (closed) {
        SplinePoint& last = spline.GetPoint(count);
        const SplinePoint& first = spline.GetPoint(0);
        last.position = first.position;
        last.inTangent = first.inTangent;
        last.outTangent = first.outTangent;
    }
    return 1;
}

} // namespace ape::world
