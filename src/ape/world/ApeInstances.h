// openTIT - Placed world objects: the renderable object base (ApeRenderObj)
// and the six instance types with their level chunk handlers.
//
// Hierarchy:
//   ApeObj -> ApeRenderObj -> ApeGeomInstance, ApeLightInstance,
//                             ApeEmitterInstance, ApeCameraInstance
//   ApeObj -> ApePointInstance, ApeSplineInstance
#pragma once

#include "ape/memory/ApeTFreeList.h"
#include "ape/world/ApeObj.h"

#include <cstdint>
#include <new>

namespace ape::iff {
struct IffReader;
}

namespace ape::world {

struct AglLightLink;
class ApeSplineDef;

// Level record of a placed instance (0x74 bytes in the file).
struct ApeInstanceRecord {
    ApeObjInstanceData base;   // 0x00
    uint32_t parentHash;       // 0x60
    uint32_t parentPartHash;   // 0x64
    uint32_t pvsHash;          // 0x68 region the object is pinned to (0 for automatic)
    uint32_t lightGroup;       // 0x6C
    uint8_t renderPass;        // 0x70 non-zero: dynamic object
    uint8_t sortOffset;        // 0x71
    uint8_t fogGroup;          // 0x72
};

// Reads the common instance record.
void ReadInstanceRecord(iff::IffReader* reader, ApeInstanceRecord& record);

// Render modes of geometry/emitter instances (from definition flags 0x600).
enum ApeDrawMode : uint8_t {
    kDrawMode_1 = 1,       // NOTE: definition flag 0x200
    kDrawMode_2 = 2,       // NOTE: no flag
    kDrawMode_Named = 3,   // definition flag 0x400; such instances are registered by name
    kDrawMode_Fixed = 4,   // definition flag 0x600; visibility cannot be changed
};

// Render frame bookkeeping shared with the renderer (0x010DDA7C,
// 0x010DDA84, 0x010DDA88 on the console).
extern uint32_t g_renderFrameCounter;
extern uint32_t g_renderWriteBuffer;
extern uint32_t g_renderReadBuffer;

// Offset added to root instances when they are loaded (0x011944E0).
extern Vec3 g_levelInstanceOffset;

// Base of the renderable instances (vtable 0x00B24660).
class ApeRenderObj : public ApeObj {
public:
    explicit ApeRenderObj(uint32_t type);
    ~ApeRenderObj() override;

    // Base "release" of render objects: leaves the dynamic list, detaches
    // the children; the reference count is handled by the instance types.
    void Release() override;
    void OnWorldTransformUpdated(bool moved) override;
    ApeSkeleton* GetSkeleton() override { return m_skeleton; }
    void SetVisible(bool visible, bool recurse) override;
    // Slot 12: sort offset; slot 13: the byte at 0x0A8 (redraws on change).
    void VirtualSlot12(uint32_t value, bool recurse) override;
    void VirtualSlot13(uint8_t value, bool recurse) override;

    void InitRenderFields();
    void InitFromRecord(const ApeInstanceRecord& record);
    void CopyRenderFrom(const ApeRenderObj& other, uint32_t skipMask);

    void SetLightGroupMask(uint32_t mask);
    // Geometry instances only accept a new mask when they already have one.
    bool TrySetLightGroupMask(uint32_t mask);
    // Recomputes the region mask from the world bounds (or resolves a pinned
    // region when `pinned` is set).
    void UpdateRegionMask(bool resolvePinned);
    void SetShadowParams(uint32_t unused, uint32_t a, uint32_t b, bool recurse);
    void SetLodField(uint32_t value);
    bool IsDrawnThisFrame() const;
    void SetFrameStamp(uint32_t stamp) { m_frameStamp[g_renderWriteBuffer & 1] = stamp; }
    void RemoveFromDynamicList();
    bool IsDynamic() const { return (m_flags141 & 0x02) != 0; }
    uint8_t GetDrawMode() const { return m_drawMode; }

    uint16_t m_visSlot = 0xFFFF;          // 0x11C
    AglLightLink* m_lightLink = nullptr;  // 0x120
    uint32_t m_lightGroupMask = 0;        // 0x124
    int32_t field_128 = -1;
    ApeSkeleton* m_skeleton = nullptr;    // 0x12C
    uint32_t m_frameStamp[2] = {};        // 0x130 per render buffer
    float m_drawDistSq = 0.0f;            // 0x138
    uint8_t m_color[4] = {};              // 0x13C
    uint8_t m_sortOffset = 0;             // 0x140
    uint8_t m_flags141 = 0;               // bit 0 fog group, bit 1 dynamic
    uint8_t m_lodField = 0;               // 6 bits (console bits 18..23 of word 0x140)
    uint8_t m_flags142 = 0;               // bit 7 instance flag 0x10, bit 6 skinned mesh, bit 5 new
    uint8_t m_drawMode = 0;               // 4 bits (console bits 9..12 of word 0x140)
    uint32_t m_regionMask = 0;            // 0x144 (a region name hash while kXform_80 is set)
    uint16_t field_148 = 2;
    uint32_t m_shadowParamA = 0;          // 0x14C
    uint32_t m_shadowParamB = 0;          // 0x150
};

// Default RGBA of render objects (0x01164CA0).
extern uint8_t g_defaultObjectColor[4];

// Per-type instance pool and name registration (0x00F8F144.. on the
// console, one static block per type).
template <class T>
class ApeTInstanceList {
public:
    static T* Create()
    {
        void* storage = ApeTListAllocator<T>::GetAllocator().Alloc();
        return storage ? new (storage) T() : nullptr;
    }
    static void SetLoadCallback(void (*callback)(ApeObj*, void*, int), void* arg)
    {
        State().callback = callback;
        State().callbackArg = arg;
    }
    static void RunLoadCallback(T* obj, int phase)
    {
        if (State().callback)
            State().callback(obj, State().callbackArg, phase);
    }
    static void Register(T* obj)
    {
        GetInstanceMap(T::kInstanceType).Add(obj->GetNameHash(), obj);
        obj->SetRegistered(true);
    }
    // @func 0x005A1CE0 ApeTInstanceList<ApeGeomInstance>::Destroy
    // @func 0x005A1E94 ApeTInstanceList<ApeGeomInstance>::Destroy (thunk)
    // @func 0x005A3BEC ApeTInstanceList<ApeLightInstance>::Destroy
    // @func 0x005A3DA0 ApeTInstanceList<ApeLightInstance>::Destroy (thunk)
    // @func 0x005A4F28 ApeTInstanceList<ApeEmitterInstance>::Destroy
    // @func 0x005A50DC ApeTInstanceList<ApeEmitterInstance>::Destroy (thunk)
    // @func 0x005A664C ApeTInstanceList<ApeCameraInstance>::Destroy
    // @func 0x005A72EC ApeTInstanceList<ApePointInstance>::Destroy
    // @func 0x005A89F0 ApeTInstanceList<ApeSplineInstance>::Destroy
    static void Destroy(void* object)
    {
        T* obj = static_cast<T*>(object);
        if (obj->IsRegistered()) {
            ApeTSortedMap<ApeObj*>& map = GetInstanceMap(T::kInstanceType);
            if (*map.Find(obj->GetNameHash()) == obj)
                map.Remove(obj->GetNameHash());
            obj->SetRegistered(false);
        }
        obj->~T();
        ApeTListAllocator<T>::GetAllocator().Free(obj);
    }

private:
    struct StateBlock {
        void (*callback)(ApeObj*, void*, int) = nullptr;
        void* callbackArg = nullptr;
    };
    static StateBlock& State()
    {
        static StateBlock s_state;
        return s_state;
    }
};

class ApeGeomInstance : public ApeRenderObj {
public:
    static constexpr uint32_t kInstanceType = kObjType_GeomInstance;
    ApeGeomInstance() : ApeRenderObj(kInstanceType) {}
    void Release() override;
    void SetDef(ApeDef* def) override;
};

class ApeLightInstance : public ApeRenderObj {
public:
    static constexpr uint32_t kInstanceType = kObjType_LightInstance;
    ApeLightInstance() : ApeRenderObj(kInstanceType) {}
    void Release() override;
};

class ApeEmitterInstance : public ApeRenderObj {
public:
    static constexpr uint32_t kInstanceType = kObjType_EmitterInstance;
    ApeEmitterInstance() : ApeRenderObj(kInstanceType) {}
    void Release() override;
};

class ApeCameraInstance : public ApeRenderObj {
public:
    static constexpr uint32_t kInstanceType = kObjType_CameraInstance;
    ApeCameraInstance() : ApeRenderObj(kInstanceType) {}
    void Release() override;
};

class ApePointInstance : public ApeObj {
public:
    static constexpr uint32_t kInstanceType = kObjType_PointInstance;
    ApePointInstance() : ApeObj(kInstanceType) {}
    void Release() override;
};

class ApeSplineInstance : public ApeObj {
public:
    static constexpr uint32_t kInstanceType = kObjType_SplineInstance;
    ApeSplineInstance() : ApeObj(kInstanceType) {}
    void Release() override;

    // Spline queries in world space (the definition's spline is in the
    // instance's local space). They return the segment found.
    int EvaluatePosition(float t, Vec3& outPos, uint32_t startSegment, bool reverse);
    int EvaluatePosition(float t, Vec3& outPos, bool reverse);
    int EvaluateTangent(float t, Vec3& outTangent, bool reverse);
    int Evaluate(float t, Vec3& outPos, Vec3& outTangent, uint32_t startSegment, bool reverse);
    int Evaluate(float t, Vec3& outPos, Vec3& outTangent, bool reverse);
    float FindClosestParam(const Vec3& worldPoint, uint32_t* outSegment, float* outDistance) const;

    ApeSplineDef* GetSplineDef() const;
};

// Chunk handlers: the instance chunk (arg: parent object or null) and the
// record chunk inside it (arg: the instance).
int LoadGeomInstanceChunk(iff::IffReader* reader, void* arg);
int LoadLightInstanceChunk(iff::IffReader* reader, void* arg);
int LoadEmitterInstanceChunk(iff::IffReader* reader, void* arg);
int LoadCameraInstanceChunk(iff::IffReader* reader, void* arg);
int LoadPointInstanceChunk(iff::IffReader* reader, void* arg);
int LoadSplineInstanceChunk(iff::IffReader* reader, void* arg);
int LoadGeomInstanceRecordChunk(iff::IffReader* reader, void* arg);
int LoadRenderInstanceRecordChunk(iff::IffReader* reader, void* arg);
int LoadCameraInstanceRecordChunk(iff::IffReader* reader, void* arg);
int LoadPointInstanceRecordChunk(iff::IffReader* reader, void* arg);
int LoadSplineInstanceRecordChunk(iff::IffReader* reader, void* arg);
// Parses nested chunks only when the target is a geometry definition.
int ParseGeomDefChunks(iff::IffReader* reader, void* arg);

} // namespace ape::world
