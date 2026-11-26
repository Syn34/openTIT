// openTIT - HUD element of definition type bit 0x2000 (vtable 0x00B1F950,
// 0x150 bytes; built by 0x00358760): a scrolling list of jobs to replay.
// Six rows ("SlotAName".."SlotFName") show the names of the list entries
// around the selection, a "DESC" field the selected entry's description,
// and a knob slides along a scroll bar. Up/down scroll the list (with a
// "ScrollUp"/"ScrollDn" animation), the accept button starts the selected
// job. The level shell fills the entries (0x003BDD28) before showing it.
//
// The element runs its own copy of the show/hide state machine with the
// states 0 hidden, 1 intro, 2 update, 3 idle (input), 4/5 scroll animations
// and 6 outro; its flag word is +0xFC.
#pragma once

#include "sly/game/hud/HudManager.h"

#include "ape/memory/ApeTArray.h"

namespace ape::world { class ApeObj; }

namespace sly {
namespace hud {

class HudJobList : public HudObject {
public:
    // Flag word (+0xFC).
    enum ListFlags : uint32_t {
        kL_QueuedDown  = 0x00080000,   // bit 19: a down press during a scroll animation
        kL_QueuedUp    = 0x00100000,   // bit 20: an up press during a scroll animation
        kL_ScriptOk    = 0x00800000,   // bit 23: the anim script took the last state
        kL_AnimOk      = 0x01000000,   // bit 24: the last state clip started
        kL_Suppressed  = 0x02000000,   // bit 25: show/hide requests are ignored
        kL_HideReq     = 0x04000000,   // bit 26: hide at the next update
        kL_ShowReq     = 0x08000000,   // bit 27: show at the next update
        kL_LeaveB      = 0x10000000,   // bit 28: destroyed once hidden
        kL_LeaveA      = 0x20000000,   // bit 29: refreshed once hidden
        kL_Shown       = 0x40000000,   // bit 30
        kL_Persistent  = 0x80000000u,  // bit 31: shown again by Refresh
    };
    // Element states (animState >> 1).
    enum ListState : uint32_t {
        kList_Hidden = 0, kList_Intro = 1, kList_Update = 2, kList_Idle = 3,
        kList_ScrollUp = 4, kList_ScrollDown = 5, kList_Outro = 6,
    };
    // One job of the list (20 bytes).
    struct Entry {
        uint32_t jobValue = 0;          // +0x00 value (+0x2D4) of the job record
        const char* name = nullptr;     // +0x04 shown in a row
        const char* description = nullptr;   // +0x08 shown in "DESC"
        bool available = false;         // +0x0C the job can be started
        uint32_t episodeName = 0;       // +0x10 name hash of the job's stage record
    };

    // 0x00358760.
    explicit HudJobList(HudObjectDef* def);
    // 0x0037EE20 / 0x0037F150.
    ~HudJobList() override;
    // Slot 2 (0x0037FAB0).
    void Update(float dt) override;
    // Slot 3 (0x00380A8C).
    void Refresh() override;
    // Slot 5 (0x0037ECC0).
    void SetActive(bool active) override;
    // Slot 6 (0x00380C70).
    void SetEnabled(bool enabled, bool flag) override;

    // 0x0037F1A0: starts the intro.
    void BeginShow();
    // 0x0037F918: shows the element (or restarts its update animation) for
    // `fade` seconds (< 0: the default); `linkedTo`: only once that
    // element's object is hidden. `text` is not used by this kind.
    void ShowTimed(float fade, const char* text, bool flag, HudObject* linkedTo, const uint8_t* color);
    // 0x0037F5E8: plays the outro, or refreshes / queues the element for
    // destruction when it is already hidden.
    void Hide(bool a, bool b);
    // 0x0037F32C: names of the entries in the six rows around the selection.
    void RefreshRowTexts();

    std::uint32_t clips[7] = {};       // +0xC8 clip per state
    uint8_t tint[4] = {};              // +0xE4 label colour (alpha 0: the fields' own)
    ape::world::ApeObj** fieldObjects = nullptr;   // +0xE8 scene object of each field
    float listDefaultFade = 0.0f;      // +0xEC
    float listFadeTime = -1.0f;        // +0xF0 requested time (< 0: the default)
    float listHoldTime = 0.0f;         // +0xF4
    HudObject* listLinked = nullptr;   // +0xF8
    uint32_t listFlags = 0;            // +0xFC ListFlags
    uint32_t selected = 0;             // +0x100 entry under the cursor
    uint32_t cursorRow = 0;            // +0x104 row the selection is shown in
    uint32_t lastScroll = 0;           // +0x108 state of the last scroll animation (0, 4 or 5)
    ape::world::ApeObj* knob = nullptr;       // +0x10C
    ape::world::ApeObj* barTop = nullptr;     // +0x110
    ape::world::ApeObj* barBottom = nullptr;  // +0x114
    ape::world::ApeObj* rows[6] = {};         // +0x118 own copies of the row geometry
    ape::ApeTArray<Entry> entries{20};        // +0x130

private:
    bool Blocked() const { return (listFlags & kL_Suppressed) != 0 || !(listFlags & kL_Shown); }
    // Clip and script state of `state`.
    void PlayListState(uint32_t state, float speed, bool loop);
    bool ListAnimDone() const;
    void SetFieldText(uint32_t fieldName, const char* text);
    // World callbacks (lists 8 and 3): draws the 3D fields at their scene
    // objects, places the row geometry and the scroll bar knob.
    static void DrawFieldsCallback(void* unused, void* context);   // 0x00380EF8
    static void PlaceRowsCallback(void* unused, void* context);    // 0x003812F4
    void DrawFields();                  // 0x00380CF8
    void PlaceRows();                   // 0x00380F28
};

} // namespace hud
} // namespace sly
