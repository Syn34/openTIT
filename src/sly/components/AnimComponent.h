// openTIT - <Anim> component (type 2, vtable 0x00B1C880, 0x1C4 bytes).
//
// Connects an entity to its animation set: on construction it takes an anim
// set slot (AnimClipManager) for its set and every parent set, it keeps the
// entity's animation controller and the names of up to eight animation
// layers with their last known state, and it owns the sounds the animations
// attached to the entity (stopped when they end or the component goes away).
#pragma once

#include "sly/entity/Entity.h"
#include "sly/game/AudioSystem.h"

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::anim { class AnimSet; struct AnimClipDef; }

namespace sly {
namespace anim { class AnimController; class AnimLayer; }
namespace components {

struct AnimComponentDesc;

class AnimComponent : public entity::EntityComponent {
public:
    static constexpr int kMaxLayers = 8;
    static constexpr uint32_t kMaxSounds = 8;

    // State kept per animation layer (0x28 bytes).
    struct LayerSlot {
        uint32_t anim = 0;            // +0x00 layer (clip) name hash
        const ape::anim::AnimClipDef* clip = nullptr; // +0x04
        uint32_t field_08 = 0;
        float time = 0.0f;            // +0x0C local time at the last refresh
        uint8_t field_10[0x10] = {};
        bool done = false;            // +0x20 finished at the last refresh
        uint32_t loops = 0;           // +0x24 completed loops at the last refresh
    };
    // A sound started by an animation (+0x180).
    struct AttachedSound {
        audio::VoiceId voice = 0;
        uint32_t tag = 0;             // NOTE: caller supplied, meaning unknown
    };

    // 0x00260464.
    explicit AnimComponent(const AnimComponentDesc* desc);
    // 0x00297900 / 0x00297B54: releases the anim set slots and stops the sounds.
    ~AnimComponent() override;
    // Slot 4 (0x002977B4): releases the anim set slots and stops the sounds.
    void ResetState() override;
    // Slot 6 (0x00297E00): drops finished sounds (skipped for a zero step).
    void Update(float dt) override;
    // Slot 7 (0x00297E38): nothing.
    void PostUpdate(float dt) override;

    // 0x002980AC: takes the owner's controller (building its layers from the
    // anim tree when it has none yet) and records up to eight layer names.
    void SetController(anim::AnimController* controller);
    // 0x00297E3C: name hash of the anim set of the current slot (0 = none).
    uint32_t CurrentSetName() const;
    // 0x00298B44: the current anim set (or one of its parents) has `clip`.
    bool HasClip(uint32_t clip) const;
    // 0x00298874: the layer slot of `anim` holds a looping clip.
    bool IsLoopingLayer(uint32_t anim) const;
    // 0x00298D98: records the state of the layer `anim` in its slot.
    void RefreshLayerSlot(uint32_t anim);

    // Queries and controls by layer (0 = default layer); a missing layer reads as
    // done / time 0.
    void StopSecondary(uint32_t anim);                       // 0x00298748
    bool IsDone(uint32_t anim, bool countLoops) const;       // 0x0029879C
    bool IsSecondaryDone(uint32_t anim, bool countLoops) const; // 0x00298808
    void ResetLoops(uint32_t anim);                          // 0x002988EC
    float Progress(uint32_t anim) const;                     // 0x00298948 time / length
    float LayerTime(bool normalized, uint32_t anim) const;   // 0x002989D8

    // TODO(0x00298C10 AnimComponent::AttachSound): plays a sound at the
    // owner (sound parameter block positions are not modelled yet) and keeps
    // the voice in `sounds`.

    uint32_t animSetName = 0;                // +0x18 "set"
    void* animTree = nullptr;                // +0x1C "tree"
    uint32_t currentSlot = 0;                // +0x20 anim set slot handle of the set itself
    anim::AnimController* controller = nullptr;  // +0x24
    LayerSlot layers[kMaxLayers];            // +0x28
    ape::ApeTArray<uint32_t> setSlots;       // +0x168 slot handles of the set and its parents
    float field_17C = 1.0f;                  // +0x17C
    AttachedSound sounds[kMaxSounds];        // +0x180
    uint32_t soundCount = 0;                 // +0x1C0

private:
    anim::AnimLayer* Layer(uint32_t anim) const;
    LayerSlot* FindSlot(uint32_t anim);
    const LayerSlot* FindSlot(uint32_t anim) const;
    void ReleaseSetSlots();
    void StopAllSounds();
    // 0x00297BA8: forgets the voices whose sound request has ended.
    void UpdateSounds();
};

} // namespace components
} // namespace sly
