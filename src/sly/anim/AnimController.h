// openTIT - The game's animation controller (0x0034A398-0x00351A40).
//
// Every animated world object (entity models, HUD elements, props) gets an
// AnimController (0x78 bytes, vtable 0x00B1F548). It owns the animation
// layers playing on the object (AnimLayer.h), the Havok rig the layers are
// bound to and the pose they are sampled into. All controllers are kept in
// a global list; far away objects only animate every few frames (the
// controllers take turns through a phase counter) and objects that are not
// drawn only animate within their cull distance.
#pragma once

#include "sly/anim/AnimHavok.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::world { class ApeObj; }

namespace sly {
namespace anim {

class AnimLayer;

// Animation event callback (controller +0x3C): `event` is the annotation
// text pointer with bit 0 cleared.
using AnimEventCallback = void (*)(uint32_t event, bool fromAnnotation, void* userData);

class AnimController {
public:
    // Flag word (+0x28).
    enum Flags : uint16_t {
        kFlag_20         = 1u << 5,    // NOTE: set by the constructor, meaning unknown
        kFlag_40         = 1u << 6,    // NOTE: set by the constructor, meaning unknown
        kFlag_80         = 1u << 7,
        kFlag_PoseDirty  = 1u << 8,    // layers were sampled this frame; the pose needs building
        kFlag_LayerFlag  = 1u << 9,    // handed to the layers' Update
        kFlag_DistanceLod = 1u << 10,  // far away: animate only on the controller's phase
        kFlag_NearHidden = 1u << 11,   // not drawn, but within the cull distance
        kFlag_AlwaysAnimate = 1u << 12,
        kFlag_2000       = 1u << 13,   // NOTE: set by the constructor, meaning unknown
        kFlag_Running    = 1u << 14,
        kFlag_NoLod      = 1u << 15,   // animate even when not drawn and far away
    };

    // A (callback, context) pair registered by a component for the
    // controller's events (+0x44, at most 16).
    struct Listener {
        void* callback = nullptr;
        void* context = nullptr;
    };
    static constexpr uint32_t kMaxListeners = 16;

    // 0x0034BDEC: controller for `object`, registered in the global list and
    // given the next update phase.
    explicit AnimController(ape::world::ApeObj* object);
    // 0x0034BF9C (TODO): releases the rig, the poses, the layers and the
    // per-bone objects and leaves the global list.
    virtual ~AnimController();

    // 0x0034CC34: the layer created for `anim`. With `anim` 0 and no layer
    // named 0, the first layer when there are at least two.
    AnimLayer* FindLayer(uint32_t anim) const;
    // 0x0035133C: creates a layer for `anim`, binds it to the rig and
    // appends it to the layer list.
    AnimLayer* AddLayer(uint32_t anim);
    // 0x0034F690 (TODO): binds the controller to a skeleton (bone objects,
    // per-bone buffers, its own animated skeleton and pose).
    void BindSkeleton(const HkaSkeleton* bones, bool force);
    // 0x00350764: binds the controller (and with `layersToo` every layer)
    // to the skeleton registered as `name`.
    void SetSkeleton(uint32_t name, bool layersToo, bool force);
    // 0x00351898: creates a blend layer for `anim` with the bone weight list
    // named `weightList` (holding its last frame when `holdLastFrame`).
    AnimLayer* AddBlendLayer(uint32_t anim, uint32_t weightList, bool holdLastFrame);
    // 0x00351A40: creates the layers listed by an anim tree.
    void BuildLayers(const struct AnimTree* tree);
    // 0x0034CDB0: the controller animates this frame (running, and drawn,
    // forced, within its cull distance or not a geometry instance).
    bool IsAnimatedThisFrame() const;
    // 0x0034C918: decides whether the layers are sampled this frame
    // (distance LOD, cull distance) and updates them.
    void UpdateLayers(float dt);
    // 0x0034D1A8 (TODO): blends the sampled layers into the object's skeleton.
    void BuildPose(float dt);

    // 0x0034F518 / 0x0034F580: event listeners.
    void AddListener(void* callback, void* context);
    void RemoveListener(void* callback, void* context);

    ape::world::ApeObj* object = nullptr;     // +0x04
    HkaAnimatedSkeleton* rig = nullptr;       // +0x08 the object's animated skeleton (holds a reference)
    HkaPose* pose = nullptr;                  // +0x0C
    uint32_t field_10 = 0;
    void* boneBuffer = nullptr;               // +0x14 (size +0x20)
    ape::world::ApeObj** boneObjects = nullptr;   // +0x18 one per bone (size +0x24)
    uint32_t field_1C = 0;
    uint32_t boneBufferSize = 0;              // +0x20
    uint32_t boneObjectsSize = 0;             // +0x24
    uint16_t flags = 0;                       // +0x28 Flags
    uint8_t phase = 0;                        // +0x2A frames left until the next LOD update
    float lodDistance = 0.0f;                 // +0x2C beyond it, animate on the phase only (< 0: default)
    float cullDistance = 0.0f;                // +0x30 hidden objects animate within it (0 = never)
    uint32_t field_34 = 0;
    uint32_t field_38 = 0;
    AnimEventCallback eventCallback = nullptr;    // +0x3C
    void* eventUserData = nullptr;            // +0x40
    Listener listeners[kMaxListeners] = {};   // +0x44 (a fixed pool on the console)
    uint32_t listenerCount = 0;
    HkaPose* pose2 = nullptr;                 // +0x54 NOTE: second pose, use unknown
    HkaAnimatedSkeleton* rig2 = nullptr;      // +0x58 (holds a reference)
    ape::ApeTArray<AnimLayer*>* layers = nullptr;   // +0x5C created on demand
    void* field_60 = nullptr;                 // +0x60 arrays released by the destructor
    void* field_64 = nullptr;
    void* field_68 = nullptr;
    void* field_6C = nullptr;
    void* field_70 = nullptr;
    void* field_74 = nullptr;                 // +0x74 0xB0-byte block
};

// An anim tree (registry 0x0106BB9C): the layers an Anim component's
// controller starts with.
struct AnimTreeLayer {
    enum Kind : uint32_t {
        kKind_Plain = 0xFF45EF93,   // NOTE: name unknown
        kKind_Blend = 0x19A18BC2,   // NOTE: name unknown
    };
    uint32_t anim = 0;            // +0x00
    uint32_t kind = 0;            // +0x04 Kind
    uint32_t field_08 = 0;
    uint32_t weightList = 0;      // +0x0C bone weight list name (blend layers)
    bool holdLastFrame = false;   // +0x10
};
struct AnimTree {
    uint32_t count = 0;
    AnimTreeLayer* layers = nullptr;
};

// -- module state ---------------------------------------------------------------

// Every live controller (0x0106BC70).
ape::ApeTArray<AnimController*>& AnimControllers();
struct AnimSettings {
    bool enabled = true;          // 0x0106BBE4 animation runs at all
    bool distanceLod = true;      // 0x0106BD64
    uint8_t lodPhases = 4;        // 0x0106BCCC frames between LOD updates. NOTE: start-up value unknown
    float defaultLodDistance = 0.0f;  // 0x0106BD18 (controllers start with it)
    uint8_t nextPhase = 0;        // 0x0106BC6C phase handed to the next controller
};
AnimSettings& Anim();

// 0x0034E2BC: builds the pose of every controller sampled this frame.
void BuildPendingPoses(float dt);
// 0x0034E364: the same for the controllers of `list`.
void BuildPendingPoses(float dt, const ape::ApeTArray<AnimController*>& list);

// Animation clips by name hash (0x0106BBE8).
ape::ApeTSortedMap<void*>& AnimClips();
// 0x0034A398: the clip registered under `name`, or null.
void* FindAnimClip(uint32_t name);

// Anim trees (0x0106BB9C), blend maps (0x0106BBB4) and skeleton bindings
// (0x0106BC14) by name hash; the lookups return null for unknown names.
ape::ApeTSortedMap<void*>& AnimTrees();
ape::ApeTSortedMap<void*>& AnimBlendMaps();
ape::ApeTSortedMap<void*>& AnimSkeletons();
void* FindAnimTree(uint32_t name);        // 0x003482B0
void* FindAnimBlendMap(uint32_t name);    // 0x00348650
void* FindAnimSkeleton(uint32_t name);    // 0x00348ECC
struct BoneWeightList;
BoneWeightList* FindBoneWeightList(uint32_t name);   // 0x003489B0

// 0x0034F600: depth first search below `root` for the object named `name`.
ape::world::ApeObj* FindDescendant(ape::world::ApeObj* root, uint32_t name);

} // namespace anim
} // namespace sly
