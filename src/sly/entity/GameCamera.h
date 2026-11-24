// openTIT - The game camera entity (the parts used outside the camera modes).
#pragma once

#include "sly/entity/Entity.h"

namespace sly {
namespace entity {

// View handed to the renderer and the camera blends.
struct CameraView {
    ape::Matrix34 transform;
    float field_40 = 0.0f;   // camera +0x164 NOTE: field of view (see camera ease code)
    float field_44 = 0.0f;   // camera +0x168
};

class GameCamera : public Entity {
public:
    static constexpr int kMaxTargets = 8;   // NOTE: capacity of the target stack not confirmed

    void GetView(CameraView& out) const;
    void SetAttachParent(Entity* parent) override;
    bool HasTarget(uint32_t target) const;
    // Target pushed just before `target`, or 0 when it is the oldest or absent.
    uint32_t TargetBelow(uint32_t target) const;
    // Pauses or resumes the camera's own animation (layer 0).
    void SetAnimPaused(bool paused);

    uint8_t targetCount = 0;                  // 0x159
    float field_164 = 0.0f;                   // 0x164
    float field_168 = 0.0f;                   // 0x168
    uint32_t targets[kMaxTargets] = {};       // 0x170 stack of target ids, oldest first
    void* cameraAnim = nullptr;               // 0x260 animation controller of the camera rig
};

} // namespace entity
} // namespace sly
