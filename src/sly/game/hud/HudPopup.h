// openTIT - Popup HUD element (definition type bit 0x20; vtable 0x00B1F7C8,
// 0xC0 bytes): plays "<prefix>_Intro", loops "<prefix>_Idle" and leaves with
// "<prefix>_Outro"; its anim script follows with a numbered state.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace sly {
namespace hud {

class HudPopup : public HudObject {
public:
    // Anim script state of variant n is kStateBase + n. NOTE: name of the
    // base state unknown.
    static constexpr uint32_t kStateBase = 0x02B81827u;

    // 0x00359CDC.
    explicit HudPopup(HudObjectDef* def);
    // 0x0036EB74 / 0x0036ECDC: hides the object.
    ~HudPopup() override;
    // Slot 2 (0x0036ED28): intro -> idle -> back to hidden once each clip ends.
    void Update(float dt) override;
    // Slot 3 (0x0036EF3C).
    void Refresh() override;
    // Slot 5 (0x0036EAD4): also restores the script state of the variant.
    void SetActive(bool active) override;
    // Slot 6 (0x0036F1C8): show or hide.
    void SetEnabled(bool enabled, bool flag) override;

    // 0x0036F0D0 / 0x0036EFC8.
    void Show();
    void Hide();

    uint32_t field_A4 = 0;           // +0xA4
    uint32_t introAnim = 0;          // +0xA8
    uint32_t idleAnim = 0;           // +0xAC
    uint32_t outroAnim = 0;          // +0xB0
    uint32_t variant = 0;            // +0xB4
    bool active = false;             // +0xB8 bit 31 (definition startShown)
};

} // namespace hud
} // namespace sly
