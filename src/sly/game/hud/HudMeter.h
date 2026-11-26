// openTIT - HUD meters: the health meter elements (type bit 8, vtable
// 0x00B1F758, 0x100 bytes) and the POW meter (type bit 0x10, vtable
// 0x00B1F790, 0x1A0 bytes).
//
// A meter follows a character's health: a loss waits `holdTime`, then the
// fill drains to the new value over `drainTime`; an empty meter plays its
// outro. The fill is a material whose U scroll the meter sets by hand (0.5
// + value / 2). Meter states (HudObject::animState >> 1): 0 hidden, 1 intro,
// 2 shown, 3 outro.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace agl { struct AglMaterialAnim; }

namespace sly {
namespace hud {

class HudMeter : public HudObject {
public:
    // 0x00364EE4.
    explicit HudMeter(HudObjectDef* def);
    // Nothing of its own (the base releases).
    // @func 0x0036D118 HudMeter::~HudMeter
    // @func 0x0036D23C HudMeter::~HudMeter (deleting)
    ~HudMeter() override = default;
    // Slot 2 (0x0036D350).
    void Update(float dt) override;
    // Slot 3 (0x0036D874): hidden, back in place, full.
    void Refresh() override;
    // Slot 5 (0x0036DAFC): object, frame and fill parts, script variant.
    void SetActive(bool active) override;
    // Slot 6 (0x0036D900): shows (intro) or hides (outro) the meter.
    void SetEnabled(bool enabled, bool flag) override;

    // 0x00364D28: starts the intro at the owner's current health.
    void Show();
    // 0x0036D288: starts the outro unless hidden or leaving.
    void Hide();
    // 0x00364C90: sets the fill of both fill geometries (0..1).
    void SetFill(float fraction);
    // 0x0036DC6C: shows or hides the second bar.
    void SetSecondBarVisible(bool visible);
    // 0x00369680: fill of the second bar (0..1); filling it up starts a
    // flash script on the meter's object.
    void SetSecondFill(float fraction);

    // The character the meter follows: the explicit owner, else the player's
    // character if it is a scripted entity (type bit 1).
    entity::Entity* Target() const;

private:
    // 0x0036D9E4: fill A and the frame follow the object and `showFill`.
    void UpdateFillVisibility();

    int32_t player_ = 0;                        // +0xA4 player block (definition +0x94)
    bool useOwner_ = false;                     // +0xAC bit 7
    uint32_t clips_[4] = {};                    // +0xB0 [0] unused, intro, idle, outro
    ape::world::ApeObj* fillA_ = nullptr;       // +0xC4
    ape::world::ApeObj* fillB_ = nullptr;       // +0xC8
    ape::world::ApeObj* frame_ = nullptr;       // +0xCC
    ape::world::ApeObj* secondFillA_ = nullptr; // +0xD0 second bar (shown with meterFlagsC0 bit 7)
    ape::world::ApeObj* secondFillB_ = nullptr; // +0xD4
    float drainTime_ = 0.0f;                    // +0xE0
    float drainTimer_ = 0.0f;                   // +0xE4
    float drainFrom_ = 0.0f;                    // +0xE8
    float drainTo_ = 0.0f;                      // +0xEC
    float fraction_ = 1.0f;                     // +0xF0 health fraction shown
    float secondFraction_ = 1.0f;               // +0xF4 second bar fill

    friend void UpdateMeterA();
    friend void AddMeterFor(entity::Entity*, uint32_t);
};

class HudPowMeter : public HudObject {
public:
    static constexpr int kPipCount = 10;

    // 0x00364700.
    explicit HudPowMeter(HudObjectDef* def);
    // Frees the hidden part list, then the base releases.
    // @func 0x0036DD30 HudPowMeter::~HudPowMeter
    // @func 0x0036DE88 HudPowMeter::~HudPowMeter (deleting)
    ~HudPowMeter() override = default;
    // Slot 2 (0x0036DED8).
    void Update(float dt) override;
    // Slot 3 (0x0036E184).
    void Refresh() override;
    // Slot 5 (0x0036E750): hiding remembers which parts were visible,
    // showing brings those back.
    void SetActive(bool active) override;
    // Slot 6 (0x0036E2F0).
    void SetEnabled(bool enabled, bool flag) override;
    // Slot 7 (0x0036E868): walks `object`'s subtree; when hiding, every
    // visible part is remembered and hidden.
    virtual void CollectParts(ape::world::ApeObj* object, bool show);

    // 0x003645B0: starts the intro (and the intro global script).
    void Show();
    // 0x0036E210: starts the outro.
    void Hide();
    // 0x00364580: starts the intro global script again.
    void PlayIntroScript();
    // 0x0036E498: fill of both fill geometries from `value` of `max`.
    void SetFill(int32_t value, int32_t max);
    // 0x0036E714: starts the global script of pip `index`.
    void PlayPipScript(int index);
    // 0x0036E9BC: shows the first `count` pips.
    void SetPipCount(int count);
    // 0x0036EA80: POW level; the anim script gets state "Level<n>".
    void SetLevel(int32_t level);

private:
    ape::ApeTArray<ape::world::ApeObj*> hiddenParts_;   // +0xA4
    int32_t level_ = 0;                         // +0xB4
    int32_t player_ = 0;                        // +0xB8
    uint32_t clips_[4] = {};                    // +0xBC [0] unused, intro, idle, outro
    ape::world::ApeObj* fillA_ = nullptr;       // +0xCC
    ape::world::ApeObj* fillB_ = nullptr;       // +0xD0
    ape::world::ApeObj* pips_[kPipCount] = {};  // +0xD4
    uint32_t introScript_ = 0;                  // +0x100
    uint32_t field_104 = 0;                     // +0x104
    uint32_t pipScripts_[kPipCount] = {};       // +0x108
    float holdTime_ = 0.0f;                     // +0x130
    float holdTimer_ = 0.0f;                    // +0x134
    float drainTime_ = 0.0f;                    // +0x138
    float drainTimer_ = 0.0f;                   // +0x13C
    float field_140 = 0.0f;
    float field_144 = 0.0f;
    float fraction_ = 1.0f;                     // +0x148
    ape::Matrix34 savedTransform_{};            // +0x150 object transform once shown
    bool transformSaved_ = false;               // +0x190
};

// 0x00364408: starts global anim script `script` (library 0) for one run.
void PlayGlobalScript(uint32_t script);
// 0x0036E568: the same with its "Level" variable set to `level`.
void PlayGlobalScriptWithLevel(uint32_t script, int32_t level);

// 0x003651F0: builds and registers the meter of definition `name` (null
// unless the definition makes meters).
HudMeter* CreateMeter(uint32_t name);
// 0x00364980: (re)builds the POW meter named by SetPowMeterPrefix.
void UpdatePowMeter();
// 0x00365360: (re)builds meter A for the context name and points it at the
// context owner.
void UpdateMeterA();
// 0x003654C0: (re)builds meter B for the pending screen name.
void UpdateMeterB();
// 0x00369910: a meter of definition `name` (0: the entity definition's
// name) follows `owner`; an owner keeps one meter.
void AddMeterFor(entity::Entity* owner, uint32_t name);
// 0x00369A80: retires `owner`'s meter (destroyed by the next HUD update).
void RemoveMeterOf(entity::Entity* owner);
// 0x00369C30: second bar fill of `owner`'s meter.
void SetSecondFillOf(const entity::Entity* owner, float fraction);
// 0x003697B0: second bar fill of meter B.
void SetMeterBSecondFill(float fraction);

} // namespace hud
} // namespace sly
