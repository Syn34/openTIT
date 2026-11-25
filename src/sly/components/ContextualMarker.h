// openTIT - <ContextualMarker> component (type 4, vtable 0x00B1C0D0, 0x80
// bytes): a piece of geometry (a button prompt, an arrow) attached to a
// locator, bone or object of the owner and scaled with the camera distance.
#pragma once

#include "sly/components/Components.h"

namespace ape::world { class ApeObj; }

namespace sly {
namespace components {

class ContextualMarkerComponent : public EntityComponent {
public:
    explicit ContextualMarkerComponent(const ContextualMarkerComponentDesc* desc);
    ~ContextualMarkerComponent() override;
    // Queues the marker for the instance render pass while its geometry is
    // shown (slot 6).
    void Update(float dt) override;
    // Places the geometry at the attach point and scales it with the camera
    // distance (slot 8).
    void RenderInstance() override;
    void SetOption(bool enabled) override;
    void SetVisible(bool visible, bool immediate) override;

    ape::Vec4 attachRot{};                   // 0x20 quaternion
    ape::Vec3 attachOffset{};                // 0x30 (only xyz copied)
    ape::Vec3 attachCameraOffset{};          // 0x40
    void* locator = nullptr;                 // 0x50
    int32_t bone = -1;                       // 0x54
    ape::world::ApeObj* attachObject = nullptr;  // 0x58
    ape::world::ApeObj* geometry = nullptr;  // 0x5C owned reference
    float scaleMin = -1.0f;                  // 0x60
    float scaleMax = -1.0f;                  // 0x64
    float scaleDistMin = -1.0f;              // 0x68
    float scaleDistMax = -1.0f;              // 0x6C
    float scaleFactorFOV = 0.0f;             // 0x70
    bool hasScaleFactorFOV = false;          // 0x74
    bool applyRotation = false;              // 0x75
};

} // namespace components
} // namespace sly
