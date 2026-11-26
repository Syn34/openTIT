// openTIT - HUD element of definition type bit 0x400 (vtable 0x00B1F8A8,
// 0xF0 bytes; built by 0x003599C4): the selection wheel for costumes or a
// character's upgrades. Each page of the wheel stands for one item; left
// and right turn the pointer to the next item that can be chosen, the
// "Info" field shows the page's text and locked pages show their "locked"
// object. While shown it holds the player's input (AppInputHeld).
//
// Show/hide state machine: 0 hidden, 1 intro, 2 update, 3 idle (input), 4
// outro; flag word +0xD8.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace ape::world { class ApeObj; }

namespace sly {
namespace hud {

class HudCostumeWheel : public HudObject {
public:
    enum WheelFlags : uint32_t {
        kW_InputWasHeld = 0x00800000,  // bit 23: AppInputHeld before the wheel took it
        kW_Hiding       = 0x01000000,  // bit 24: Hide ran since the last show
        kW_ScriptOk     = 0x02000000,  // bit 25
        kW_AnimOk       = 0x04000000,  // bit 26
        kW_Suppressed   = 0x08000000,  // bit 27
        kW_HideReq      = 0x10000000,  // bit 28: hide at the next update
        kW_ShowReq      = 0x20000000,  // bit 29: show at the next update
        kW_Shown        = 0x40000000,  // bit 30
        kW_Persistent   = 0x80000000u, // bit 31
    };
    // Turn angle of a page (12 bytes): eased towards the target each frame.
    struct PageAngle {
        float current = 0.0f;
        float target = 0.0f;
        float field_08 = 0.0f;
    };
    // Kinds of wheel (definition +0xD8).
    static constexpr uint32_t kKind_Costume = 0x63C971C0u;   // "Costume"
    static constexpr uint32_t kKind_Bentley = 0x1D8C2817u;   // "Bentley"
    static constexpr uint32_t kKind_Murray = 0x88397820u;    // "Murray"
    static constexpr uint32_t kKind_DF0611F9 = 0xDF0611F9u;  // NOTE: a third character, name unknown

    // 0x003599C4.
    explicit HudCostumeWheel(HudObjectDef* def);
    // 0x003747F4 / 0x00374A8C.
    ~HudCostumeWheel() override;
    // Slot 2 (0x00374AF0): requests, page angles, the pulse, label colours,
    // left/right page selection and the show/hide state machine.
    void Update(float dt) override;
    // Slot 3 (0x003767C8).
    void Refresh() override;
    // Slot 5 (0x00374704).
    void SetActive(bool active) override;
    // Slot 6 (0x00376954).
    void SetEnabled(bool enabled, bool flag) override;

    // 0x00367C20: starts the intro on the page of the item in use.
    void BeginShow();
    // 0x00368360: shows the wheel (or restarts its update animation) and
    // takes the player's input.
    void Show();
    // 0x003684B8: plays the outro and gives the input back.
    void Hide();
    // 0x00376998 / 0x003769C4: item of the current page / of page `page`
    // (0 out of range); 0x00376A08: number of pages.
    uint32_t CurrentItem() const;
    uint32_t PageItem(int page) const;
    int PageCount() const;
    // The item can be chosen on this wheel.
    bool ItemAvailable(uint32_t item) const;

    ape::world::ApeObj* glow = nullptr;      // +0xA4 object 0x6D1B6072 (NOTE: name unknown)
    ape::world::ApeObj* pointer = nullptr;   // +0xA8 level instance turned to the page
    int32_t page = 0;                        // +0xAC current page
    int32_t previousPage = 0;                // +0xB0
    uint32_t clips[5] = {};                  // +0xB4 clip per state
    float wheelHoldTime = 0.0f;              // +0xC8
    HudObject* wheelLinked = nullptr;        // +0xCC
    PageAngle* angles = nullptr;             // +0xD0 one per page
    uint32_t pointerTarget = 0;              // +0xD4 object the pointer follows
    uint32_t wheelFlags = 0;                 // +0xD8 WheelFlags
    ape::world::ApeObj** fieldObjects = nullptr;   // +0xDC
    ape::world::ApeObj* glowObject = nullptr;      // +0xE0 definition +0xD4
    float pulseTimer = 0.0f;                 // +0xE4
    float pulse = 0.0f;                      // +0xE8 degrees the glow is turned by

private:
    // Next page from `from` in direction `step` (+1 / -1) whose item can be
    // chosen; `from` itself when none.
    int FindAvailablePage(int from, int step) const;
    void PlayWheelState(uint32_t state, bool loop);
    void SetFieldText(uint32_t fieldName, const char* text);
    void SetFieldsVisible();
    static void DrawFieldsCallback(void* unused, void* context);   // 0x00376C20
    void DrawFields();                       // 0x00376A20
    // 0x00375C60 (world callback 0x00376C4C): turns the pointer to the
    // current page, shows each page's available or locked object and turns
    // the spokes by their page angles.
    void UpdatePointer();
    static void UpdatePointerCallback(void* unused, void* context);
    // 0x00376598 (world callback 0x00376C70): turns the glow object (else
    // the element's object) about Y by the pulse angle.
    void TurnGlow();
    static void TurnGlowCallback(void* unused, void* context);
};

} // namespace hud
} // namespace sly
