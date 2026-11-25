// openTIT - Screen HUD element (definition type bit 2; vtable 0x00B1F720,
// 0xE0 bytes): intro -> idle -> outro clips on its scene object, six named
// sub-objects that the definition can hide, and an optional lock that keeps
// it on screen through resets.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace sly {
namespace hud {

class HudScreen : public HudObject {
public:
    // Names of the six sub-objects (hashes; NOTE: names unknown).
    static constexpr uint32_t kSubObjects[6] = {0x0E272E73u, 0x92B78972u, 0xBA018A9Bu,
                                                0x92A4B2F9u, 0x5CC0B718u, 0x47DA3E5Cu};

    // 0x0035A5B0.
    explicit HudScreen(HudObjectDef* def);
    // 0x0036C400 / 0x0036C574.
    ~HudScreen() override;
    // Slot 2 (0x0036C5BC).
    void Update(float dt) override;
    // Slot 3 (0x0036C818): nothing while locked.
    void Refresh() override;
    // Slot 5 (0x0036CA0C).
    void SetActive(bool active) override;
    // Slot 6 (0x0036CA70): nothing.
    void SetEnabled(bool enabled, bool flag) override;

    // 0x0036C898: hides the sub-objects when the definition says so
    // (definition byte +0x9C).
    void HideSubObjects();

    uint32_t field_A4 = 0;            // +0xA4
    uint32_t introAnim = 0;           // +0xA8
    uint32_t updateAnim = 0;          // +0xAC never set here
    uint32_t idleAnim = 0;            // +0xB0
    uint32_t outroAnim = 0;           // +0xB4
    HudTextField* fields = nullptr;   // +0xB8 copies of the definition's fields
    ape::world::ApeObj* subObjects[6] = {};   // +0xBC
    bool locked = false;              // +0xD4
};

} // namespace hud
} // namespace sly
