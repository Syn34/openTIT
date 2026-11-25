// openTIT - The common HUD element (vtable 0x00B1F6B0, 0x190 bytes): an
// element with text fields, state animations (intro, update, idle, outro),
// a hold time and a text that waits for the next intro. Most element kinds
// derive from it.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace sly {
namespace hud {

class HudElement : public HudObject {
public:
    // 0x0036ABF8 / 0x0036AF54.
    ~HudElement() override;
    // Slot 3 (0x0036BCAC): back to the hidden state, keeping a persistent
    // element shown.
    void Refresh() override;
    // Slot 5 (0x0036AACC): the object, the text field objects and the anim
    // script state ("visibleON"/"visibleOFF").
    void SetActive(bool active) override;
    // Slot 6 (0x0036BF38): disabling plays the outro (`flag` as its first
    // reason); enabling shows a hidden element.
    void SetEnabled(bool enabled, bool flag) override;
    // Slot 7 (0x0036BAA0): slides along the stack and counts the hold time
    // down; when it runs out the element leaves.
    virtual void TickStack(float dt);
    // Slot 8 (0x0036BBC0): once the intro animation and script are done the
    // element idles for its hold time.
    virtual void OnIntroDone();
};

} // namespace hud
} // namespace sly
