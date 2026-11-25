// openTIT - <tracker> component (type 0x26, vtable 0x00B1D100, 0x38 bytes):
// runs a script that follows how far and in which direction the player is,
// switching its states with the player's gadget mode.
#pragma once

#include "sly/components/Components.h"

namespace ape { class ApeAnimScriptInstance; class ApeAnimScriptVar; }

namespace sly {
namespace components {

class TrackerComponent : public EntityComponent {
public:
    explicit TrackerComponent(const TrackerComponentDesc* desc);
    ~TrackerComponent() override;
    // TODO(0x002CC018 TrackerComponent::Update): feeds the range/angle to the
    // script.
    void SetOption(bool enabled) override;

    // Gadget mode classes the script states are chosen by.
    enum Mode : int32_t { kMode_None = 0, kMode_Gadget = 1, kMode_Binocucom = 2 };

    entity::Entity* player = nullptr;            // 0x18 NOTE: set by Update
    ape::ApeAnimScriptInstance* script = nullptr; // 0x1C
    ape::ApeAnimScriptVar* valueVar = nullptr;   // 0x20 float variable 0x4DE86539
    float minRange = 0.0f;                       // 0x24
    float maxRange = 1.0f;                       // 0x28
    float minAngle = 0.0f;                       // 0x2C radians
    float maxAngle = 0.0f;                       // 0x30 radians
    int32_t mode = kMode_None;                   // 0x34
};

} // namespace components
} // namespace sly
