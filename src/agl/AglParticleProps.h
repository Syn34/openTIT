// openTIT - Agl particle emitter properties: the description of an emitter
// loaded from a scene file ("EMPC" chunk and its property chunks).
//
// An emitter's properties hold the emission parameters (rates, lifetimes,
// culling distances, burst timing) and up to eight reference counted
// property objects that drive each particle: the initial velocity (a
// sphere or box of directions), the colour over the particle's life, the
// size over its life, a constant force, rotation, texture animation and a
// path. The particle system that evaluates them is not ported yet.
#pragma once

#include "ape/spline/ApeSpline.h"

#include <cstdint>

namespace ape::iff { struct IffReader; }

namespace agl {

// Base of the property objects: a 16-bit reference count; the last release
// destroys the object (virtual slot 1 on the console).
class AglParticleProperty {
public:
    AglParticleProperty() = default;
    virtual ~AglParticleProperty() = default;
    AglParticleProperty(const AglParticleProperty&) = delete;
    AglParticleProperty& operator=(const AglParticleProperty&) = delete;

    void AddRef() { ++refCount; }
    void Release()
    {
        if (--refCount < 1)
            delete this;
    }
    // Drops the reference the creator held without destroying the object
    // (the property slot that took it keeps it alive).
    void DropCreatorRef() { --refCount; }

    int16_t refCount = 1;               // +0x04
};

// Initial velocity: a speed and a cone of directions, emitted from a sphere
// or a box (console classes 0x00B23170/0x00B23190 and 0x00B231B0/0x00B231D0).
class AglVelocityProperty : public AglParticleProperty {
public:
    enum Shape : uint8_t { kShape_Sphere = 0, kShape_Box = 1 };

    Shape shape = kShape_Sphere;
    bool fromSurface = false;           // flags bit 0. NOTE: meaning guessed (it selects the class)
    float speed = 0.0f;                 // +0x08
    float speedVariance = 0.0f;         // +0x0C
    bool hasSpread = false;             // +0x10 any offset or spread angle set
    float minAngleX = 0.0f;             // +0x14 radians
    float spreadX = 0.0f;               // +0x18
    float minAngleZ = 0.0f;             // +0x1C
    float spreadZ = 0.0f;               // +0x20
    float radius = 0.0f;                // +0x24 sphere
    float halfExtents[3] = {};          // +0x30 box
};

// A constant force (console class 0x00B23260), optionally only after a delay.
class AglForceProperty : public AglParticleProperty {
public:
    float force[3] = {};                // +0x10
    float delay = 0.0f;                 // +0x20 "noForceTime"
};

// Colour keys spread over the particle's life with alpha fades at both ends
// (console class 0x00B23200).
class AglColorProperty : public AglParticleProperty {
public:
    static constexpr int kMaxKeys = 16;
    // Adds a key; the first key is also stored as the second, so a single
    // colour still forms a two key gradient.
    void AddKey(const uint8_t rgba[4]);

    uint8_t count = 0;                  // +0x06
    int16_t shadeVariance = 0;          // +0x08 8.8 fixed point
    float fadeInRate = 1e8f;            // +0x0C life / fade-in time (1e8: no fade)
    float fadeOutRate = 1e8f;           // +0x10
    uint8_t keys[kMaxKeys][4] = {};     // +0x14 RGBA
};

// Size keys over the particle's life (console class 0x00B23220).
class AglSizeProperty : public AglParticleProperty {
public:
    static constexpr int kMaxKeys = 16;
    void AddKey(float size);

    uint32_t count = 0;                 // +0x08
    float variance = 0.0f;              // +0x0C clamped to [0, 1]
    float keys[kMaxKeys] = {};          // +0x10
};

// Texture animation through the frames of a strip (console class
// 0x00B23280). NOTE: the per-mode values are named from their formulas.
class AglTexAnimProperty : public AglParticleProperty {
public:
    float speed = 1.0f;                 // +0x08 frames per second
    float frameWidth = 0.0f;            // +0x0C 1 / frame count
    float frameCount = 0.0f;            // +0x10
    uint8_t mode = 0;                   // +0x14 1..4
    float frameStep = 0.0f;             // +0x18 1 / frame count (modes 1-4)
    float field_1C = 0.0f;              // +0x1C mode 2: speed / frames; mode 3: speed / span
    float span = 0.0f;                  // +0x20 mode 3: (frames - 1) * 2 (there and back)
};

// Collision planes the particles bounce off (console class 0x00B232A0).
class AglCollisionProperty : public AglParticleProperty {
public:
    static constexpr int kMaxPlanes = 6;
    uint32_t count = 0;                 // +0x08
    float bounce = 1.0f;                // +0x0C
    float planes[kMaxPlanes][4] = {};   // +0x10 normal and distance
};

// Fields of the emitter record chunk ("EMPH").
struct AglEmitterRecord {
    float ttl = 0.0f;
    float ttlVariance = 0.0f;
    float emitRate = 0.0f;
    uint32_t emitObjectHash = 0;
    uint8_t emitObjectType = 0;
    uint8_t renderFlags = 0;
    uint8_t processClass = 0;
    uint8_t alignment = 0;
    float particleNearZCull = 0.0f;
    float particleFarZCull = 0.0f;
    float emitterFarZCull = 0.0f;
    float emitterFarZFadeDist = 0.0f;
    float emitRateVariance = 0.0f;
    float burstDelay = 0.0f;
    float burstDelayVariance = 0.0f;
    float burstDuration = 0.0f;
    float burstDurationVariance = 0.0f;
    uint32_t emitMeshHash[3] = {};
};

// Spin around an axis (console class 0x00B23240, 0xC0 bytes). The rate
// keys are in degrees; angles[] holds the running total at each key in
// radians, with one more entry than there are keys.
class AglRotationProperty : public AglParticleProperty {
public:
    // Which unit axis the rotation is about (kAxis_Any for other axes).
    enum Axis : uint8_t { kAxis_X = 0, kAxis_Y = 1, kAxis_Z = 2, kAxis_Any = 3 };
    static constexpr uint32_t kMaxKeys = 16;

    ape::Vec3 axis = {};                 // +0x10 normalised
    float variance = 0.0f;               // +0x20 in [-1, 1]
    uint32_t keyCount = 0;               // +0x24 entries of angles[]
    float angles[kMaxKeys] = {};         // +0x28
    float rates[kMaxKeys] = {};          // +0x68
    float lifetime = 0.0f;               // +0xA8 the properties' ttl
    float initialAngleRange = 0.0f;      // +0xAC in turns
    Axis axisType = kAxis_Any;           // +0xB0
    bool randomDirection = false;        // +0xB1
};

// Path the particles follow: a Bezier spline (console class 0x00B232C0;
// the spline sits at +0x10).
class AglPathProperty : public AglParticleProperty {
public:
    ape::ApeSpline spline;              // +0x10
};

// The emitter properties (0x8C bytes, pooled; console AglEmitProperties).
// NOTE: the meaning of most scalar fields comes from the emitter record,
// whose consumer (0x004FF4C0) is not ported; they keep their offsets.
class AglEmitProperties {
public:
    AglEmitProperties();
    ~AglEmitProperties();
    AglEmitProperties(const AglEmitProperties&) = delete;
    AglEmitProperties& operator=(const AglEmitProperties&) = delete;

    // Copies the emitter record into the properties and picks the
    // particle material or meshes.
    void ApplyRecord(const AglEmitterRecord& record);
    // Sprite particles drawn with 'material' (switched to the particle
    // shader class).
    void SetMaterial(class AglMaterial* material);
    // Adds a mesh to emit (switched to the mesh particle shader class).
    void AddMesh(class AglSubMesh* mesh);

    int16_t refCount = 1;               // +0x00
    float ttl = 0.0f;                   // +0x04 particle lifetime (seconds); the colour fades are relative to it
    float invTtl = 0.0f;                // +0x08 1 / ttl
    float ttlVariance = 0.0f;           // +0x0C fraction, clamped to [0, 0.75]
    float emitRate = 1.0f;              // +0x10 particles per second
    float emitRateVariance = 0.0f;      // +0x14 [0, 0.75]
    float burstDelay = 0.0f;            // +0x18
    float burstDelayVariance = 0.0f;    // +0x1C [0, 0.75]
    float burstDuration = 0.0f;         // +0x20
    float burstDurationVariance = 0.0f; // +0x24 [0, 0.75]
    uint16_t maxParticles = 0;          // +0x28 rate x lifetime, at least 1
    // Render flags: bit 7 / 6 / 5 from record flags 0x04 / 0x20 / 0x10
    // (meanings unknown); the constructor sets bit 4.
    uint8_t flags2A = 0x10;
    uint8_t processClass = 0;           // +0x2B
    uint8_t alignment = 0;              // +0x2C
    float particleNearCull = -3.0f;     // +0x30 negated distances (view space z)
    float particleFarCull = -50.0f;     // +0x34
    float emitterFarCull = -50.0f;      // +0x38
    float emitterFadeDist = 5.0f;       // +0x3C at least 1
    float field_40 = 1.0f;              // +0x40 1 when record flag 0x08 is set, else 0
    bool emitsMeshes = false;           // +0x44 particles are meshes rather than sprites
    // +0x48: the sprite material, or (overlaid on the console) up to four
    // particle meshes.
    class AglMaterial* material = nullptr;
    class AglSubMesh* meshes[4] = {};
    uint8_t meshCount = 0;              // +0x58
    // +0x5A: largest pre-recorded mesh draw (console command words).
    uint16_t maxDrawWords = 0;
    float aspect[2] = { 1.0f, 1.0f };   // +0x5C particle width / height (normalised to max 1)
    float materialParams[2] = {};       // +0x64 the material's first two constants
    AglVelocityProperty* velocity = nullptr;   // +0x6C
    AglColorProperty* color = nullptr;         // +0x70
    AglSizeProperty* size = nullptr;           // +0x74
    AglForceProperty* force = nullptr;         // +0x78
    AglRotationProperty* rotation = nullptr;   // +0x7C
    AglTexAnimProperty* texAnim = nullptr;     // +0x80
    AglCollisionProperty* collision = nullptr; // +0x84
    AglPathProperty* path = nullptr;           // +0x88
};

// Load callback run for each new emitter properties object (phase 0 before
// its chunks are read, phase 1 after; 0x00F8F128 +8/+0xC).
void SetEmitPropertiesLoadCallback(void (*callback)(AglEmitProperties* props, void* arg, int phase), void* arg);

// Chunk handlers ("EMPC", "EMPH", "EPIB", "EPMN", "EPNM", "EPTC", "EPCT", "EPTS").
int LoadEmitPropertiesChunk(ape::iff::IffReader* reader, void* arg);
int LoadEmitterRecordChunk(ape::iff::IffReader* reader, void* arg);
int LoadVelocityChunk(ape::iff::IffReader* reader, void* arg);
int LoadForceChunk(ape::iff::IffReader* reader, void* arg);
int LoadDelayedForceChunk(ape::iff::IffReader* reader, void* arg);
int LoadColorChunk(ape::iff::IffReader* reader, void* arg);
int LoadShadedColorChunk(ape::iff::IffReader* reader, void* arg);
int LoadSizeChunk(ape::iff::IffReader* reader, void* arg);
int LoadTexAnimChunk(ape::iff::IffReader* reader, void* arg);         // "EPUV"
int LoadCollisionChunk(ape::iff::IffReader* reader, void* arg);       // "EPCP"
int LoadPathChunk(ape::iff::IffReader* reader, void* arg);            // "EPSP"
int LoadRotationChunk(ape::iff::IffReader* reader, void* arg);        // "EPTR"
// Registers the handlers above (part of the particle system start-up 0x0050E614).
void RegisterParticleChunkHandlers();

} // namespace agl
