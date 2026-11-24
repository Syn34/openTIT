// openTIT - Visibility regions and portal quads of a level.
#include "ape/world/ApeRegions.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"

#include <cstring>

namespace ape::world {

ApeRegionSet* g_regionSet = nullptr;
ApeQuadList g_quads;
uint16_t g_worldCounter44B0 = 0;
uint16_t g_worldCounter3000 = 0;

namespace {

// TODO(0x00576F28 BuildRegionVolume): the console builds the region's
// bounds and oriented planes from the box corners, the rotation quaternion
// and a scene global (0x011944E0). That routine belongs to the scene module
// and is not ported; only the axis aligned bounds are filled in here.
void BuildRegionVolume(ApeRegion& region, const Vec3& minPoint, const Vec3& maxPoint, const Vec4& rotation)
{
    region.bounds.min = minPoint;
    region.bounds.max = maxPoint;
    std::memset(region.planes, 0, sizeof(region.planes));
}

} // namespace

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
    BuildRegionVolume(region, {minPoint[0], minPoint[1], minPoint[2], 0.0f},
                      {maxPoint[0], maxPoint[1], maxPoint[2], 0.0f},
                      {rotation[0], rotation[1], rotation[2], rotation[3]});
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
