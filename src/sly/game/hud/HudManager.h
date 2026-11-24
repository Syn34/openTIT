// openTIT - HUD object registry (sly HUD module, 0x003568E0-0x00369D98).
//
// Every HUD element (meters, timers, text boxes, radar...) registers under
// its name hash in a sorted map (0x0106C228). Typed lookups check the
// element's type bits (HudObject::typeFlags) and return null on a mismatch.
// Only the lookups and the small state helpers are ported so far; the HUD
// element classes themselves are not.
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace sly {
namespace hud {

// HudObject::typeFlags (+0x04). NOTE: only kHud_Object is named from use;
// the others are the element classes' own bits.
enum HudTypeFlags : uint32_t {
    kHud_Object  = 0x00000001,   // every registered element
    kHud_Type2   = 0x00000002,   // NOTE: the pause/countdown screen element (goal timer test)
    kHud_Type4   = 0x00000004,
    kHud_Type40  = 0x00000040,
    kHud_Type200 = 0x00000200,
    kHud_Type400 = 0x00000400,
    kHud_Type800 = 0x00000800,
    kHud_Type1000 = 0x00001000,
    kHud_Type2000 = 0x00002000,
    kHud_Type4000 = 0x00004000,
    kHud_Type8000 = 0x00008000,
    kHud_Type10000 = 0x00010000,
    kHud_Type20000 = 0x00020000,
};

// Screen corner an element is stacked in (HudObjectDef::anchor).
enum HudAnchor : int32_t {
    kAnchor_TopLeft     = 0,   // NOTE: corner names follow the mirroring in AnchorMirror()
    kAnchor_TopRight    = 1,
    kAnchor_BottomRight = 2,
    kAnchor_BottomLeft  = 3,
    kAnchor_Count       = 4,   // four stacks; 4 itself is not stacked
    kAnchor_None        = 5,
};

// Definition of an element (shared by its instances).
struct HudObjectDef {
    int32_t anchor = kAnchor_None;   // +0x54 HudAnchor
    float offset[3] = {};            // +0x60 space the element takes in its stack
};

class HudObject {
public:
    virtual ~HudObject() = default;
    // slot 3 (+0x0C)
    virtual void Refresh() {}
    // slot 5 (+0x14)
    virtual void SetActive(bool active) { (void)active; }

    // 0x00363A3C: shows the element with a text (fade time, -1 = default).
    void ShowText(float fade, const char* text, bool flag);
    // 0x00362F1C / 0x00362B40: plays the element's intro / outro.
    void PlayIntro(bool a, bool b);
    void PlayOutro(bool a, bool b);
    // 0x0036325C: intro or outro unless suppressed.
    void Show(bool a, bool intro, bool b);
    // 0x003626E8: moves the element to a stack offset.
    void SetStackOffset(const float offset[3], bool immediate);
    // 0x003628B4: offset of the element in its anchor stack (sum of the
    // elements before it, mirrored for the corner).
    void StackOffset(float out[3]) const;
    // 0x003629D8: leaves its anchor stack and moves the elements after it up.
    void LeaveStack();

    bool Suppressed() const { return (stateFlags & kState_Suppressed) != 0; }

    enum StateFlags : uint32_t {
        kState_Suppressed = 0x02000000,   // bit 25: show/hide requests are ignored
        kState_Leaving    = 0x04000000,   // bit 26: outro requested
        kState_Bit27      = 0x08000000,
        kState_OutroB     = 0x10000000,   // bit 28
        kState_OutroA     = 0x20000000,   // bit 29
        kState_Shown      = 0x40000000,   // bit 30
    };

    uint32_t typeFlags = 0;        // +0x04 HudTypeFlags
    HudObjectDef* def = nullptr;   // +0x08
    void* gui = nullptr;           // +0x18 GUI instance of the element
    uint32_t animState = 0;        // +0xA0 (state = value >> 1; 0 idle, 4 NOTE: looping)
    bool field_A4 = false;         // +0xA4
    uint32_t updateFlags = 0;      // +0xD0 bit 31 cleared by ResetUpdateFlags()
    float targetValue = 0.0f;      // +0xD8
    float shownValue = 0.0f;       // +0xDC
    uint32_t stateFlags = 0;       // +0x18C StateFlags
};


// Module state (the globals around 0x0106C228).
struct HudManagerState {
    ape::ApeTSortedMap<HudObject*> objects;      // 0x0106C228 every element by name
    ape::ApeTSortedMap<HudObject*> updateList;   // 0x0106C258 elements updated per frame
    uint32_t contextValue = 0;                   // 0x00F64CB4
    uint32_t contextName = 0;                    // 0x0106C270
    bool pendingScreen = false;                  // 0x0106C274
    uint32_t pendingScreenName = 0;              // 0x0106C278
    uint32_t powMeterName = 0;                   // 0x0106C27C
    uint32_t activeType200Name = 0;              // 0x00F64D18 preferred type-0x200 element
    uint32_t fallbackType200Name = 0;            // 0x00F64D10
    HudObject* meterA = nullptr;                 // 0x00F64CB8
    HudObject* meterB = nullptr;                 // 0x00F64CBC
    ape::ApeTArray<HudObject*> stacks[kAnchor_Count];   // 0x0106C2D4 elements per corner
};

HudManagerState& Hud();

// GUI instance +0xA4 bit 15: its animation is playing. NOTE: the GUI module
// is not ported; stubbed in HudManager.cpp.
bool GuiIsPlaying(const void* gui);

// Lookups (null when the name is 0, unknown, or of another type).
HudObject* Find(uint32_t name);                    // 0x003568E0 kHud_Object
HudObject* FindType2(uint32_t name);               // 0x00365E30
HudObject* FindType4(uint32_t name);               // 0x00368FA0
HudObject* FindType40(uint32_t name);              // 0x00365EAC
HudObject* FindType400(uint32_t name);             // 0x00365BA4
HudObject* FindType800(uint32_t name);             // 0x00368E2C
HudObject* FindType1000(uint32_t name);            // 0x00368EA8
HudObject* FindType2000(uint32_t name);            // 0x00368F24
HudObject* FindType4000(uint32_t name);            // 0x00368DB0
HudObject* FindType8000(uint32_t name);            // 0x00368A48
HudObject* FindType10000(uint32_t name);           // 0x0035B908
HudObject* FindType20000(uint32_t name);           // 0x00365B28
// 0x00365C24: the active type-0x200 element (preferred name, then fallback).
HudObject* FindActiveType200();

void SetSuppressed(HudObject* object, bool suppressed);              // 0x00367518
void ShowTextByName(float fade, uint32_t name, const char* text, bool flag);   // 0x0036735C
void ShowByName(uint32_t name, bool a, bool intro, bool b);          // 0x0036742C
void ShowType4000ByName(uint32_t name, bool a, bool intro, bool b);  // 0x00367B28
int CountElementsAtAnchor(int32_t anchor);                           // 0x00367134
// 0x00362830: mirrors an offset for a corner.
void AnchorMirror(float out[3], int32_t anchor, const float in[3]);
void SetContext(uint32_t value, uint32_t name);                      // 0x00369D00
void SetContextByString(uint32_t value, const char* name);           // 0x00369D14
void RequestScreen(bool skip, const char* name);                     // 0x0036960C
void SetPowMeterPrefix(const char* prefix);                          // 0x00369CAC
void SnapMeterA();                                                   // 0x00369D58
void SnapMeterB();                                                   // 0x00369658
void ResetUpdateFlags();                                             // 0x00369500
bool AnyUpdatingElementVisible();                                    // 0x003695A0

} // namespace hud
} // namespace sly
