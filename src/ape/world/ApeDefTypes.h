// openTIT - The world definition types: geometry, light, camera, emitter,
// point and spline definitions, and their level chunk handlers.
#pragma once

#include "ape/spline/ApeSpline.h"
#include "ape/world/ApeDef.h"

#include <cstdint>

namespace ape::world {

struct AglMesh;
struct ApeSkeletonDef;

// --- geometry ------------------------------------------------------------------------

struct ApeGeomLod {
    AglMesh* mesh;
    float switchDist;    // the LOD is used up to this camera distance
};

// Draw request handed to definitions when an instance is drawn.
// NOTE: only the fields the world code touches are known.
struct ApeDrawParams {
    uint8_t field_00[0x30];
    Vec3 position;       // 0x30 world position of the instance
    uint8_t field_40[5];
    uint8_t alpha;       // 0x45 0..255, scaled by distance fades
};

// Geometry definition record as stored in the level data.
struct ApeGeomDefRecord {
    float bboxMin[3];          // 0x00
    float bboxMax[3];          // 0x0C
    uint32_t nameHash;         // 0x18
    uint32_t animHash;         // 0x1C
    uint32_t lightGroup;       // 0x20
    char name[32];             // 0x24
    uint32_t collisionHash;    // 0x44
    float nearZFadeRange;      // 0x48
    float nearZFadeDist;       // 0x4C
    uint8_t shadowType;        // 0x50
    uint8_t shadowExtrusionLength; // 0x51
    uint8_t shadowLod;         // 0x52
    float shadowZCullDist;     // 0x54
    float minNearFadeAlpha;    // 0x58
};

class ApeGeomDef : public ApeDef {
public:
    static constexpr uint32_t kDefType = kObjType_GeomDef;
    static constexpr int kMaxLods = 4;

    ApeGeomDef();
    ~ApeGeomDef() override;
    void Release() override;
    // Draws the LOD selected for the draw parameters; false when nothing
    // is drawn.
    int VirtualSlot5(void* drawParams) override;
    void VirtualSlot6(int value) override;

    void SetFromRecord(ApeGeomDefRecord& record);
    // LOD for a draw request (distance from the LOD camera minus the
    // definition radius), applying the near and far fades to the request's
    // alpha. Null when the object is out of range or fully faded.
    ApeGeomLod* SelectLod(ApeDrawParams& params);
    // Copy with duplicated meshes and a shared skeleton.
    ApeGeomDef* Clone() const;

    uint8_t m_lodCount = 0;            // 0x70
    uint8_t m_shadowType = 0;          // 0x71
    uint8_t m_shadowExtrusionLength = 0; // 0x72
    uint8_t m_shadowLod = 0;           // 0x73 index of the shadow mesh
    float m_shadowZCullDist = 0.0f;    // 0x74
    float m_nearZFadeRange = 0.0f;     // 0x78
    float m_nearZFadeDist = 0.0f;      // 0x7C
    float m_minNearFadeAlpha = 0.0f;   // 0x80
    ApeGeomLod m_lods[kMaxLods] = {};  // 0x84
    ApeSkeletonDef* m_skeletonDef = nullptr; // 0xA4
};

// --- light -------------------------------------------------------------------------------

struct ApeLightDefRecord {
    uint8_t type;              // 0x00
    uint8_t lightModel;        // 0x01 0: constant/linear/quadratic, else start/end
    uint32_t lightGroup;       // 0x04
    uint8_t color[4];          // 0x08 RGBA
    float intensity;           // 0x0C
    float atten[3];            // 0x10 model 0: constant, linear, quadratic; else start, end
    uint32_t nameHash;         // 0x1C
    uint32_t animHash;         // 0x20
    char name[32];             // 0x24
    float radius;              // 0x44
    uint8_t priority;          // 0x48
    float fadeDist;            // 0x4C
    float fadeRange;           // 0x50
};

class ApeLightDef : public ApeDef {
public:
    static constexpr uint32_t kDefType = kObjType_LightDef;

    ApeLightDef();
    void Release() override;

    // Default light: white, model 1, intensity 1, priority 127.
    void Reset();
    void SetFromRecord(const ApeLightDefRecord& record);

    Vec3 m_color;                  // 0x70 0..1
    uint32_t m_lightType = 0;      // 0x80
    uint32_t m_lightModel = 1;     // 0x84
    float m_intensity = 1.0f;      // 0x88 square root of the stored intensity
    uint8_t m_priority = 0x7F;     // 0x8C
    float m_atten[3] = {};         // 0x90
    float m_fadeRange = 0.0f;      // 0x9C
    float m_fadeDist = 0.0f;       // 0xA0
};

// --- camera ------------------------------------------------------------------------------

class ApeCameraDef : public ApeDef {
public:
    static constexpr uint32_t kDefType = kObjType_CameraDef;

    ApeCameraDef();
    void Release() override;
    void Reset();
    // @func 0x0059CA7C ApeCameraDef::VirtualSlot5
    int VirtualSlot5(void* arg) override { return 0; }
    // @func 0x0059CAC8 ApeCameraDef::VirtualSlot6
    void VirtualSlot6(int arg) override {}

    Matrix34 m_transform;          // 0x70
    float m_fov = 0.0f;            // 0xB0
};

// --- emitter -----------------------------------------------------------------------------

class ApeEmitterDef : public ApeDef {
public:
    static constexpr uint32_t kDefType = kObjType_EmitterDef;

    ApeEmitterDef();
    void Release() override;
    // @func 0x0059D6B8 ApeEmitterDef::VirtualSlot6
    void VirtualSlot6(int arg) override {}

    // NOTE: the particle system members (0x70 particle definition, 0x74
    // particle ring buffer) belong to the renderer and are not ported.
    void* m_particleDef = nullptr;     // 0x70
    void* m_particleBuffer = nullptr;  // 0x74
    uint8_t m_isDynamic = 0;           // 0x78 cleared when the instance leaves the dynamic list
};

// --- point -------------------------------------------------------------------------------

class ApePointDef : public ApeDef {
public:
    static constexpr uint32_t kDefType = kObjType_PointDef;

    ApePointDef();
    void Release() override;
    // @func 0x0059E204 ApePointDef::VirtualSlot6
    void VirtualSlot6(int arg) override {}

    // Shared definition used by point instances created at run time.
    static ApePointDef* GetDefault();
};

// --- spline ------------------------------------------------------------------------------

struct ApeSplineDefHeader {
    char name[32];             // 0x00
    uint16_t flags;            // 0x20 bit 0 closed, bit 1 no tangent smoothing
    uint16_t numPoints;        // 0x22
};

class ApeSplineDef : public ApeDef {
public:
    static constexpr uint32_t kDefType = kObjType_SplineDef;

    ApeSplineDef();
    ~ApeSplineDef() override;
    void Release() override;
    // @func 0x0059F5CC ApeSplineDef::VirtualSlot6
    void VirtualSlot6(int arg) override {}

    void InitFromHeader(const ApeSplineDefHeader& header);
    // Recomputes the arc length (unless skipped) and the bounds.
    void UpdateBounds(bool skipLength);
    // Turns the absolute control points read from the level data into
    // tangents relative to their points, and smooths interior tangents
    // unless the spline asks not to.
    void FinishLoad();

    // Shared definition used by spline instances created at run time.
    static ApeSplineDef* GetDefault();

    ApeSpline m_spline;            // 0x70
};

// --- chunk handlers --------------------------------------------------------------------

int LoadGeomDefChunk(iff::IffReader* reader, void* arg);
int LoadGeomDefRecordChunk(iff::IffReader* reader, void* arg);
int LoadGeomDefLodChunk(iff::IffReader* reader, void* arg);
int LoadLightDefChunk(iff::IffReader* reader, void* arg);
int LoadLightDefRecordChunk(iff::IffReader* reader, void* arg);
int LoadCameraDefChunk(iff::IffReader* reader, void* arg);
int LoadCameraDefRecordChunk(iff::IffReader* reader, void* arg);
int LoadEmitterDefRecordChunk(iff::IffReader* reader, void* arg);
int LoadPointDefChunk(iff::IffReader* reader, void* arg);
int LoadPointDefRecordChunk(iff::IffReader* reader, void* arg);
int LoadSplineDefChunk(iff::IffReader* reader, void* arg);
int LoadSplineDefHeaderChunk(iff::IffReader* reader, void* arg);
int LoadSplineDefPointsChunk(iff::IffReader* reader, void* arg);
// Locators of a definition ("numLocators" and the locator records).
int LoadLocatorListChunk(iff::IffReader* reader, void* arg);
int LoadLocatorChunk(iff::IffReader* reader, void* arg);

} // namespace ape::world
