// openTIT - The effect kinds the FX manager plays (0x002EC2F8-0x002F5D6C).
//
// A play request (FxManager::Play, 0x002F0E0C) takes an effect description
// whose first word names the kind. Every description starts with the same
// head (kind, time scale, world object); the rest is kind specific. Script
// commands (FxObjShake, FxTrail), the actor effect lists and the sequencer
// keep such descriptions inside their own data and hand them over as is.
//
// Kinds (description hash -> class):
//   0x489EBD81 object shake       FxObjShakeEffect   (vtable 0x00B1D5A0, 0x84 bytes)
//   0xEB2F2138 trail              FxTrailEffect      (vtable 0x00B1D550, 0x100 bytes)
//   0xD0E940EF geometry display   FxGeoDisplayEffect (vtable 0x00B1D500, 0xD0 bytes)
//   0x996564D0 motion trail       FxMotionTrailEffect(vtable 0x00B1D578, 0x14 bytes)
//   0x8733FD75 delayed copies     FxDelayEffect      (vtable 0x00B1D528, 0x9C bytes)
// The trail, geometry display and delayed-copy kinds are only partly ported.
#pragma once

#include "sly/game/FxManager.h"

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { namespace world { class ApeObj; struct ApeSkeletonBone; class ApeGeomDef; } }

namespace sly {
namespace game {

enum FxEffectKind : uint32_t {
    kFx_ObjShake    = 0x489EBD81u,
    kFx_Trail       = 0xEB2F2138u,
    kFx_GeoDisplay  = 0xD0E940EFu,
    kFx_MotionTrail = 0x996564D0u,
    kFx_Delay       = 0x8733FD75u,
};

// Head shared by every effect description.
struct FxEffectDesc {
    uint32_t kind = 0;                        // +0x00 FxEffectKind
    // +0x04 the owner's time scale (entity +0xD4, see FxEffect::value).
    float timeScale = 1.0f;
    ape::world::ApeObj* object = nullptr;     // +0x08 object the effect is attached to
};

// Object shake (the FxObjShake script command's block from its +0x20).
struct FxObjShakeDesc : FxEffectDesc {
    uint32_t bone = 0;                        // +0x0C bone (or child object) that shakes
    float duration = 1.0f;                    // +0x10 negative: until stopped
    float magnitude = 1.0f;                   // +0x14
    float startMagnitude = 0.0f;              // +0x18
    uint32_t falloffMode = 0x85D98325u;       // +0x1C FxShakeFalloff
    float falloffStartTime = 0.0f;            // +0x20
    float angles[3] = {1.0f, 0.0f, 1.0f};     // +0x24 pitch, yaw, roll amplitude (degrees)
    float frequencies[3] = {4.0f, 6.5f, 2.5f};// +0x30 per axis (Hz)
    float offsets[3] = {};                    // +0x3C X/Y/Z offset
    float extra[8] = {};                      // +0x48 NOTE: further shape parameters
    float field_68 = 0.0f;                    // +0x68
    float cullDistance = -1.0f;               // +0x6C negative: the tuning default
    bool flags[4] = {};                       // +0x70 per axis "no oscillation", flag 3 unknown
};

// Fall-off curves of an object shake once falloffStartTime has passed.
enum FxShakeFalloff : uint32_t {
    kShakeFalloff_Linear      = 0x85D98325u,   // "Linear"
    kShakeFalloff_Smooth      = 0x920469AEu,   // "Smooth" (half sine)
    kShakeFalloff_Exponential = 0x17197D25u,   // "Exponential" (squared)
    kShakeFalloff_ToStart     = 0x9F9D6710u,   // NOTE: name unknown; blends towards startMagnitude
};

// Motion trail: the object is drawn into the motion trail post-process.
struct FxMotionTrailDesc : FxEffectDesc {
    float timeStep = 0.016f;                  // +0x0C clamped to the effect's limits
    float alpha = 0.75f;                      // +0x10 tint alpha
    uint32_t pad14[3] = {};
    float tint[3] = {1.0f, 1.0f, 1.0f};       // +0x20
    uint32_t pad2C = 0;
    float params[6] = {};                     // +0x30 copied to the effect (+0x70..+0x84)
    float distance = 0.0f;                    // +0x48 squared into the effect (+0x88)
    bool drawHidden = false;                  // +0x4C also while the object is hidden
    bool children = false;                    // +0x4D also the geometry children
};

// -- the effect classes ------------------------------------------------------------

// Object shake: rotates a bone (or the object) with up to three sine waves
// whose amplitude follows the fall-off curve.
class FxObjShakeEffect : public FxEffect {
public:
    explicit FxObjShakeEffect(const FxObjShakeDesc& desc);
    ~FxObjShakeEffect() override;
    // Slot 2: queues the shake on the game world's callback list 3.
    void Slot2() override;
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;
    // Advances the shake by `dt` and poses the bone (0x002F4A38, TODO).
    void Tick(float dt);

    ape::world::ApeObj* attachedTo = nullptr;          // +0x0C
    ape::world::ApeObj* child = nullptr;               // +0x10 child object named like the bone
    ape::world::ApeSkeletonBone* bone = nullptr;       // +0x14
    ape::world::ApeSkeletonBone* parentBone = nullptr; // +0x18
    float elapsed = 0.0f;                              // +0x1C
    float duration = 0.0f;                             // +0x20
    float magnitude = 0.0f;                            // +0x24
    float startMagnitude = 0.0f;                       // +0x28
    uint32_t falloffMode = 0;                          // +0x2C
    float falloffStartTime = 0.0f;                     // +0x30
    float angles[3] = {};                              // +0x34 radians
    float frequencies[3] = {};                         // +0x40
    float offsets[3] = {};                             // +0x4C
    float extra[8] = {};                               // +0x58
    float field_78 = 0.0f;                             // +0x78
    float cullDistanceSq = 0.0f;                       // +0x7C
    bool flags[4] = {};                                // +0x80
};

// Trail ribbon between two bones (only the attach overrides are ported).
class FxTrailEffect : public FxEffect {
public:
    explicit FxTrailEffect(const FxEffectDesc& desc);   // TODO(0x002F0058)
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;
    ape::world::ApeObj* attachedTo = nullptr;   // +0xD4
};

// "FXAnimControlGeoDisplay": shows a geometry definition at a bone.
class FxGeoDisplayEffect : public FxEffect {
public:
    explicit FxGeoDisplayEffect(const FxEffectDesc& desc);   // TODO(0x002EC920)
    ape::world::ApeObj* attachedTo = nullptr;   // +0x10 (the base attach overrides are kept)
};

// Delayed copies ("FXDly%02d_<name>"): clones of the object that replay its
// recorded transforms and poses a little later (only the attach overrides
// are ported).
class FxDelayEffect : public FxEffect {
public:
    explicit FxDelayEffect(const FxEffectDesc& desc);   // TODO(0x002ED6F4)
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;
    ape::world::ApeObj* attachedTo = nullptr;   // +0x0C
};

// Motion trail (see FxMotionTrailDesc).
class FxMotionTrailEffect : public FxEffect {
public:
    explicit FxMotionTrailEffect(const FxMotionTrailDesc& desc);
    ~FxMotionTrailEffect() override;
    // Slot 3: queues the object (and its children when asked) for the trail.
    void Update() override;
    // Slot 5: ends the effect and switches the post-process off.
    void Stop() override;
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;

    ape::world::ApeObj* attachedTo = nullptr;   // +0x0C
    bool drawHidden = false;                    // +0x10
    bool children = false;                      // +0x11

private:
    void QueueObject(ape::world::ApeObj* object);
};

// Applies a pending switch of the motion trail post-process (0x01068841 /
// 0x01068840) to the main view; called once per frame.
void ApplyMotionTrailState();

// Draws `object` into the motion trail mask this frame: 0x005A1548 pushed
// on the game world's draw call buffer (+0x1285E8). TODO stub.
void QueueMotionTrailDraw(ape::world::ApeObj* object);

} // namespace game
} // namespace sly
