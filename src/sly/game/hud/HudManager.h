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
namespace ape::xml { class XmlElement; }
namespace sly { namespace anim { class AnimController; } }
namespace sly { namespace entity { class Entity; } }

#include <cstdint>

namespace sly {
namespace hud {

// HudObject::typeFlags (+0x04). NOTE: only kHud_Object is named from use;
// the others are the element classes' own bits.
enum HudTypeFlags : uint32_t {
    kHud_Object  = 0x00000001,   // every registered element
    kHud_Type2   = 0x00000002,   // NOTE: the pause/countdown screen element (goal timer test)
    kHud_Type4   = 0x00000004,
    kHud_Type8   = 0x00000008,   // health meter (HudMeter)
    kHud_Type10  = 0x00000010,   // POW meter (HudPowMeter)
    kHud_Type20  = 0x00000020,   // popup element (HudPopup)
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
    kAnchor_TopLeft     = 0,   // "TL" (definition attribute 0x0026E1FE)
    kAnchor_TopRight    = 1,   // "TR"
    kAnchor_BottomLeft  = 2,   // "BL"
    kAnchor_BottomRight = 3,   // "BR"
    kAnchor_Count       = 4,   // four stacks; 4 itself is not stacked
    kAnchor_None        = 5,
};

// One line of the list (0x28 bytes, definition +0xA4).
struct HudTimedLine {
    uint32_t field = 0;          // +0x00 text field it goes to
    float time = 0.0f;           // +0x04 seconds after the delay
    char text[0x20] = {};        // +0x08
};
struct HudTimedLines {
    HudTimedLine* lines = nullptr;   // definition +0xA4
    int32_t count = 0;               // definition +0xA8
    float delay = 0.0f;              // definition +0xAC
};

// Definition of an element (shared by its instances). Built by the
// definition loader (LoadHudDefinitions); the arrays it points to are owned
// and freed with it.
struct HudObjectDef {
    HudObjectDef() = default;
    ~HudObjectDef();
    HudObjectDef(const HudObjectDef&) = delete;
    HudObjectDef& operator=(const HudObjectDef&) = delete;

    char defName[0x20] = {};         // +0x04
    char animPrefix[0x20] = {};      // +0x24 base of the element's clip names (empty: defName)
    uint32_t name = 0;               // +0x44
    ape::world::ApeObj* templateObject = nullptr;   // +0x48 the element's scene object (shared or cloned)
    uint32_t field_4C = 0;           // +0x4C lifetime anim script of the elements (HudObject::field_10)
    uint32_t typeMask = 0;           // +0x50 element classes the definition can build (HudTypeFlags)
    int32_t anchor = kAnchor_None;   // +0x54 HudAnchor
    int32_t priority = 0;            // +0x70 order in the anchor stack (ascending)
    ape::Vec3 baseOffset{};          // +0x80 screen position (zero: the default position)
    enum ObjectFlags : uint8_t {
        kObject_Clone = 0x80,        // each element gets its own copy of the object
        kObject_Lod   = 0x40,        // geometry objects get render LOD 13. NOTE: meaning assumed
    };
    uint8_t objectFlags = 0;         // +0x90 ObjectFlags
    uint8_t startShown = 0;          // +0x91 (popup elements)
    float offset[3] = {};            // +0x60 space the element takes in its stack
    struct TextFieldDef {            // 0x20 bytes
        enum Flags : uint32_t {
            kField_Clone       = 0x80000000u,   // the element clones the template
            kField_PlayerColor = 0x40000000u,   // the label takes the player character's HUD colour
        };
        uint8_t color[4] = {};       // +0x00 label colour (used when its alpha is not zero)
        uint32_t objectName = 0;     // +0x04 scene object of the field (in the element's hierarchy)
        ape::ApeGuiObject* templateObject = nullptr;   // +0x08 GUI object of the field
        ape::ApeGuiObject* framePath = nullptr;        // +0x0C names the frame inside a clone
        ape::ApeGuiObject* labelPath = nullptr;        // +0x10 names the label inside the frame
        uint32_t introScript = 0;    // +0x14 GUI script played on the field with the intro
        uint32_t outroScript = 0;    // +0x18 ... with the outro
        uint32_t field_1C = 0;
    };
    TextFieldDef* textFieldDefs = nullptr;   // +0x94
    uint32_t textFieldCount = 0;     // +0x98 text fields of the element
    float defaultFade = 0.0f;        // +0x9C hold time of timed texts
    // Byte +0x9C of screen element definitions, which keep a flag where the
    // other kinds keep defaultFade: hide the screen's sub-objects.
    bool hideSubObjects = false;
    uint8_t elementFlags = 0;        // +0xA0 bit 0: the element starts shown and stays (persistent)
    // Prompt definitions (type bit 0x40) keep three GUI objects (+0x94..
    // +0x9C) and three scene object names (+0xA0..+0xA8) where the other
    // kinds keep the members above.
    ape::ApeGuiObject* promptGui[3] = {};
    uint32_t promptObjects[3] = {};
    // Tracker list definitions (type bit 4) keep 16-byte field records at
    // +0x94 (count +0x98) and the name of their marker object at +0x9C.
    struct TrackerFieldDef {
        uint32_t objectName = 0;                 // +0x00
        ape::ApeGuiObject* object = nullptr;     // +0x04 shown with the element ("parentFrame2D")
        ape::ApeGuiObject* textFrame = nullptr;  // +0x08 "textFrame2D"
        ape::ApeGuiObject* label = nullptr;      // +0x0C "text2D"
    };
    TrackerFieldDef* trackerFields = nullptr;
    uint32_t trackerMarkerName = 0;
    // Job list definitions (type bits 0x1000 / 0x2000, +0x94..+0xC0) overlay
    // the members above. NOTE: the 0x1000 kind is assumed to share it.
    struct ListFieldDef {            // 0x1C bytes
        uint32_t name = 0;           // +0x00 field name ("SlotAName", "DESC", ...)
        uint8_t color[4] = {};       // +0x04 label colour (used when its alpha is not zero)
        uint32_t objectName = 0;     // +0x08 scene object the field follows
        ape::ApeGuiObject* object = nullptr;     // +0x0C shown with the element
        ape::ApeGuiObject* object3d = nullptr;   // +0x10 drawn at its scene object (world callback)
        ape::ApeGuiObject* label = nullptr;      // +0x14 the label holding the text
        uint32_t flags = 0;          // +0x18 bit 31: the label takes the player's HUD colour
    };
    struct ListDef {
        uint32_t rowCount = 0;       // +0x94 rows whose geometry is placed (at most 6)
        uint32_t cursorRow = 0;      // +0x98 row the selection starts on
        uint32_t knobName = 0;       // +0x9C scroll bar knob
        uint32_t barTopName = 0;     // +0xA0 knob position of the first entry
        uint32_t barBottomName = 0;  // +0xA4 knob position of the last entry
        uint32_t field_A8 = 0;
        uint32_t rowGeometry = 0;    // +0xAC geometry instance copied for every row
        const uint32_t* rowLocators = nullptr;   // +0xB0 per row: object placing the row's copy
        ListFieldDef* fields = nullptr;          // +0xB4
        uint32_t fieldCount = 0;     // +0xB8
        float defaultFade = 0.0f;    // +0xBC
        uint8_t flags = 0;           // +0xC0 bit 0: persistent (starts shown)
    } list;
    // Costume / upgrade wheel definitions (type bit 0x400, +0x94..+0xD8).
    struct WheelPageDef {            // 0x38 bytes
        uint32_t spokeName = 0;      // +0x00 scene object of the page's spoke
        uint32_t availableName = 0;  // +0x04 shown when the page's item can be chosen
        uint32_t lockedName = 0;     // +0x08 shown when it cannot
        uint32_t pointerName = 0;    // +0x0C object the pointer follows on this page
        uint32_t item = 0;           // +0x10 costume / upgrade the page stands for
        uint32_t field_14 = 0;       // +0x14 attribute 0xE4D43099
        char text[0x20] = {};        // +0x18 shown in the "Info" field
    };
    struct WheelFieldDef {           // 0x1C bytes
        uint8_t color[4] = {};       // +0x00
        uint32_t objectName = 0;     // +0x04
        uint32_t name = 0;           // +0x08 field name ("Info", ...)
        ape::ApeGuiObject* object = nullptr;     // +0x0C
        ape::ApeGuiObject* object3d = nullptr;   // +0x10
        ape::ApeGuiObject* label = nullptr;      // +0x14
        uint32_t flags = 0;          // +0x18 bit 31: player colour
    };
    struct WheelDef {
        WheelPageDef* pages = nullptr;   // +0x94
        int32_t pageCount = 0;       // +0x98
        uint32_t pointerInstance = 0;    // +0x9C level instance of the pointer
        uint32_t pointerName = 0;    // +0xA0 object the pointer starts on
        ape::Vec3 axis{};            // +0xB0 axis the pointer turns about
        float pageAngle = 0.0f;      // +0xC0 degrees between two pages
        WheelFieldDef* fields = nullptr;     // +0xC4
        uint32_t fieldCount = 0;     // +0xC8
        float pulsePeriod = 1.0f;    // +0xCC
        float pulseAmplitude = 1.0f; // +0xD0
        uint32_t glowName = 0;       // +0xD4
        uint32_t kind = 0;           // +0xD8 "Costume", "Bentley", "Murray" or 0xDF0611F9
    } wheel;
    // Hideout menu definitions (type bit 0x200, +0x94..+0xD4).
    struct MenuFieldDef {            // 0x10 bytes
        uint32_t objectName = 0;     // +0x00 scene object the field follows
        ape::ApeGuiObject* object = nullptr;     // +0x04 "parentFrame2D"
        ape::ApeGuiObject* object3d = nullptr;   // +0x08 "textFrame2D", drawn at the scene object
        ape::ApeGuiObject* label = nullptr;      // +0x0C "text2D"
    };
    struct MenuPageDef {             // 0x10 bytes
        uint32_t name = 0;           // +0x00
        MenuFieldDef* fields = nullptr;   // +0x04
        uint32_t fieldCount = 0;     // +0x08
        uint8_t animA = 0;           // +0x0C transition clip set (0..2)
        uint8_t animB = 0;           // +0x0D
    };
    struct MenuDef {
        MenuPageDef* pages = nullptr;    // +0x94
        uint32_t pageCount = 0;      // +0x98
        uint32_t backButton = 0;     // +0x9C element showing the "back" prompt
        uint32_t selectButton = 0;   // +0xA0 element showing the "select" prompt
        uint32_t texture = 0;        // +0xA4 sprite texture of the map
        uint32_t corners[4] = {};    // +0xAC (stride 8) level instances framing the map
        uint32_t childName = 0;      // +0xD4 child of the element's object
        uint8_t mapColor[4] = {};    // +0xA8 attribute 0x4C2E77FD
        uint8_t cornerColors[4][4] = {};   // +0xB0 (stride 8) colour of each corner point
        float field_CC = 1.0f;       // +0xCC attribute 0xEA68DF12
        float field_D0 = 1.0f;       // +0xD0 attribute 0xC9037FF5
    } menu;
    // Radar definitions (type bit 0x8000, +0x94..+0xBC): level instances
    // the radar copies for its blips.
    struct RadarDef {
        float defaultFade = 0.0f;    // +0x94
        uint8_t flags = 0;           // +0x98 bit 0: persistent
        uint32_t playerBlip = 0;     // +0x9C (9 copies)
        uint32_t background = 0;     // +0xA0 used as is
        uint32_t enemyBlip = 0;      // +0xA4 (12 copies)
        uint32_t goalBlip = 0;       // +0xA8 (12 copies)
        uint32_t goalEdgeBlip = 0;   // +0xAC (12 copies)
        uint32_t hideoutBlip = 0;    // +0xB0
        uint32_t followBlip = 0;     // +0xB4
        uint32_t bottleBlip = 0;     // +0xB8 (32 copies)
        uint32_t safeBlip = 0;       // +0xBC
        float field_C0 = 0.0f;       // +0xC0 attribute 0x81B883B4
        float field_C4 = 0.0f;       // +0xC4 attribute 0xEDF0A684
        uint8_t colors[3][4] = {};   // +0xC8 attributes 0x894AC0DB, 0xAFE4553D, 0x62E42FD6
    } radar;
    // Type bit 0x800 definitions (+0x98..+0xC4): like the job list's, with
    // one more object name and everything after it four bytes further on.
    struct List800Def {
        uint32_t rowCount = 0;       // +0x94
        uint32_t cursorRow = 0;      // +0x98
        uint32_t objectNames[5] = {};    // +0x9C..+0xAC
        uint32_t rowGeometry = 0;    // +0xB0
        const uint32_t* rowLocators = nullptr;   // +0xB4
        ListFieldDef* fields = nullptr;          // +0xB8
        uint32_t fieldCount = 0;     // +0xBC
        float defaultFade = 0.0f;    // +0xC0
        uint8_t flags = 0;           // +0xC4 bit 0: persistent
        bool flagC5 = false;         // +0xC5 attribute 0x379040C0
    } list800;
    // Timed text definitions (type bit 0x4000): the lines (+0xA4..+0xAC).
    HudTimedLines timedLines;
    // Meter definitions (type bit 8, +0x94..+0xC0) overlay the members above.
    struct MeterDef {
        int32_t player = 0;          // +0x94 local player whose character the meter follows
        uint8_t useOwner = 0;        // +0x98 follow an explicit owner instead
        uint32_t fillA = 0;          // +0x9C geometry with the fill material (scrolled by the value)
        uint32_t fillB = 0;          // +0xA0 second fill geometry
        uint32_t frame = 0;          // +0xA4 shown with fill A
        uint8_t showFill = 0;        // +0xA8 bit 0: fill A and the frame are shown
        uint32_t extra = 0;          // +0xAC fill geometry shown with `showExtra`
        uint32_t extra2 = 0;         // +0xB0 NOTE: looked up but never used
        uint8_t showExtra = 0;       // +0xB4
        float holdTime = 0.0f;       // +0xB8 delay before a loss drains the meter
        float drainTime = 0.0f;      // +0xBC duration of the drain (0: at once)
        uint8_t healthMeter = 0;     // +0xC0 clip names get the "HealthMeter" suffix
    } meter;
    // POW meter definitions (type bit 0x10, +0x94..+0x128).
    struct PowMeterDef {
        int32_t player = 0;          // +0x94
        uint32_t fillA = 0;          // +0x98
        uint32_t fillB = 0;          // +0x9C
        uint32_t pips[10] = {};      // +0xA0 one geometry per POW level ("icon%d")
        uint32_t sounds[10] = {};    // +0xC8 "sound%d"
        uint32_t field_F0 = 0;       // +0xF0 attribute 0x00273D2D
        uint32_t introScript = 0;    // +0xF4 global anim script started with the intro
        uint32_t field_F8 = 0;       // +0xF8
        uint32_t pipScripts[10] = {};   // +0xFC global anim script per pip ("addBitScript%d")
        float holdTime = 0.0f;       // +0x124 NOTE: kept like the meter's; unused by the POW meter
        float drainTime = 0.0f;      // +0x128
    } pow;
};

// Anim script states of the element states (0x00F64F88): "NONE", "Intro",
// "OnUpdate", "Idle", "Outro".
extern const uint32_t kHudStateNames[5];

// A text field of an element (16 bytes, array at HudObject +0x148).
struct HudTextField {
    ape::world::ApeObj* sceneObject = nullptr;   // +0x00 TextFieldDef::objectName
    ape::ApeGuiObject* object = nullptr;   // +0x04 shown with the element
    ape::ApeGuiObject* field_08 = nullptr; // +0x08 frame; `object` is only shown when set
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

    HudObject() = default;
    // 0x0035695C: element of `def` starting in state `state`; takes (or
    // clones) the definition's object and gives it an animation controller.
    HudObject(uint32_t state, HudObjectDef* def);
    // 0x0036A098 / 0x0036A1C0: ends the anim script, releases the object and
    // the animation controller.
    virtual ~HudObject();
    // Slot 2 (0x0036A2FC): animates the element and places its object at
    // its stack offset.
    virtual void Update(float dt);
    // Slot 3 (0x0036A410): deactivated and back at its stack origin.
    virtual void Refresh();
    // Slot 4 (empty in the base): start of a frame (tracker lists forget
    // their entries).
    // @func 0x00772940 HudObject::BeginFrame
    virtual void BeginFrame() {}
    // Slot 5 (0x0036A470): shows or hides the element's object.
    virtual void SetActive(bool active);
    // Slot 6: bit 0 of +0xA0 (set by the constructor).
    // @func 0x00772944 HudObject::SetEnabled
    virtual void SetEnabled(bool enabled, bool flag)
    {
        (void)flag;
        animState = (animState & ~1u) | (enabled ? 1u : 0u);
    }

    // 0x00362BEC: queues the element to be hidden at the end of the frame.
    void HideNow();
    // 0x003632C8: sets the text of text field `index` (null text keeps it).
    void SetFieldText(const char* text, uint32_t index);
    // 0x0036336C: scale of text field `index` (unit scale without one).
    void FieldScale(float out[3], uint32_t index) const;
    // 0x0036BFAC: shows or hides the scene object of text field `index`
    // (and its children).
    void SetFieldVisible(bool visible, uint32_t index);
    // 0x0036C014: text of text field `index` (null without a label).
    const char* FieldText(uint32_t index) const;
    // 0x0036C06C: world position of text field `index`'s scene object (zero
    // without one).
    ape::Vec3 FieldPosition(uint32_t index) const;
    // 0x0036C0E0: while updating or idle, the hold time starts again.
    void RestartHold();
    // 0x0036C124: replaces the clip of the idle (3, `loop` sets bit 19) or
    // outro (4) state.
    void SetStateAnim(int state, uint32_t clip, bool loop);
    // 0x0036A4C0: sets a state of the element's anim script.
    void SetScriptState(uint32_t state);
    // 0x0035BD08: starts the element's lifetime anim script (definition
    // +0x4C) with "camera" and "this" bound.
    void StartLifetimeScript();
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
        kState_Bit19      = 0x00080000,   // NOTE: loop the alternative idle clip
        kState_Bit20      = 0x00100000,
        kState_Bit21      = 0x00200000,   // handed to PlayIntro as `b` when the hold time ends
        kState_Bit22      = 0x00400000,   // handed to PlayIntro as `a` when the hold time ends
        kState_Bit27      = 0x08000000,
        kState_OutroB     = 0x10000000,   // bit 28
        kState_OutroA     = 0x20000000,   // bit 29
        kState_Shown      = 0x40000000,   // bit 30
        kState_Persistent = 0x80000000u,  // bit 31: Refresh keeps the element shown. NOTE: name assumed
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
    ape::Vec3 springTarget{};      // +0x160 position the spring pulls the object to
    ape::Vec3 springVelocity{};    // +0x170
    float springStiffness = 0.0f;  // +0x180
    float springDamping = 0.0f;    // +0x184
    float animSpeed = 1.0f;        // +0x188
    // Meter elements: owner (+0xA8, overlaying the text of other kinds) and
    // the flag byte +0xC0 (bit 5 switches the meter's script variant).
    entity::Entity* meterOwner = nullptr;
    uint8_t meterFlagsC0 = 0;
    uint32_t updateFlags = 0;      // +0xD0 bit 31 cleared by ResetUpdateFlags()
    float meterHoldTime = 0.0f;    // +0xD8 meters: delay before a loss drains the meter
    float meterHoldTimer = 0.0f;   // +0xDC meters: delay left (SnapMeter* restart it)
    HudTextField* textFields = nullptr;   // +0x148 def->textFieldCount entries
    uint32_t stateFlags = 0;       // +0x18C StateFlags
};


// Module state (the globals around 0x0106C228).
struct HudManagerState {
    ape::ApeTSortedMap<HudObject*> objects;      // 0x0106C228 every element by name
    ape::ApeTSortedMap<HudObject*> hideList;     // 0x0106C240 elements to hide at the end of the frame
    ape::ApeTSortedMap<HudObject*> updateList;   // 0x0106C258 elements updated per frame
    bool allEnabled = false;                     // 0x00F64CC4
    ape::ApeTArray<HudObject*> suspended;        // 0x0106C2C0 elements SuspendHud hid (a refresh drops them)
    bool hudSuspended = false;                   // 0x0106C3C0
    HudObject* element394 = nullptr;             // 0x0106C394 element 0xBC38D74A (0x00F64D28; NOTE: name unknown)
    HudObject* coinCounter = nullptr;            // 0x0106C398 "CoinCounter" (0x00F64CDC)
    bool flagF64CC8 = false;                     // 0x00F64CC8 NOTE: cleared when the elements are built
    ape::ApeTSortedMap<HudObjectDef*> defs;      // 0x0106C210 element definitions by name
    entity::Entity* contextEntity = nullptr;     // 0x00F64CB4 owner the next meter A follows
    uint32_t contextName = 0;                    // 0x0106C270
    bool pendingScreen = false;                  // 0x0106C274
    uint32_t pendingScreenName = 0;              // 0x0106C278
    uint32_t powMeterName = 0;                   // 0x0106C27C
    uint32_t activeType200Name = 0;              // 0x00F64D18 preferred type-0x200 element
    uint32_t fallbackType200Name = 0;            // 0x00F64D10
    HudObject* meterA = nullptr;                 // 0x00F64CB8
    HudObject* meterB = nullptr;                 // 0x00F64CBC
    HudObject* powMeter = nullptr;               // 0x00F64CC0
    ape::ApeTArray<HudObject*> meters;           // 0x0106C280 every meter element with an owner
    ape::ApeTArray<HudObject*> retiredMeters;    // 0x0106C290 destroyed by the next update
    // 0x0106C2A8 colour styles of the definition file (HudColorStyle records,
    // freed at unload).
    ape::ApeTSortedMap<void*> defBuffers;
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
// 0x00365A54: enables or disables every element (slot 6); disabling also
// drops the text override.
void SetAllEnabled(bool enabled, bool flag);

// A temporary restyling of an element's first label (0x0106C330).
struct HudTextOverride {
    // Phases of the count-up (+0x78).
    enum Phase : int32_t {
        kPhase_FlyIn = 0,     // `owner`'s object flies along the curve to the counter
        kPhase_CountUp = 1,   // the counter label counts up, restyled
        kPhase_Show = 2,      // the label is back to normal; the counter shows a while
    };
    // +0x00 control points of the fly-in curve (cubic Bezier, rows 0-3).
    ape::Matrix34 transform{};
    uint8_t savedColor[4] = {};         // +0x40 label colour to restore
    ape::Vec3 savedScale{};             // +0x50 label scale to restore (zero: nothing saved)
    const uint8_t* profileIndex = nullptr;   // +0x60 profile whose coin count the counter reaches
    HudObject* owner = nullptr;         // +0x64 flies in; refreshed when the override ends
    HudObject* element = nullptr;       // +0x68 element whose label is restyled
    float value = 0.0f;                 // +0x6C value shown
    float target = 0.0f;                // +0x70 value to count to (at least the profile's coins)
    float rate = 0.0f;                  // +0x74 count per second
    int32_t phase = kPhase_FlyIn;       // +0x78
    float timer = 0.0f;                 // +0x7C time left in the phase
    float showDelay = 0.0f;             // +0x80 the counter shows `value` when it runs out
    bool active = false;                // +0x84
};
HudTextOverride& TextOverride();
// 0x0035B470: restores the label and clears the override.
void ResetTextOverride(HudTextOverride& state);

void RefreshElement(HudObject* object);                             // 0x0035B3BC
void SetFieldScale(HudObject* object, const ape::Vec3& scale, uint32_t index);   // 0x0035B404
void BeginFrameAll();                                               // 0x0035C070
void DetachAnimScripts();                                           // 0x0035C028

// Screen position of elements whose definition gives none (0x0106C9F0).
extern ape::Vec3 g_defaultHudOffset;

// Clip helpers of the elements (the engine's 0x004A5C90 / 0x004A5D40):
// plays the clip registered as `clip` on the controller; the default
// layer's clip is done (true without a controller or layer).
bool PlayHudClip(anim::AnimController* controller, uint32_t clip, float speed, bool loop);
bool HudClipDone(anim::AnimController* controller);

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
void SetContext(entity::Entity* owner, uint32_t name);               // 0x00369D00
void SetContextByString(entity::Entity* owner, const char* name);    // 0x00369D14
void RequestScreen(bool skip, const char* name);                     // 0x0036960C
void SetPowMeterPrefix(const char* prefix);                          // 0x00369CAC
void SnapMeterA();                                                   // 0x00369D58
void SnapMeterB();                                                   // 0x00369658
// Meter B's anim script state for on/off (0x003697E0) and its variant
// flag (0x00369874).
void SetMeterBState(bool on);
void SetMeterBVariant(bool on);
// 0x00369BC8: snaps the meter of `owner` to its target value.
void SnapMeterOf(const entity::Entity* owner);
// 0x0035BFB0: every registered element starts its lifetime script.
void StartLifetimeScripts();
// 0x003655D0: per-frame HUD update: destroys queued elements, applies the
// meter requests, runs the text override and updates the enabled
// elements; elements whose object went away leave their stacks.
void UpdateHud(float dt);
// 0x00366E20: hides every visible element but the busy element and three
// other system elements, remembering them for ResumeHud.
void SuspendHud();
// 0x00367018: shows the elements SuspendHud hid again, stack priorities
// 0..3 in turn.
void ResumeHud();
// 0x003677C4: a plain element built from `source`'s definition, registered
// as `name`.
HudObject* CloneElement(HudObject* source, uint32_t name);
// 0x00367900: the same for the element registered as `sourceName`.
HudObject* CloneElementByName(uint32_t sourceName, uint32_t name);
// 0x0035B668: level unload: every element and definition is destroyed.
void UnloadHud();
// A colour style of the definition file (0x1C bytes, element 0xFA405F99).
struct HudColorStyle {
    uint32_t name = 0;               // +0x00 "Name"
    uint32_t field_04 = 0;           // +0x04 attribute 0x7A30D7E3
    uint32_t field_08 = 0;           // +0x08 attribute 0x760CB725
    uint8_t colors[3][4] = {};       // +0x0C attributes 0xEAB9B1D2, 0xEAB9B1D3, 0x68762E36
    uint32_t field_18 = 0;           // +0x18 attribute 0x31F8E7E5
};
// 0x0036196C: reads the element definitions and colour styles of a HUD
// definition file into the definition map.
void LoadHudDefinitions(ape::xml::XmlElement* root);
// 0x0035AC7C: builds one element per definition (the element class is
// chosen by the definition's type bits) and registers it by name.
void BuildHudElements();
// 0x00363B78: animates the text override (TODO).
void UpdateTextOverride(float dt, HudTextOverride& state);
void ResetUpdateFlags();                                             // 0x00369500
bool AnyUpdatingElementVisible();                                    // 0x003695A0

} // namespace hud
} // namespace sly
