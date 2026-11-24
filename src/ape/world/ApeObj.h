// openTIT - ApeObj: base class of every placed world object (instances).
//
// An ApeObj has a local transform (relative to its parent, after an optional
// pivot matrix), a per-axis scale, a world transform and world bounding
// sphere computed from them, a definition (ApeDef) and a place in an object
// hierarchy (parent, first child, next sibling, root).
//
// Transform changes do not update the world matrices at once: MarkDirty()
// flags the object and queues the root of its hierarchy in a global list
// that the world update walks once per frame (see UpdateWorld()).
//
// Objects of the renderable types (geometry, light, emitter and camera
// instances) also own a render node, an entry in a renderer side table (see
// ApeRenderNode.h).
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"
#include "ape/world/ApeBase.h"

#include <cstdint>

namespace ape::world {

class ApeDef;
class ApeObj;
struct ApeSkeleton;
struct ApeRenderNode;

// Placement record of an instance in the level data (instance chunks).
struct ApeObjInstanceData {
    float xBasis[3];       // 0x00
    float zBasis[3];       // 0x0C
    float position[3];     // 0x18
    float scale[3];        // 0x24
    uint32_t flags;        // 0x30 kInstanceFlag_*
    uint32_t nameHash;     // 0x34
    uint32_t field_38;     // NOTE: not read by ApeObj
    char name[32];         // 0x3C
    uint32_t defHash;      // 0x5C
};

enum ApeInstanceDataFlags : uint32_t {
    kInstanceFlag_Visible = 0x01,
    kInstanceFlag_02      = 0x02,   // copied to kObjFlag_4000
    kInstanceFlag_C0      = 0xC0,   // copied unchanged to the object flags
};

// Object flags (low half of the console word at 0x0A4; the high half holds
// the render node index).
enum ApeObjFlags : uint32_t {
    kObjFlag_C0          = 0x00C0,  // NOTE: copied from the instance data
    kObjFlag_0100        = 0x0100,  // NOTE: selects the skinned bounds path of UpdateWorld
    kObjFlag_InDirtyList = 0x0400,  // root is queued in the dirty root list
    kObjFlag_Dirty       = 0x0800,  // world transform needs recomputing
    kObjFlag_1000        = 0x1000,  // NOTE: only copied by CopyFrom
    kObjFlag_HasScale    = 0x2000,  // scale differs from (1, 1, 1)
    kObjFlag_4000        = 0x4000,  // NOTE: set recursively by SetFlag4000
    kObjFlag_Visible     = 0x8000,
};

// Transform options (console byte 0x0A9).
enum ApeObjTransformFlags : uint8_t {
    kXform_01               = 0x01,  // NOTE: meaning unknown, copied by CopyFrom
    kXform_02               = 0x02,  // NOTE: meaning unknown
    kXform_InheritScale     = 0x04,  // take the parent's scale instead of composing it
    kXform_ScaleTranslation = 0x08,  // NOTE: see UpdateWorldFromParent
    kXform_IgnoreParent     = 0x10,  // NOTE: path not decoded, see UpdateWorldFromParent
    kXform_20               = 0x20,  // set by the render object setup
    kXform_80               = 0x80,
};

constexpr uint16_t kNoRenderNode = 0xFFFF;

class ApeObj : public ApeBase {
public:
    explicit ApeObj(uint32_t type);
    ~ApeObj() override;

    // -- virtual interface (console vtable 0x00B24610) ----------------------
    virtual const char* GetName() const { return m_name; }
    // Called after the world transform was recomputed; `moved` is true when
    // the world position changed.
    virtual void OnWorldTransformUpdated(bool moved) {}
    virtual void AttachChild(ApeObj* child, int16_t parentBone);
    virtual void DetachChild(ApeObj* child);
    virtual ApeSkeleton* GetSkeleton() { return nullptr; }
    virtual void VirtualSlot8() {}
    // Pushes the world bounding sphere to the render node.
    virtual void OnBoundsChanged();
    virtual void SetVisible(bool visible, bool recurse);
    virtual void SetDef(ApeDef* def);
    // NOTE: slots 12 and 13 only recurse into the children in the base class;
    // the value they pass down is interpreted by subclasses.
    virtual void VirtualSlot12(uint32_t value, bool recurse);
    virtual void VirtualSlot13(uint8_t value, bool recurse);
    // NOTE: slot 15: asks the definition (slot 6) when count > 0 and recurses.
    virtual void VirtualSlot15(int count, int arg);
    // Rebuilds the definition's bounds as a cube/sphere of radius `radius`
    // around the current centre and updates the world bounds.
    virtual void SetBoundingRadius(float radius);

    // -- non virtual --------------------------------------------------------
    // Back to the default state (identity transforms, unit scale, visible,
    // no hierarchy, name "ApeObj::Unknown"). The definition pointer is
    // cleared without being released.
    void Reset();
    // Sets name, transform, scale, flags and definition from level data.
    // Returns true (a missing definition only prints a warning).
    bool InitFromInstanceData(const ApeObjInstanceData& data);

    enum CopyMask : uint32_t {
        kCopy_SkipDef       = 0x01,
        kCopy_SkipFlags     = 0x02,
        kCopy_SkipTags      = 0x04,
        kCopy_SkipTransform = 0x08,
        kCopy_SkipName      = 0x20,
    };
    // Copies another object's state except the parts excluded by `skipMask`.
    void CopyFrom(const ApeObj& other, uint32_t skipMask);

    void SetName(const char* name, bool updateHash);
    void SetScale(const Vec3& scale, bool recurse);
    void SetFlag4000(bool value, bool recurse);

    // Flags the object as needing a world update and queues its root.
    void MarkDirty();

    // Recomputes the world transform of a root object and of its subtree.
    void UpdateWorld(bool unused, bool forceSkeletonBounds);
    // Recomputes the world transform of a child from its parent.
    void UpdateWorldFromParent(const Vec3& parentScale);
    // World bounding sphere from the local sphere, scale and world matrix.
    const Sphere& UpdateWorldBounds();

    // Calls `fn(obj, arg)` on this object (optional) and then depth first on
    // its descendants, until fn returns false. At most ten pending siblings
    // or children are remembered, deeper levels are dropped (console limit).
    using VisitFn = bool (*)(ApeObj* obj, void* arg);
    bool ForEachInHierarchy(VisitFn fn, void* arg, bool includeSelf);

    // Releases the render nodes of this object and of its descendants.
    void ReleaseRenderNodes();

    ApeRenderNode* GetRenderNode() const;

    // -- accessors -----------------------------------------------------------
    const Matrix34& GetLocal() const { return m_local; }
    // Replaces the local transform (callers mark the object dirty).
    void SetLocal(const Matrix34& local) { m_local = local; }
    const Matrix34& GetWorld() const { return m_world; }
    const Vec3& GetScale() const { return m_scale; }
    ApeDef* GetDef() const { return m_def; }
    uint32_t GetNameHash() const { return m_nameHash; }
    ApeObj* GetParent() const { return m_parent; }
    ApeObj* GetRoot() const { return m_root; }
    ApeObj* GetFirstChild() const { return m_firstChild; }
    ApeObj* GetNextSibling() const { return m_nextSibling; }
    bool IsVisible() const { return (m_flags & kObjFlag_Visible) != 0; }
    bool IsDirty() const { return (m_flags & kObjFlag_Dirty) != 0; }
    uint16_t GetRenderNodeIndex() const { return m_renderNode; }
    void SetRenderNodeIndex(uint16_t index) { m_renderNode = index; }
    void SetRoot(ApeObj* root) { m_root = root; }

    // -- data -----------------------------------------------------------------
    Matrix34 m_local;                     // 0x010
    Matrix34 m_world;                     // 0x050
    Vec3 m_scale;                         // 0x090
    const Matrix34* m_pivot;              // 0x0A0 applied before m_local
    uint16_t m_renderNode = kNoRenderNode; // 0x0A4
    uint16_t m_flags = 0;                 // 0x0A6 ApeObjFlags
    uint8_t m_byteA8 = 0xFF;              // NOTE: 0xFF after Reset, meaning unknown
    uint8_t m_xformFlags = 0;             // 0x0A9 ApeObjTransformFlags
    char m_name[32];                      // 0x0AA
    uint32_t m_nameHash = 0;              // 0x0CC
    ApeDef* m_def = nullptr;              // 0x0D0
    Sphere m_localBounds;                 // 0x0E0 (from the definition)
    Sphere m_worldBounds;                 // 0x0F0
    ApeObj* m_root = nullptr;             // 0x100 top of the hierarchy (self when unparented)
    ApeObj* m_parent = nullptr;           // 0x104
    int16_t m_parentBone = 0;             // 0x108 bone of the parent's skeleton, -1 for none
    ApeObj* m_nextSibling = nullptr;      // 0x10C (holds a reference)
    ApeObj* m_firstChild = nullptr;       // 0x110 (holds a reference)
    void* field_114 = nullptr;            // NOTE: cleared by the constructor only
    void* field_118 = nullptr;            // NOTE: cleared by the constructor only
};

// Roots with a pending world update (0x0116BD00) and the flag that stops new
// roots from being queued while the list is processed (0x0116BD04).
ApeTArray<ApeObj*>& GetDirtyRootList();
extern bool g_dirtyRootListLocked;

// Objects with dynamic lighting (geometry and emitter instances), 0x011944C4.
ApeTArray<ApeObj*>& GetDynamicObjectList();

// Name hash -> instance map of one instance type (kObjType_GeomInstance ..
// kObjType_SplineInstance), filled by the instance loaders.
ApeTSortedMap<ApeObj*>& GetInstanceMap(uint32_t instanceType);

// Definition lookup by name hash. `type` is an instance or definition type;
// for other values the geometry, light and emitter definitions are searched.
ApeDef* FindDef(uint32_t nameHash, uint32_t type);
// Instance lookup by name hash. FindInstance covers geometry, light, emitter
// and camera instances; FindObject also point and spline instances.
ApeObj* FindGeomInstance(uint32_t nameHash);
ApeObj* FindInstance(uint32_t nameHash, uint32_t type);
ApeObj* FindObject(uint32_t nameHash, uint32_t type);

// Full affine product with row vectors: out = a * b (a applied first).
// `out` may alias `a` but not `b`.
void Matrix34Mul(Matrix34& out, const Matrix34& a, const Matrix34& b);
// Builds a transform from the x and z axes and a position (y = z x x).
void Matrix34FromBasis(Matrix34& out, const float xBasis[3], const float zBasis[3], const float position[3]);

} // namespace ape::world
