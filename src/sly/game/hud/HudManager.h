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
#include "sly/game/Tween.h"

namespace ape::world { class ApeObj; }
namespace ape { class ApeGuiObject; }
namespace sly { namespace anim { class AnimController; } }

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
    uint32_t name = 0;               // +0x44
    int32_t anchor = kAnchor_None;   // +0x54 HudAnchor
    int32_t priority = 0;            // +0x70 order in the anchor stack (ascending)
    float offset[3] = {};            // +0x60 space the element takes in its stack
    struct TextFieldDef {            // 0x20 bytes
        uint8_t field_00[0x14] = {};
        uint32_t introScript = 0;    // +0x14 GUI script played on the field with the intro
        uint32_t outroScript = 0;    // +0x18 ... with the outro
        uint32_t field_1C = 0;
    };
    TextFieldDef* textFieldDefs = nullptr;   // +0x94
    uint32_t textFieldCount = 0;     // +0x98 text fields of the element
};

// Anim script states of the element states (0x00F64F88): "NONE", "Intro",
// "OnUpdate", "Idle", "Outro".
extern const uint32_t kHudStateNames[5];

// A text field of an element (16 bytes, array at HudObject +0x148).
struct HudTextField {
    uint32_t field_00 = 0;
    ape::ApeGuiObject* object = nullptr;   // +0x04 shown with the element
    uint32_t field_08 = 0;                 // +0x08 non-zero: show `object`
    ape::ApeGuiObject* label = nullptr;    // +0x0C the label holding the text
};

// Base of the HUD elements (vtable 0x00B1F678). NOTE: the console's base is
// 0xC0 bytes; the fields from +0xD0 on (meter values, the state word +0x18C)
// belong to the larger element classes and are kept here until those
// classes are ported.
class HudObject {
public:
    // User tag the element writes into its object (ApeObj +0x114, with the
    // element in +0x118).
    static constexpr uint32_t kObjectTag = 0x7DF841DCu;

    // 0x0036A098 / 0x0036A1C0: ends the anim script, releases the object and
    // the animation controller.
    virtual ~HudObject();
    // Slot 2 (0x0036A2FC): animates the element and places its object at
    // its stack offset.
    virtual void Update(float dt);
    // Slot 3 (0x0036A410): deactivated and back at its stack origin.
    virtual void Refresh();
    // Slot 5 (0x0036A470): shows or hides the element's object.
    virtual void SetActive(bool active);
    // Slot 6 (0x00772944, other module): bit 0 of +0xA0.
    virtual void SetHidden(bool hidden, bool flag)
    {
        (void)flag;
        animState = (animState & ~1u) | (hidden ? 1u : 0u);
    }

    // 0x00362BEC: queues the element to be hidden at the end of the frame.
    void HideNow();
    // 0x003632C8: sets the text of text field `index` (null text keeps it).
    void SetFieldText(const char* text, uint32_t index);
    // 0x0036336C: scale of text field `index` (unit scale without one).
    void FieldScale(float out[3], uint32_t index) const;
    // Name the element is registered under (its own or its definition's).
    uint32_t RegisteredName() const { return name ? name : (def ? def->name : 0); }

    // 0x00363A3C: shows the element with a text. Timed: (re)starts its
    // update animation for `fade` seconds (< 0: the default), else the text
    // is kept for the next intro. `linked`: show only once that element's
    // object is hidden. `color` (4 bytes) tints the text.
    void ShowText(float fade, const char* text, bool timed, bool flag = false, HudObject* linked = nullptr,
                  const uint8_t* color = nullptr);
    // 0x00363890: the timed part of ShowText.
    void ShowTimed(float fade, const char* text, bool flag, HudObject* linked, const uint8_t* color);
    // 0x003636EC: starts the intro (object, text fields and their scripts).
    void BeginShow();
    // 0x00362D28: plays the animation and script state of `state` (1 intro,
    // 2 update, 3 idle, 4 outro; see kHudStateNames).
    void PlayState(int state, bool loop);
    // 0x0036341C: joins its anchor stack in priority order and moves the
    // elements after it.
    void EnterStack();
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
        kState_Bit18      = 0x00040000,   // NOTE: set by ShowBusyElement, meaning unknown
        kState_ScriptOk   = 0x00800000,   // bit 23: the anim script took the last state
        kState_AnimOk     = 0x01000000,   // bit 24: the last state animation started
        kState_Bit27      = 0x08000000,
        kState_OutroB     = 0x10000000,   // bit 28
        kState_OutroA     = 0x20000000,   // bit 29
        kState_Shown      = 0x40000000,   // bit 30
    };

    uint32_t typeFlags = 0;        // +0x04 HudTypeFlags
    HudObjectDef* def = nullptr;   // +0x08
    uint32_t name = 0;             // +0x0C (0: the definition's name)
    uint32_t field_10 = 0;         // +0x10 from the definition (+0x4C)
    void* animScript = nullptr;    // +0x14 ApeAnimScriptInstance
    ape::world::ApeObj* object = nullptr;   // +0x18 the element's scene object
    anim::AnimController* animController = nullptr;   // +0x1C
    ape::Vec3 offset{};            // +0x20 current stack offset
    Vec3Tween offsetTween;         // +0x30 slides `offset` to a new stack position
    ape::Vec3 baseOffset{};        // +0x90 screen position of the element (definition +0x80)
    uint32_t animState = 0;        // +0xA0 bit 0 hidden; state = value >> 1 (0 idle, 4 NOTE: looping)
    char text[0x80] = {};          // +0xA4 text kept for the next intro
    uint32_t stateAnims[5] = {};   // +0x128 animation clip per state
    uint32_t altIdleAnim = 0;      // +0x13C replaces the idle clip when set
    uint32_t altOutroAnim = 0;     // +0x140 replaces the outro clip when set
    uint8_t textColor[4] = {};     // +0x144
    float defaultFade = 0.0f;      // +0x14C
    float fadeTime = 0.0f;         // +0x150
    float holdTime = 0.0f;         // +0x154
    HudObject* linked = nullptr;   // +0x158 shown once this element's object is hidden
    float animSpeed = 1.0f;        // +0x188
    uint32_t updateFlags = 0;      // +0xD0 bit 31 cleared by ResetUpdateFlags()
    float targetValue = 0.0f;      // +0xD8
    float shownValue = 0.0f;       // +0xDC
    HudTextField* textFields = nullptr;   // +0x148 def->textFieldCount entries
    uint32_t stateFlags = 0;       // +0x18C StateFlags
};


// Module state (the globals around 0x0106C228).
struct HudManagerState {
    ape::ApeTSortedMap<HudObject*> objects;      // 0x0106C228 every element by name
    ape::ApeTSortedMap<HudObject*> hideList;     // 0x0106C240 elements to hide at the end of the frame
    ape::ApeTSortedMap<HudObject*> updateList;   // 0x0106C258 elements updated per frame
    bool allHidden = false;                      // 0x00F64CC4
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

// Name of the clue bottle counter element (0x00F64CD4, "BottleCounter").
extern uint32_t g_bottleCounterName;
// Element shown by ShowBusyElement (0x00F64D44, hash 0x39A21479). NOTE: its
// name is unknown; the game application shows it around saving/loading.
extern uint32_t g_busyElementName;
// 0x003671B0 / 0x00367290: shows / hides that element.
void ShowBusyElement();
void HideBusyElement();
// 0x003679C8: resets an element and shows it with a text (see ShowText).
void ReshowText(float fade, HudObject* object, const char* text, bool timed);

// The element's object is visible (ApeObj flags bit 15).
bool ObjectVisible(const ape::world::ApeObj* object);
// Elements queued by HideNow (0x0106C240), hidden at the end of the frame.
// NOTE: lives in HudManagerState.
// 0x00365A54: hides or shows every element (slot 6); showing again also
// refreshes the anchor stacks (0x0035B470, not ported).
void SetAllHidden(bool hidden, bool flag);

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

void SetSuppressed(HudObject* object, bool suppressed);
// Null-safe text field helpers.
void SetFieldText(HudObject* object, const char* text, uint32_t index);       // 0x00367530
void SetFieldTextByName(uint32_t name, const char* text, uint32_t index);     // 0x003675D8
// Glyph animation of the first text field: play mode by name / frame time
// in seconds (< 0: 3 s).
void SetFieldAnimMode(HudObject* object, uint32_t modeName);                  // 0x003676DC
void SetFieldAnimPeriod(HudObject* object, float seconds);                    // 0x00367758              // 0x00367518
void ShowTextByName(float fade, uint32_t name, const char* text, bool timed, bool flag = false,
                    HudObject* linked = nullptr, const uint8_t* color = nullptr);   // 0x0036735C
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
