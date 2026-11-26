// openTIT - HUD element of definition type bit 0x8000 (vtable 0x00B1F9C8,
// 0x220 bytes; built by 0x00357DC4). The gadget and targeting components and
// the job manager show and hide it. It runs its own copy of the show/hide
// state machine of the common element, with its fields at other offsets
// and its flag word at +0x214.
//
// It is the radar: the constructor names its parts "RadarBlipPlayer_NN",
// "RadarBlipEnemy_NN", "RadarBlipGoal_NN", "RadarBlipGoalEdge_NN",
// "RadarBlipHideout", "RadarBlipFollow", "RadarBlipBottle_NN" and
// "RadarBlipSafe_01". Its per-frame update (0x00382308, 7.5 KB) is not
// ported yet.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace sly {
namespace hud {

class HudType8000 : public HudObject {
public:
    // Flag word (+0x214).
    enum Flags8000 : uint32_t {
        kF_OutroB      = 0x00100000,   // bit 20: handed to Hide as `b` when the hold time ends
        kF_OutroA      = 0x00200000,   // bit 21: handed to Hide as `a` when the hold time ends
        kF_ScriptOk    = 0x00400000,   // bit 22: the anim script took the last state
        kF_AnimOk      = 0x00800000,   // bit 23: the last state clip started
        kF_Blocked     = 0x01000000,   // bit 24: NOTE: show/hide requests are ignored (like bit 25)
        kF_Suppressed  = 0x02000000,   // bit 25
        kF_LeaveB      = 0x10000000,   // bit 28
        kF_LeaveA      = 0x20000000,   // bit 29
        kF_Shown       = 0x40000000,   // bit 30
        kF_Persistent  = 0x80000000u,  // bit 31: stays shown through Refresh unless leaving
        kF_Bit26       = 0x04000000,
        kF_Bit27       = 0x08000000,
    };

    HudType8000() = default;
    // 0x00357DC4: copies of the definition's blip instances.
    explicit HudType8000(HudObjectDef* def);
    // 0x00381AA8 / 0x00381DF8: hidden, the parts released.
    ~HudType8000() override;

    // Slot 3 (0x003842D0): back to hidden at its stack origin; a pending
    // "leave B" request queues it for destruction.
    void Refresh() override;
    // Slot 5 (0x003817E0): shows or hides its object; hiding also hides
    // every part.
    void SetActive(bool active) override;
    // Slot 6 (0x003846E8): shown again (when not already shown) or hidden.
    void SetEnabled(bool enabled, bool flag) override;
    // Slot 7 (0x00384088): slides to its stack offset and counts the hold
    // time down; when it runs out the element leaves.
    virtual void UpdateHold(float dt);
    // Slot 8 (0x003841B0): once the intro clip and script are done the
    // element goes to its idle state for the hold time.
    virtual void FinishIntro();

    // 0x003686D0: plays the clip and script state of `state` (1 intro,
    // 2 update, 3 idle, 4 outro).
    void PlayState8000(int state, bool loop);
    // 0x00368770: starts the intro.
    void BeginShow8000();
    // 0x00368870: shows the element for `fade` seconds (< 0: the default);
    // `linkedTo`: only once that element's object is hidden.
    void ShowTimed8000(float fade, bool flag, HudObject* linkedTo);
    // 0x00368AC8: leaves (outro), or queues the element to be destroyed /
    // refreshes it when it is idle with a reason to go.
    void Hide8000(bool a, bool b);
    // 0x003689D8 / 0x00368D78: show / hide a possibly missing element.
    static void ShowElement(HudType8000* element);
    static void HideElement(HudType8000* element);
    // 0x00381E48: plays the alert intro (`intro`) or outro clip.
    void PlayAlert(bool intro);

    bool Blocked() const { return (stateWord & (kF_Blocked | kF_Suppressed)) != 0; }

    uint32_t clips[5] = {};          // +0xA4 clip per state
    uint32_t alertClips[2] = {};     // +0xB8 "_AlertIntro", "_AlertOutro"
    float defaultFade8000 = 0.0f;    // +0xC0
    float fadeTime8000 = 0.0f;       // +0xC4 requested time (< 0: the default)
    float holdTime8000 = 0.0f;       // +0xC8 time left in the update/idle state
    HudObject* linked8000 = nullptr; // +0xCC
    ape::world::ApeObj* background = nullptr;   // +0xD0 definition +0xA0 (not copied)
    // Blips (own copies of the definition's instances).
    ape::world::ApeObj* markers[9] = {};     // +0xD4 players
    ape::world::ApeObj* rowA[12] = {};       // +0xF8 enemies
    ape::world::ApeObj* rowB[12] = {};       // +0x128 goals
    ape::world::ApeObj* rowC[12] = {};       // +0x158 goals beyond the radar's edge
    ape::world::ApeObj* part188 = nullptr;   // +0x188 hideout
    ape::world::ApeObj* part18C = nullptr;   // +0x18C followed entity
    ape::world::ApeObj* slots[32] = {};      // +0x190 clue bottles
    ape::world::ApeObj* part210 = nullptr;   // +0x210 safe
    uint32_t stateWord = 0;          // +0x214 Flags8000

private:
    void HideParts();
};

} // namespace hud
} // namespace sly
