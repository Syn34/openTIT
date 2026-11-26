// openTIT - The HUD element kinds of definition type bits 0x800, 0x1000 and
// 0x10000. They show per-episode content (their constructors and SetActive
// look up level instances from tables indexed by the episode's time period:
// Prologue, Japan, West, Ice Age, England, Arabia, ...). Their purposes are
// not identified beyond that, so they keep their type bits as names.
//
// Each runs its own copy of the show/hide state machine (flag word +0xFC
// for 0x800 / 0x1000, +0x3C0 for 0x10000). The 0x1000 kind is complete;
// the 0x800 update / show helper and the 0x10000 constructor are TODO.
#pragma once

#include "sly/game/hud/HudManager.h"

#include "ape/memory/ApeTArray.h"

namespace ape::world { class ApeObj; }

namespace sly {
namespace hud {

// Flag word bits shared by the 0x800 and 0x1000 kinds (+0xFC).
enum EpisodeListFlags : uint32_t {
    kE_QueuedDown = 0x00080000,   // bit 19: a down press during a scroll animation
    kE_QueuedUp   = 0x00100000,   // bit 20: an up press during a scroll animation
    kE_ScriptOk   = 0x00800000,   // bit 23
    kE_AnimOk     = 0x01000000,   // bit 24
    kE_Suppressed = 0x02000000,   // bit 25
    kE_HideReq    = 0x04000000,   // bit 26: hide at the next update
    kE_ShowReq    = 0x08000000,   // bit 27: show at the next update
    kE_LeaveB     = 0x10000000,   // bit 28: destroyed once hidden
    kE_LeaveA     = 0x20000000,   // bit 29
    kE_Shown      = 0x40000000,   // bit 30
    kE_Persistent = 0x80000000u,  // bit 31
};

// Type bit 0x800 (vtable 0x00B1F8E0, 0x1B0 bytes; built by 0x00359284): a
// scrolling list of six rows with two extra objects per row and a per-episode
// instance (0x00F64E1C) shown with the element.
class HudType800 : public HudObject {
public:
    // 0x00359284.
    explicit HudType800(HudObjectDef* def);
    // 0x00376F88 / 0x0037726C.
    ~HudType800() override;
    // Slot 2. TODO(0x00378540).
    void Update(float dt) override;
    // Slot 3 (0x0037965C).
    void Refresh() override;
    // Slot 5 (0x00376C98).
    void SetActive(bool active) override;
    // Slot 6 (0x003798D8).
    void SetEnabled(bool enabled, bool flag) override;
    // 0x003780E0: shows the list (or restarts its update animation) and
    // rebuilds its entries: every upgrade outside a job, else the available
    // ones with the new ones first, each with the icon of its character.
    void ShowTimed(float fade, bool flag, HudObject* linkedTo, const uint8_t* color);
    // 0x0037744C: name and price of the upgrade in each of the six rows.
    void RefreshRows();
    // 0x003772C0: starts the intro.
    void BeginShow();
    // 0x00377D18: plays the outro (clearing the "new" mark of the upgrades
    // the list showed), or refreshes / queues the element when hidden.
    void Hide(bool a, bool b);

    uint32_t clips[7] = {};            // +0xC8 (Intro, OnUpdate, Idle, ScrollUp, ScrollDn, Outro)
    uint8_t tint[4] = {};              // +0xE4
    ape::world::ApeObj** fieldObjects = nullptr;   // +0xE8
    float listDefaultFade = 0.0f;      // +0xEC
    float listFadeTime = -1.0f;        // +0xF0
    float listHoldTime = 0.0f;         // +0xF4
    HudObject* listLinked = nullptr;   // +0xF8
    uint32_t listFlags = 0;            // +0xFC EpisodeListFlags
    uint32_t selected = 0;             // +0x100
    uint32_t cursorRow = 0;            // +0x104
    uint32_t field_108 = 0;            // +0x108
    ape::world::ApeObj* objects[5] = {};     // +0x10C definition +0x9C..+0xAC
    ape::ApeTArray<ape::world::ApeObj*> entryObjects{4};   // +0x120 released with the entries
    ape::ApeTArray<uint8_t> entryFlags{1};   // +0x134
    ape::world::ApeObj* rows[6] = {};        // +0x148 own copies of the row geometry
    ape::world::ApeObj* rowExtraA[6] = {};   // +0x160 objects 0x00F64D88
    ape::world::ApeObj* rowExtraB[6] = {};   // +0x178 objects 0x00F64DA0
    ape::ApeTArray<uint32_t> entries{4};     // +0x190

private:
    void ReleaseEntryObjects();
};

// Type bit 0x1000 (vtable 0x00B1F918, 0x1E0 bytes; built by 0x00358B64):
// an episode selector with four row copies and per-episode objects.
class HudType1000 : public HudObject {
public:
    // 0x00358B64.
    explicit HudType1000(HudObjectDef* def);
    // 0x0037A93C / 0x0037AC20.
    ~HudType1000() override;
    // Slot 2 (0x0037CCA8): requests, label colours, the scroll / accept
    // state machine (accept travels to the episode's hideout level) and the
    // description of the selected episode.
    void Update(float dt) override;
    // Slot 3 (0x0037DC70).
    void Refresh() override;
    // Slot 5 (0x0037A58C).
    void SetActive(bool active) override;
    // Slot 6 (0x0037DE50).
    void SetEnabled(bool enabled, bool flag) override;
    // 0x0037AC70: starts the intro.
    void BeginShow();
    // 0x0037C7E0: plays the outro, or refreshes / queues the element.
    void Hide(bool a, bool b);
    // 0x0037CB10: shows the element (or restarts its update animation);
    // the rows are refreshed.
    void ShowTimed(float fade, const char* text, bool flag, HudObject* linkedTo, const uint8_t* color);
    // 0x0037AE00: fills the title, completion and collectible rows from the
    // episode records (both parts summed for a two-part episode).
    void RefreshRows();
    // 0x0037DED8: draws the 3D text frames at their scene objects (world
    // callback 0x0037E0D8, list 8).
    void DrawFields();
    static void DrawFieldsCallback(void* unused, void* context);
    // 0x0037E108: places the row copies, the per-episode objects of the two
    // title slots and the visible rows, and the scroll knob (world callback
    // 0x0037EC94, list 3).
    void PlaceRows();
    static void PlaceRowsCallback(void* unused, void* context);

    uint32_t clips[7] = {};            // +0xC8
    uint8_t tint[4] = {};              // +0xE4
    ape::world::ApeObj** fieldObjects = nullptr;   // +0xE8
    float listDefaultFade = 0.0f;      // +0xEC
    float listFadeTime = -1.0f;        // +0xF0
    float listHoldTime = 0.0f;         // +0xF4
    HudObject* listLinked = nullptr;   // +0xF8
    uint32_t listFlags = 0;            // +0xFC EpisodeListFlags
    uint32_t selected = 0;             // +0x100 episode under the cursor (0..5)
    uint32_t field_104 = 0;            // +0x104 NOTE: counts down scrolls started from rest; never read here
    uint32_t cursorRow = 0;            // +0x108
    uint32_t lastScroll = 0;           // +0x10C state of the last finished scroll (4 up, 5 down, 0 none)
    ape::world::ApeObj* objects[4] = {};     // +0x110 definition +0x9C..+0xA8
    ape::world::ApeObj* rows[4] = {};        // +0x120 own copies of the row geometry
    ape::world::ApeObj* episodeA[6] = {};    // +0x130 instances 0x00F64E58
    ape::world::ApeObj* episodeB[6] = {};    // +0x148 instances 0x00F64E70
    ape::world::ApeObj* episodeC[6] = {};    // +0x160 instances 0x00F64EA0
    ape::world::ApeObj* episodeD[6] = {};    // +0x178 instances 0x00F64EE8
    uint32_t titleB = 0;               // +0x190 episode whose name is in field "Title b"
    uint32_t titleA = 1;               // +0x194 episode whose name is in field "Title A"
    ape::world::ApeObj* copiesA[6] = {};     // +0x198 copies of instance 0xEF3B431E
    ape::world::ApeObj* copiesB[6] = {};     // +0x1B0 copies of instance 0x04DABC6F
    ape::world::ApeObj* copiesC[6] = {};     // +0x1C8 copies of instance 0x15AA444A

private:
    bool ListAnimDone() const;
    // Clip (at double speed) and script state of scroll state `state`.
    void PlayScroll(uint32_t state);
};

// Type bit 0x10000 (vtable 0x00B1FA08, 0x3D0 bytes): the map / status
// screen ("_Status", "_Map" clips, a sprite map framed by four instances,
// one blip per area). TODO(0x00356BA0): its 4.6 KB constructor is not
// ported; the element is built with the base fields only.
class HudType10000 : public HudObject {
public:
    explicit HudType10000(HudObjectDef* def);

    // +0x3C0 flag word. Bit 20 is set when the HUD is reset with its
    // request flag (ResetHudElements). NOTE: meaning unknown.
    uint32_t mapFlags = 0;
};

} // namespace hud
} // namespace sly
