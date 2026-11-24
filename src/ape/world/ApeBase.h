// openTIT - Ape engine world object base class, shared constants and object types.
//
// Every world definition (geometry, light, camera, emitter, point and spline
// definitions, see ApeDef.h) and every placed object (ApeObj and its
// instances, see ApeObj.h) derives from one small engine base (vtable
// 0x00B242B8 on the console). The base carries
//   * a 16-bit reference count, released through the virtual Release(),
//   * a 6-bit object type (ApeObjType),
//   * a unique id taken from a global counter,
//   * a reference to a shared "tag block" (attached parameters, built by the
//     level's ATAG/TAG2 chunks; not ported yet, see NOTES.md).
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world {

// Object type ids (6 bits). Instances and their definitions are 12 apart.
enum ApeObjType : uint32_t {
    kObjType_None            = 0,
    kObjType_GeomInstance    = 8,
    kObjType_LightInstance   = 9,
    kObjType_EmitterInstance = 10,
    kObjType_CameraInstance  = 11,
    kObjType_PointInstance   = 12,
    kObjType_SplineInstance  = 13,
    kObjType_FirstInstance   = kObjType_GeomInstance,
    kObjType_EndRenderable   = 15,  // types [8, 15) own a render node
    // NOTE: AttachChild() climbs the parent chain until it meets an object of
    // this type and makes it the root of the attached subtree. Which class it
    // is (probably a game-side container object) is not known.
    kObjType_HierarchyRoot   = 0x13,
    kObjType_GeomDef         = 0x14,
    kObjType_LightDef        = 0x15,
    kObjType_EmitterDef      = 0x16,
    kObjType_CameraDef       = 0x17,
    kObjType_PointDef        = 0x18,
    kObjType_SplineDef       = 0x19,
};
constexpr uint32_t kDefTypeOffset = kObjType_GeomDef - kObjType_GeomInstance;

// Constant vectors and matrices the world code initialises objects from.
// NOTE: on the console these are globals filled by static initialisers
// (0x01164B30 unit scale, 0x01164B40 zero, 0x01164BC0 empty sphere,
// 0x01164BD0 identity, 0x01193010 identity used as the default pivot).
extern const Vec3 kWorldUnitScale;
extern const Vec3 kWorldZero;
extern const Sphere kWorldEmptySphere;
extern const Matrix34 kWorldIdentity;

// Tolerance used when comparing scales against kWorldUnitScale.
constexpr float kScaleEpsilon = 1e-5f;

struct ApeTagBlock;   // shared parameter block (ATAG/TAG2), not ported yet

class ApeBase {
public:
    explicit ApeBase(uint32_t type);
    virtual ~ApeBase();

    ApeBase(const ApeBase&) = delete;
    ApeBase& operator=(const ApeBase&) = delete;

    // Drops one reference; the object is destroyed when none is left. The
    // base version does nothing (0x00558F34); world objects call it from
    // their own Release().
    virtual void Release();

    void AddRef() { ++m_refCount; }
    int16_t GetRefCount() const { return m_refCount; }
    // Decrements the count and returns true when it dropped below one.
    bool DropRef() { return --m_refCount < 1; }

    uint32_t GetType() const { return m_type; }
    uint32_t GetUniqueId() const { return m_uniqueId; }

    // Bit kept in the base flags while a definition is registered in its
    // definition list's name map.
    bool IsRegistered() const { return m_registered; }
    void SetRegistered(bool registered) { m_registered = registered; }

    // Shares the tag block of another object.
    void ShareTags(const ApeBase& other);

protected:
    int16_t m_refCount = 1;
    uint8_t m_type = 0;            // ApeObjType
    // NOTE: the low 10 bits of the console word holding the count and type
    // are preserved by every constructor and never read in this module.
    uint8_t m_tagFlags = 0;        // NOTE: 4 bits; non-zero runs the tag release hook
    bool m_registered = false;
    uint32_t m_uniqueId = 0;
    ApeTagBlock* m_tags = nullptr;
};

// Next unique object id (0x01164CD0). Ids start at 1.
extern uint32_t g_nextObjectUniqueId;

} // namespace ape::world
