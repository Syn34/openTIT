// openTIT - "Press to continue" HUD element (definition type bit 0x40;
// vtable 0x00B1F800, 0xD0 bytes): after its intro it waits for the confirm
// button, plays a sound, opens the linked screen element and continues the
// game once that screen's intro ended.
#pragma once

#include "sly/game/hud/HudManager.h"
#include "sly/game/AudioSystem.h"

namespace sly {
namespace hud {

// Name of the screen the prompt opens (0x00F64CF0). NOTE: value unknown.
extern uint32_t g_promptScreenName;

class HudPrompt : public HudObject {
public:
    static constexpr uint32_t kIntroClip = 0xA5108477u;   // NOTE: names unknown
    static constexpr uint32_t kIdleClip = 0x73C8B956u;
    static constexpr uint32_t kConfirmSound = 0xB106F474u;

    // 0x0035A258.
    explicit HudPrompt(HudObjectDef* def);
    // 0x0036F1FC / 0x0036F3EC.
    ~HudPrompt() override;
    // Slot 2 (0x0036F438).
    void Update(float dt) override;
    // Slot 3 (0x0036F8C4).
    void Refresh() override;
    // Slot 5 (0x0036F9CC).
    void SetActive(bool active) override;
    // Slot 6 (0x0036FAC8): nothing.
    void SetEnabled(bool enabled, bool flag) override;

    ape::world::ApeObj* sceneObjects[3] = {};   // +0xA4 definition names +0xA0..+0xA8
    ape::ApeGuiObject* guiObjects[3] = {};      // +0xB0 definition +0x94..+0x9C, shown when idle
    uint32_t field_BC = 0;
    uint32_t introClip = kIntroClip;            // +0xC0
    uint32_t idleClip = kIdleClip;              // +0xC4
    audio::SoundName* sound = nullptr;          // +0xC8
    int32_t step = 0;                           // +0xCC 0 waiting, 1 confirmed, 2 done
};

} // namespace hud
} // namespace sly
