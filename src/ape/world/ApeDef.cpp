// openTIT - World object definitions (ApeDef), lookups and shared helpers.
#include "ape/world/ApeDef.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMem.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeObj.h"

#include <cfloat>

namespace ape::world {

// --- ApeDef --------------------------------------------------------------------------

ApeDef::ApeDef(uint32_t type) : ApeBase(type)
{
    m_bounds.min = {FLT_MAX, FLT_MAX, FLT_MAX, 0.0f};
    m_bounds.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f};
    ResetBase();
}

// The definition types without members of their own share this destructor
// on the console (geometry, light, camera, point definitions).
// @func 0x00584A6C ApeDef::~ApeDef
// @func 0x0058627C ApeDef::~ApeDef (copy 2)
// @func 0x0059C90C ApeDef::~ApeDef (copy 3)
// @func 0x0059E050 ApeDef::~ApeDef (copy 4)
// @func 0x00584B14 ApeDef::~ApeDef (deleting)
// @func 0x00586324 ApeDef::~ApeDef (deleting, copy 2)
// @func 0x0059C9B4 ApeDef::~ApeDef (deleting, copy 3)
// @func 0x0059E0F8 ApeDef::~ApeDef (deleting, copy 4)
ApeDef::~ApeDef()
{
    ReleaseLocatorSet(m_locators);
    m_locators = nullptr;
}

void ApeDef::ResetBase()
{
    AabbFromPoint(m_bounds, kWorldZero);
    m_sphere = kWorldEmptySphere;
    m_nameHash = 0;
    m_animHash = 0;
    m_name[0] = '\0';
    field_68 = 0;
    ReleaseLocatorSet(m_locators);
    m_locators = nullptr;
}

// @func 0x0058492C ApeDef::CopyFrom
void ApeDef::CopyFrom(const ApeDef& other)
{
    m_bounds.min.x = other.m_bounds.min.x;
    m_bounds.min.y = other.m_bounds.min.y;
    m_bounds.min.z = other.m_bounds.min.z;
    m_bounds.max.x = other.m_bounds.max.x;
    m_bounds.max.y = other.m_bounds.max.y;
    m_bounds.max.z = other.m_bounds.max.z;
    m_sphere = other.m_sphere;
    StrCopy(m_name, other.m_name);
    m_nameHash = other.m_nameHash;
    m_animHash = other.m_animHash;
    field_68 = other.field_68;
    if (m_locators != other.m_locators) {
        ApeLocatorSet* old = m_locators;
        m_locators = other.m_locators;
        if (m_locators)
            ++m_locators->refCount;
        ReleaseLocatorSet(old);
    }
}

// @func 0x00584BDC ApeDef::SetName
void ApeDef::SetName(const char* name, bool updateHash)
{
    StrCopyBounded(m_name, name, sizeof(m_name));
    if (updateHash)
        m_nameHash = HashNameNoCase(name);
}

// --- registries --------------------------------------------------------------------

namespace {

constexpr uint32_t kNumKinds = kObjType_SplineInstance - kObjType_GeomInstance + 1;

// Maps a definition or instance type to 0..5, or -1.
int KindIndex(uint32_t type)
{
    if (type >= kObjType_GeomInstance && type <= kObjType_SplineInstance)
        return int(type - kObjType_GeomInstance);
    if (type >= kObjType_GeomDef && type <= kObjType_SplineDef)
        return int(type - kObjType_GeomDef);
    return -1;
}

ApeDef* FindDefOfKind(uint32_t nameHash, int kind)
{
    return *GetDefMap(kObjType_GeomDef + uint32_t(kind)).Find(nameHash);
}

ApeObj* FindInstanceOfKind(uint32_t nameHash, int kind)
{
    return *GetInstanceMap(kObjType_GeomInstance + uint32_t(kind)).Find(nameHash);
}

enum Kind { kGeom = 0, kLight = 1, kEmitter = 2, kCamera = 3, kPoint = 4, kSpline = 5 };

} // namespace

ApeTSortedMap<ApeDef*>& GetDefMap(uint32_t defType)
{
    static ApeTSortedMap<ApeDef*>* s_maps[kNumKinds] = {};
    int kind = KindIndex(defType);
    if (kind < 0)
        kind = kGeom;
    if (!s_maps[kind])
        s_maps[kind] = new ApeTSortedMap<ApeDef*>(16, nullptr, nullptr);
    return *s_maps[kind];
}

ApeTSortedMap<ApeObj*>& GetInstanceMap(uint32_t instanceType)
{
    static ApeTSortedMap<ApeObj*>* s_maps[kNumKinds] = {};
    int kind = KindIndex(instanceType);
    if (kind < 0)
        kind = kGeom;
    if (!s_maps[kind])
        s_maps[kind] = new ApeTSortedMap<ApeObj*>(16, nullptr, nullptr);
    return *s_maps[kind];
}

// @func 0x0057CE08 FindDef
ApeDef* FindDef(uint32_t nameHash, uint32_t type)
{
    if (nameHash == 0)
        return nullptr;
    const int kind = KindIndex(type);
    if (kind >= 0)
        return FindDefOfKind(nameHash, kind);
    if (ApeDef* def = FindDefOfKind(nameHash, kGeom))
        return def;
    if (ApeDef* def = FindDefOfKind(nameHash, kLight))
        return def;
    return FindDefOfKind(nameHash, kEmitter);
}

// @func 0x005801A0 FindGeomInstance
ApeObj* FindGeomInstance(uint32_t nameHash)
{
    return FindInstanceOfKind(nameHash, kGeom);
}

// @func 0x005801DC FindInstance
ApeObj* FindInstance(uint32_t nameHash, uint32_t type)
{
    if (nameHash == 0)
        return nullptr;
    switch (type) {
    case kObjType_GeomInstance:
    case kObjType_GeomDef:
        return FindInstanceOfKind(nameHash, kGeom);
    case kObjType_LightInstance:
    case kObjType_LightDef:
        return FindInstanceOfKind(nameHash, kLight);
    case kObjType_EmitterInstance:
    case kObjType_EmitterDef:
        return FindInstanceOfKind(nameHash, kEmitter);
    case kObjType_CameraInstance:
        return FindInstanceOfKind(nameHash, kCamera);
    default:
        break;
    }
    for (int kind : {kCamera, kGeom, kLight, kEmitter})
        if (ApeObj* obj = FindInstanceOfKind(nameHash, kind))
            return obj;
    return nullptr;
}

// @func 0x00580388 FindObject
ApeObj* FindObject(uint32_t nameHash, uint32_t type)
{
    if (nameHash == 0)
        return nullptr;
    switch (type) {
    case kObjType_PointInstance:
    case kObjType_PointDef:
        return FindInstanceOfKind(nameHash, kPoint);
    case kObjType_SplineInstance:
    case kObjType_SplineDef:
        return FindInstanceOfKind(nameHash, kSpline);
    default:
        break;
    }
    if (ApeObj* obj = FindInstance(nameHash, type))
        return obj;
    if (ApeObj* obj = FindInstanceOfKind(nameHash, kPoint))
        return obj;
    return FindInstanceOfKind(nameHash, kSpline);
}

// --- deferred destruction / chunk helpers --------------------------------------------

DeferredDestroyFn g_deferredDestroy = nullptr;

void DeferDestroy(void* object, void (*destroy)(void*))
{
    if (g_deferredDestroy)
        g_deferredDestroy(object, destroy);
    else
        destroy(object);
}

// @func 0x0059AF58 ParseNestedChunks
// @func 0x0059B25C ParseNestedChunks (copy 2)
int ParseNestedChunks(iff::IffReader* reader, void* arg)
{
    reader->ParseChunks(nullptr);
    return 1;
}

} // namespace ape::world
