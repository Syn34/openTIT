// openTIT - Visibility regions and portal quads of a level.
#include "ape/world/ApeRegions.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"

#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeInstances.h"

#include <cfloat>
#include <cmath>

#include <cstring>

namespace ape::world {

ApeRegionSet* g_regionSet = nullptr;
ApeQuadList g_quads;
uint16_t g_worldCounter44B0 = 0;
uint16_t g_worldCounter3000 = 0;

namespace {

bool QuatIsIdentity(const Quat& q)
{
    constexpr float kEpsilon = 1e-5f;
    return std::fabs(q.x) < kEpsilon && std::fabs(q.y) < kEpsilon && std::fabs(q.z) < kEpsilon &&
           std::fabs(q.w - 1.0f) < kEpsilon;
}

} // namespace

// The six planes bounding `box` after transforming it by `m`, normals
// pointing inside (a point p is inside when dot(normal, p) + w >= 0).
// NOTE: decoded from behaviour; the console routine is vector code Ghidra
// cannot read, so the plane order (-x, +x, -y, +y, -z, +z faces) is a guess.
// @func 0x00576BD8 BuildBoxPlanes
void BuildBoxPlanes(Plane planes[6], const Aabb& box, const Matrix34& m)
{
    const Vec3* axes[3] = {&m.xAxis, &m.yAxis, &m.zAxis};
    const float mins[3] = {box.min.x, box.min.y, box.min.z};
    const float maxs[3] = {box.max.x, box.max.y, box.max.z};
    for (int axis = 0; axis < 3; ++axis) {
        const Vec3& a = *axes[axis];
        const float length = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
        const float inv = length > 0.0f ? 1.0f / length : 0.0f;
        const Vec3 n = {a.x * inv, a.y * inv, a.z * inv, 0.0f};
        // Offset of the face along the (unit) axis in world space.
        const float originAlong = n.x * m.translation.x + n.y * m.translation.y + n.z * m.translation.z;
        const float minAlong = originAlong + mins[axis] * length;
        const float maxAlong = originAlong + maxs[axis] * length;
        planes[axis * 2] = {n.x, n.y, n.z, -minAlong};
        planes[axis * 2 + 1] = {-n.x, -n.y, -n.z, maxAlong};
    }
}

// Region volume from its box, rotation and the level offset: the bounds of
// the box rotated about its centre and, for a real rotation, the oriented
// planes.
// @func 0x00576F28 BuildRegionVolume
void BuildRegionVolume(ApeRegion& region, const Aabb& box, const Quat& rotation, const Vec3& offset)
{
    Vec3 center;
    AabbCenter(box, center);
    Matrix34 transform;
    Matrix34FromQuat(transform, rotation, {center.x + offset.x, center.y + offset.y, center.z + offset.z, 0.0f});
    Aabb localBox = {{FLT_MAX, FLT_MAX, FLT_MAX, 0.0f}, {-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f}};
    AabbTranslated(localBox, box, {-center.x, -center.y, -center.z, 0.0f});
    AabbTransform(region.bounds, localBox, transform);
    if (!QuatIsIdentity(rotation)) {
        region.flags |= kRegionFlag_Oriented;
        BuildBoxPlanes(region.planes, localBox, transform);
    }
}

// @func 0x0057711C ApeRegionSet::ApeRegionSet
ApeRegionSet::ApeRegionSet() : ApeBase(kObjType_RegionSet)
{
    for (ApeRegion& region : regions) {
        region.id = 0xFF;
        region.group = 0;
        region.flags = 0;
        region.visibleMask = 0xFFFFFFFFu;
        region.bounds.min = {FLT_MAX, FLT_MAX, FLT_MAX, 0.0f};
        region.bounds.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f};
        region.name[0] = '\0';
        region.nameHash = 0;
        region.field_B4[0] = 0;
    }
}

// @func 0x00577200 ApeRegionSet::~ApeRegionSet
// @func 0x00577230 ApeRegionSet::~ApeRegionSet (deleting)
ApeRegionSet::~ApeRegionSet() = default;

// @func 0x00577288 ApeRegionSet::Release
void ApeRegionSet::Release()
{
    if (DropRef())
        delete this;
}

// @func 0x00577338 ApeRegionSet::ComputeObjectMask
uint32_t ApeRegionSet::ComputeObjectMask(const ApeObj& object) const
{
    const Aabb& localBounds = object.GetDef()->m_bounds;
    Vec3 corners[8];
    AabbTransformCorners(localBounds, corners, object.GetWorld());
    Aabb worldBox;
    AabbTransform(worldBox, localBounds, object.GetWorld());

    uint32_t mask = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const ApeRegion& region = regions[i];
        bool inside = true;
        if (region.flags & kRegionFlag_Oriented) {
            for (const Vec3& corner : corners) {
                for (const Plane& plane : region.planes) {
                    if (plane.x * corner.x + plane.y * corner.y + plane.z * corner.z + plane.w < 0.0f) {
                        inside = false;
                        break;
                    }
                }
                if (!inside)
                    break;
            }
        } else {
            inside = AabbContainsBox(region.bounds, worldBox);
        }
        if (inside)
            mask |= 1u << (region.id & 0x3F);
    }
    return mask;
}

// @func 0x00577520 ApeRegionSet::UpdateVisibleMask
bool ApeRegionSet::UpdateVisibleMask(const Vec3& point)
{
    bool found = false;
    uint32_t mask = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const ApeRegion& region = regions[i];
        if (!AabbContainsPoint(region.bounds, point))
            continue;
        bool inside = true;
        if (region.flags & kRegionFlag_Oriented) {
            for (const Plane& plane : region.planes) {
                if (plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w < 0.0f) {
                    inside = false;
                    break;
                }
            }
        }
        if (inside) {
            found = true;
            mask |= region.visibleMask;
        }
    }
    visibleMask = found ? mask : 0xFFFFFFFFu;
    return found;
}

// @func 0x005772E4 GetObjectWorldBounds
Aabb& GetObjectWorldBounds(const ApeObj& object, Aabb& out)
{
    AabbTransform(out, object.GetDef()->m_bounds, object.GetWorld());
    return out;
}

uint32_t ComputeRegionMask(const ApeRegionSet& set, const Aabb& worldBox)
{
    uint32_t mask = 0;
    for (uint32_t i = 0; i < set.count; ++i) {
        const ApeRegion& region = set.regions[i];
        if (!AabbContainsBox(region.bounds, worldBox))
            continue;
        if ((region.flags & kRegionFlag_Oriented) && !FrustumContainsAabb(region.planes, worldBox))
            continue;
        mask |= 1u << (region.id & 0x3F);
    }
    return mask;
}

ApeRegion* FindRegionByNameHash(ApeRegionSet& set, uint32_t nameHash)
{
    for (uint32_t i = 0; i < set.count; ++i)
        if (set.regions[i].nameHash == nameHash)
            return &set.regions[i];
    return nullptr;
}

// Starts a new group of regions.
// @func 0x0059AF84 LoadRegionSetChunk
int LoadRegionSetChunk(iff::IffReader* reader, void* arg)
{
    g_regionSet->groupBase = g_regionSet->count;
    uint8_t version = 0;
    uint16_t numRegions = 0;
    reader->ReadU8(&version);
    reader->ReadU16(&numRegions);
    ++g_regionSet->groupCount;
    return 1;
}

// @func 0x0059B018 LoadRegionChunk
int LoadRegionChunk(iff::IffReader* reader, void* arg)
{
    uint8_t version = 0;
    uint8_t regionId = 0;
    uint32_t visibleMask = 0;
    float minPoint[3] = {}, maxPoint[3] = {};
    float rotation[4] = {};
    char name[32] = {};
    uint32_t nameHash = 0;
    reader->ReadU8(&version);
    reader->ReadU8(&regionId);
    reader->ReadU32(&visibleMask);
    reader->ReadVec3(minPoint);
    reader->ReadVec3(maxPoint);
    reader->ReadVec4(rotation);
    reader->ReadRaw32(name);
    reader->ReadU32(&nameHash);

    ApeRegionSet& set = *g_regionSet;
    const uint8_t base = set.groupBase;
    if (base != 0) {
        regionId = uint8_t(regionId + base);
        visibleMask <<= (base & 0x3F);
    }
    ApeRegion& region = set.regions[set.count++];
    region.group = set.groupCount;
    region.id = regionId;
    region.visibleMask = visibleMask | (1u << (regionId & 0x3F));
    const Aabb box = {{minPoint[0], minPoint[1], minPoint[2], 0.0f}, {maxPoint[0], maxPoint[1], maxPoint[2], 0.0f}};
    BuildRegionVolume(region, box, {rotation[0], rotation[1], rotation[2], rotation[3]}, g_levelInstanceOffset);
    StrCopyBounded(region.name, name, sizeof(region.name));
    // The hash of the name is computed and then replaced by the stored one.
    region.nameHash = nameHash;
    return 1;
}

// Quad list header: allocates the quad array. When the file is reloaded
// (reader flag 0x04) with the same quad count the old array is kept.
// @func 0x0059B288 LoadQuadListChunk
int LoadQuadListChunk(iff::IffReader* reader, void* arg)
{
    uint8_t version = 0;
    uint16_t count = 0;
    reader->ReadU8(&version);
    reader->ReadU16(&count);

    // NOTE: the meaning of reader flag 0x04 is inferred from this use.
    if ((reader->flags & 0x04) && g_quads.count == count)
        return 1;

    if (g_quads.quads) {
        MemFree(g_quads.quads);
        g_quads.quads = nullptr;
    }
    g_worldCounter44B0 = 0;
    g_worldCounter3000 = 0;
    g_quads.count = count;
    if (count > 0) {
        g_quads.quads = static_cast<ApeQuad*>(MemAlloc(sizeof(ApeQuad) * count, kMemClass_Default, 16));
        std::memset(g_quads.quads, 0, sizeof(ApeQuad) * count);
    }
    return 1;
}

// @func 0x0059B388 LoadQuadCornersChunk
int LoadQuadCornersChunk(iff::IffReader* reader, void* arg)
{
    reader->GetChunkSize();
    for (uint32_t i = 0; i < g_quads.count; ++i) {
        for (Vec3& corner : g_quads.quads[i].corners) {
            float v[3];
            reader->ReadVec3(v);
            corner.x = v[0];
            corner.y = v[1];
            corner.z = v[2];
        }
    }
    return 1;
}

// @func 0x0059B474 LoadQuadNameHashesChunk
int LoadQuadNameHashesChunk(iff::IffReader* reader, void* arg)
{
    reader->GetChunkSize();
    for (uint32_t i = 0; i < g_quads.count; ++i)
        reader->ReadU32(&g_quads.quads[i].nameHash);
    return 1;
}

// @func 0x0059B51C LoadQuadFlagsChunk
int LoadQuadFlagsChunk(iff::IffReader* reader, void* arg)
{
    reader->GetChunkSize();
    for (uint32_t i = 0; i < g_quads.count; ++i) {
        uint16_t flags = 0;
        reader->ReadU16(&flags);
        g_quads.quads[i].flags = uint16_t(flags & 0x0F);
    }
    return 1;
}

// Names are stored as 64-byte records; at most 31 characters are kept.
// @func 0x0059B648 LoadQuadNamesChunk
int LoadQuadNamesChunk(iff::IffReader* reader, void* arg)
{
    reader->GetChunkSize();
    for (uint32_t i = 0; i < g_quads.count; ++i) {
        char record[64];
        reader->Read(record, sizeof(record));
        record[sizeof(record) - 1] = '\0';
        int length = StrLen(record);
        if (length > 31)
            length = 31;
        StrNCopy(g_quads.quads[i].name, record, size_t(length));
    }
    return 1;
}

// @func 0x0059B724 FindQuad
ApeQuad* FindQuad(uint32_t nameHash)
{
    for (uint32_t i = 0; i < g_quads.count; ++i)
        if (g_quads.quads[i].nameHash == nameHash)
            return &g_quads.quads[i];
    return nullptr;
}

} // namespace ape::world
