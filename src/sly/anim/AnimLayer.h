// openTIT - Animation layers of the game's animation controller
// (0x0034F518-0x00354C18).
//
// An animation controller (AnimController.h) owns a list of layers; each
// layer plays one clip on its own Havok animated skeleton through a main
// animation control, optionally with a secondary control blended on top
// (an additive or overlay clip). Layers are named by the hash of the clip
// they were created for. The layer tracks the playback direction of the
// main control and latches when the clip finished, and its Update steps
// the skeleton, prunes eased-out controls, fires the clip's annotations as
// animation events and samples the pose.
#pragma once

#include "sly/anim/AnimHavok.h"

#include "ape/container/ApeTSortedMap.h"

#include <cstdint>

namespace sly {
namespace anim {

class AnimController;

// A named list of per-bone weights (registry 0x0106BBCC), applied to the
// transform track weights of a layer's main control.
struct BoneWeight {
    uint32_t bone = 0;           // +0x00 bone name hash (the part after ':' in the skeleton's names)
    float weight = 0.0f;         // +0x04
    float minWeight = 0.0f;      // +0x08 NOTE: the base layer uses max(weight, minWeight)
    bool children = false;       // +0x0C also weight the bones after it
};
struct BoneWeightList {
    uint32_t count = 0;          // +0x00
    BoneWeight* entries = nullptr;   // +0x04
};
// Bone weight lists by name hash (0x0106BBCC).
ape::ApeTSortedMap<BoneWeightList*>& BoneWeightLists();
// Control listeners by name hash (0x0106BC40).
ape::ApeTSortedMap<void*>& ControlListeners();

// Base layer (vtable 0x00B1F558, 0x2C bytes).
class AnimLayer {
public:
    // Layer flags (+0x0C).
    enum Flags : uint32_t {
        kFlag_Paused       = 1u << 22,   // playback speed 0
        kFlag_Reverse      = 1u << 23,   // playback speed < 0
        kFlag_Forward      = 1u << 24,   // playback speed > 0
        kFlag_JustFinished = 1u << 25,   // finished during the last update
        kFlag_Finished     = 1u << 26,   // finished (latched until replayed)
        kFlag_Looping      = 1u << 27,   // never finishes at the clip end
    };

    AnimLayer(AnimController* owner, uint32_t anim);
    // 0x00353334: releases the secondary pose and the skeleton.
    virtual ~AnimLayer();
    // Slot 2: steps the controls by `dt`; unless `noEvents`, fires the
    // annotations of the main clip and samples the pose when the owner is
    // animated this frame. NOTE: `flag` (owner flags bit 9) is passed on to
    // derived layers; the base ignores it, and a further register argument
    // is unused.
    virtual void Update(float dt, bool flag, bool noEvents);
    // Slot 3: per-bone weights of the main control (null keeps them).
    virtual void SetBoneWeights(const BoneWeightList* list);

    // 0x003535C4: adds a secondary control for `binding` playing at `speed`
    // from `time` (looping endlessly when `loop`), then applies the bone
    // weight list named `weightList` (0 = none).
    void AddSecondaryControl(HkaAnimationBinding* binding, bool loop, float speed, float time,
                             uint32_t weightList);
    // 0x00350860: attaches the control listener named `listener` (0 = the
    // null listener) to the newest control of the skeleton.
    void AttachListener(uint32_t listener);
    // 0x003540BC: adds a listener object to the main control.
    void AddControlListener(void* listener);

    float Duration() const;                   // 0x00353748 main clip length
    float SecondaryDuration() const;          // 0x00354088
    float LocalTime() const;                  // 0x00353780
    void SetLocalTime(float time);            // 0x00354150
    float PlaybackSpeed() const;              // 0x003541B0
    void SetPlaybackSpeed(float speed);       // 0x003541E0 (also the direction flags)
    void SetSecondarySpeed(float speed);      // 0x00354254
    void SetSecondaryMasterWeight(float w);   // 0x00354184
    uint32_t OverflowCount() const;           // 0x00353E88 completed loops
    void SetOverflowCount(uint32_t count);    // 0x00354198
    // 0x00353CC0: eases the main control out at once, or with `remove`
    // takes it off the skeleton.
    void Stop(bool remove);
    // 0x00353DC8: eases the secondary control out at once.
    void StopSecondary();
    // 0x00353EB0 / 0x00353F98: the main / secondary control has finished
    // (eased out, at the clip end in the playing direction unless looping
    // or paused, or with `countLoops` after a completed loop).
    bool IsDone(bool countLoops) const;
    bool IsSecondaryDone(bool countLoops) const;
    // 0x0034CD50: some control of the skeleton is not eased out yet.
    bool HasActiveControls() const;

    uint32_t AnimName() const { return anim; }

    AnimController* owner = nullptr;          // +0x04
    uint32_t anim = 0;                        // +0x08 clip name hash
    uint32_t flags = 0;                       // +0x0C Flags (bits 0-21 not used here)
    float weight = 1.0f;                      // +0x10
    float effectiveWeight = 1.0f;             // +0x14 weight after the layer's own fade
    HkaAnimatedSkeleton* skeleton = nullptr;  // +0x18 (holds a reference)
    HkaPose* pose = nullptr;                  // +0x1C
    HkaAnimationControl* control = nullptr;   // +0x20 main control (owned by the skeleton)
    HkaAnimationControl* secondary = nullptr; // +0x24
    uint32_t field_28 = 0;                    // +0x28

protected:
    // Done test shared by IsDone/IsSecondaryDone: `c` is tested against the
    // main clip's length and the layer's direction flags.
    bool ControlDone(const HkaAnimationControl* c, bool countLoops) const;
    // Eases `c` out with no delay (status "easing out", or "eased out" when
    // already fully faded).
    static void EaseOutNow(HkaAnimationControl* c);
};

// Layer with per-bone weight maps and a short fade when it stops on its last
// frame (vtable 0x00B1F570, 0x6C bytes).
class AnimBlendLayer : public AnimLayer {
public:
    using AnimLayer::AnimLayer;
    ~AnimBlendLayer() override;               // 0x003544AC / 0x00354598
    // 0x003546D0: after the base update, fades the effective weight from 1
    // to 0 over a quarter second once the clip holds its last frame at
    // speed 0 (unless `holdLastFrame`).
    void Update(float dt, bool flag, bool noEvents) override;
    // 0x003547EC: rebuilds the two bone -> weight maps.
    void SetBoneWeights(const BoneWeightList* list) override;

    // 0x0034CCB8 / 0x0034CD04: weight of `bone` in each map, scaled by the
    // effective weight.
    float BoneWeightA(uint32_t bone) const;
    float BoneWeightB(uint32_t bone) const;

    uint32_t field_2C = 0;                    // +0x2C
    const BoneWeightList* weightList = nullptr;   // +0x30 list the maps were built from
    ape::ApeTSortedMap<float> weightsA;       // +0x34 bone index -> weight
    ape::ApeTSortedMap<float> weightsB;       // +0x4C bone index -> min weight
    float fadeTime = 0.0f;                    // +0x64
    bool holdLastFrame = false;               // +0x68
};

// Strips an optional "<prefix>:" from a skeleton bone name and hashes the
// rest (the bone names of the weight lists are hashed that way).
uint32_t HashBoneName(const char* boneName);

} // namespace anim
} // namespace sly
