// openTIT - Timed text HUD element (definition type bit 0x4000; vtable
// 0x00B1F988, 0x1A0 bytes): a common element that appears after a delay and
// then writes a list of lines into its text fields at their times.
#pragma once

#include "sly/game/hud/HudElement.h"

namespace sly {
namespace hud {

class HudSubtitle : public HudElement {
public:
    // Built inline by the element factory (0x0035AC7C).
    HudSubtitle(HudObjectDef* def, const HudTimedLines* lines);
    // 0x00381320 / 0x00381460.
    ~HudSubtitle() override;
    // Slot 2 (0x003814E0).
    void Update(float dt) override;
    // Slot 3 (0x00381758): leaves and starts over.
    void Refresh() override;

    const HudTimedLines* timed = nullptr;   // the definition's line list (+0xA4..+0xAC)
    float delayTimer = 0.0f;     // +0x190
    float lineTime = 0.0f;       // +0x194
    int32_t lineIndex = -1;      // +0x198 last line written
};

} // namespace hud
} // namespace sly
