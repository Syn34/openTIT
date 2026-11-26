// openTIT - Tracker list HUD element (definition type bit 4; vtable
// 0x00B1F838, 0xE0 bytes): keeps a list of tracked entities (dropped once
// they die) and plays intro -> idle -> outro; elements of this kind are
// also in the per-frame update list.
#pragma once

#include "sly/game/hud/HudManager.h"

#include "ape/memory/ApeTArray.h"

namespace sly {
namespace entity { class Entity; }
namespace hud {

class HudTracker : public HudObject {
public:
    // One tracked entity (12 bytes; only the entity is used here).
    struct Entry {
        uint32_t field_00 = 0;
        entity::Entity* entity = nullptr;   // +0x04
        uint8_t field_08 = 0;               // +0x08 (9 bytes are copied when entries move)
    };

    // 0x0035A3A0.
    explicit HudTracker(HudObjectDef* def);
    // 0x00370220 / 0x00370498.
    ~HudTracker() override;
    // Slot 2 (0x003704E8).
    void Update(float dt) override;
    // Slot 3 (0x00370860): a sticky element starts over shown.
    void Refresh() override;
    // Slot 4 (0x00370928): forgets the entries.
    void BeginFrame() override;
    // Slot 5 (0x00370130).
    void SetActive(bool active) override;
    // Slot 6 (0x00370934): nothing.
    void SetEnabled(bool enabled, bool flag) override;
    // 0x00370938: shows or hides the scene object of field `index`.
    void SetTrackerFieldVisible(bool visible, uint32_t index);
    // 0x003709A0: world position of field `index`'s scene object (zero
    // without one).
    ape::Vec3 TrackerFieldPosition(uint32_t index) const;
    // 0x0036901C: sets the label of field `index` (null text keeps it).
    void SetTrackerFieldText(const char* text, uint32_t index);
    // 0x003690F0: shows the list: fields 0/1 get `title`, 2/3 `text`, the
    // entries are copied; `sticky` keeps it up through Refresh.
    void ShowList(const char* title, const char* text, const ape::ApeTArray<Entry>* source, bool sticky);
    // 0x003694A0: ShowList on a possibly missing tracker.
    static void ShowListOf(HudTracker* tracker, const char* title, const char* text,
                           const ape::ApeTArray<Entry>* source, bool sticky);
    // 0x00370B88: draws the marker and the text frames at their scene
    // objects (world callback 0x00370E04).
    void DrawFields();
    static void DrawFieldsCallback(void* unused, void* context);

    ape::ApeTArray<Entry> entries;            // +0xA4
    uint32_t field_B8 = 0;                    // +0xB8
    uint32_t introAnim = 0;                   // +0xBC
    uint32_t idleAnim = 0;                    // +0xC0
    uint32_t outroAnim = 0;                   // +0xC4
    ape::world::ApeObj** fieldObjects = nullptr;   // +0xC8 one per definition field
    ape::world::ApeObj* marker = nullptr;     // +0xCC
    bool sticky = false;                      // +0xD0 bit 31
};

} // namespace hud
} // namespace sly
