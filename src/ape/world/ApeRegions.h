// openTIT - Visibility regions and portal quads of a level.
//
// A level is split into up to 32 visibility regions. Each region is an
// oriented box (stored as its axis aligned bounds plus the six planes of the
// oriented volume) with a bit index and a mask of the regions visible from
// it. Region files can be loaded in groups: the ids and masks of a later
// group are shifted past the regions already loaded.
//
// Quads are named four-corner polygons (portals / occluders) loaded from a
// separate chunk family.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/sys/ApeGeometry.h"

#include <cstdint>

namespace ape::iff {
struct IffReader;
}

namespace ape::world {

class ApeObj;

struct alignas(16) ApeRegion {
    uint8_t id;            // 0x00 bit index in region masks
    uint8_t group;         // 0x01 load group the region came from
    uint8_t flags;         // 0x02 kRegionFlag_*
    uint8_t pad03;
    uint32_t visibleMask;  // 0x04 regions visible from here (includes itself)
    uint32_t field_08[2];
    Aabb bounds;           // 0x10
    Plane planes[6];       // 0x30 oriented volume, normals pointing inside
    char name[32];         // 0x90
    uint32_t nameHash;     // 0xB0
    uint32_t field_B4[3];
};

enum ApeRegionFlags : uint8_t {
    kRegionFlag_Oriented = 0x01,   // the planes must be tested as well as the bounds
};

constexpr int kMaxRegions = 32;

struct ApeRegionSet {
    uint8_t field_00[0x10];
    uint8_t count;         // 0x10 regions loaded
    uint8_t groupBase;     // 0x11 first region of the group being loaded
    uint8_t groupCount;    // 0x12 groups loaded
    uint8_t pad13[0x0D];
    ApeRegion regions[kMaxRegions];   // 0x20
};

// The level's region set (0x01193084); null when the level has no regions.
extern ApeRegionSet* g_regionSet;

// Mask of the regions whose volume contains a world box.
uint32_t ComputeRegionMask(const ApeRegionSet& set, const Aabb& worldBox);
// Region with the given name hash, or null.
ApeRegion* FindRegionByNameHash(ApeRegionSet& set, uint32_t nameHash);

struct alignas(16) ApeQuad {
    Vec3 corners[4];       // 0x00
    uint32_t nameHash;     // 0x40
    uint16_t flags;        // 0x44 four flag bits
    char name[32];         // 0x46 (at most 31 characters)
    uint8_t pad[10];
};

struct ApeQuadList {
    ApeQuad* quads = nullptr;   // 0x011930A0
    uint16_t count = 0;         // 0x011930A4
};
extern ApeQuadList g_quads;

ApeQuad* FindQuad(uint32_t nameHash);

// Chunk handlers.
int LoadRegionSetChunk(iff::IffReader* reader, void* arg);   // "numRegions"
int LoadRegionChunk(iff::IffReader* reader, void* arg);
int LoadQuadListChunk(iff::IffReader* reader, void* arg);    // "numQuads"
int LoadQuadCornersChunk(iff::IffReader* reader, void* arg);
int LoadQuadNameHashesChunk(iff::IffReader* reader, void* arg);
int LoadQuadFlagsChunk(iff::IffReader* reader, void* arg);
int LoadQuadNamesChunk(iff::IffReader* reader, void* arg);

// Per-level counters cleared when the quad list is replaced (0x011944B0,
// 0x01193000). NOTE: their meaning is unknown.
extern uint16_t g_worldCounter44B0;
extern uint16_t g_worldCounter3000;

} // namespace ape::world
