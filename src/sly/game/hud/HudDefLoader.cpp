// openTIT - The HUD definition file: one definition per element element and
// the colour styles (0x0035C100-0x00361B00).
//
// Every element of the file names its kind by its element name ("HudSimple",
// "HudPrompt", "HudWheel", "HudMenu", "HudRadar", "HudMap", the others are
// only known by hash); the kind's parser allocates the definition, reads the
// common attributes (ParseDefCommon) and its own block, and the loader files
// it in the definition map under its name.
#include "sly/game/hud/HudManager.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "ape/xml/ApeXml.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace sly {
namespace hud {

using ape::xml::XmlElement;

namespace {

// Element names of the definition kinds.
namespace kind {
constexpr uint32_t kSimple = 0xCFF60149u;      // "HudSimple"
constexpr uint32_t kPrompt = 0xCB5728FBu;      // "HudPrompt" (type 0x20000)
constexpr uint32_t kTracker = 0x91E9E41Au;
constexpr uint32_t kPowMeter = 0x81DE879Du;
constexpr uint32_t kType1000 = 0xFC34BC83u;
constexpr uint32_t kColorStyle = 0xFA405F99u;
constexpr uint32_t kScreen = 0xE131E1A4u;
constexpr uint32_t kMeter = 0xD22F2156u;
constexpr uint32_t kWheel = 0x40BB98C4u;       // "HudWheel"
constexpr uint32_t kRadar = 0x4071F06Fu;       // "HudRadar"
constexpr uint32_t kType800 = 0x3D883554u;
constexpr uint32_t kPopup = 0x2B1B81ABu;
constexpr uint32_t kJobList = 0x043A35DDu;
constexpr uint32_t kMap = 0x7FAACD05u;         // "HudMap"
constexpr uint32_t kMenu = 0x75AEE2B6u;        // "HudMenu"
constexpr uint32_t kSubtitle = 0x74194F1Au;
constexpr uint32_t kType40 = 0x50EF70F0u;
} // namespace kind

// Attributes. Names in quotes are recovered; the others keep their hash.
namespace attr {
constexpr uint32_t kObject = 0x2911AA63u;
constexpr uint32_t kClone = 0xA489BE2Au;
constexpr uint32_t kLod = 0x38C41ADCu;
constexpr uint32_t kOffset = 0x2E6543D2u;
constexpr uint32_t kLifetimeScript = 0xCF4A1DD4u;  // "LifetimeScript"
constexpr uint32_t kSlotSize = 0xEDF08AFFu;        // "SlotSize"
constexpr uint32_t kSlotPriority = 0xA4CA6DA2u;    // "SlotPriority"
constexpr uint32_t kSlot = 0x0026E1FEu;            // "Slot"
constexpr uint32_t kTextObjectName = 0x934CAC5Eu;
constexpr uint32_t kColor = 0x0F1B2B8Eu;
constexpr uint32_t kIntroScript = 0xB4B14A17u;     // "IntroScript"
constexpr uint32_t kOutroScript = 0xE00F9276u;     // "OutroScript"
constexpr uint32_t kCloneText = 0x6F82CABEu;
constexpr uint32_t kPlayerColor = 0xBE95A9DFu;
constexpr uint32_t kId = 0x0000091Bu;              // "ID"
constexpr uint32_t kDisplayTime = 0xE690CE6Fu;     // "DisplayTime"
constexpr uint32_t kStartActive = 0x280D85C8u;     // "StartActive"
constexpr uint32_t kName = 0x0024728Bu;            // "Name"
constexpr uint32_t kType = 0x0027873Au;            // "Type"
constexpr uint32_t kText = 0x00273D2Du;            // "TEXT"
constexpr uint32_t kIndex = 0x042930B2u;           // "INDEX"
constexpr uint32_t kTime = 0x00274ACDu;            // "Time"
constexpr uint32_t kStartDelay = 0xBF654E61u;      // "StartDelay"
constexpr uint32_t kMessageObject = 0x8BB559E6u;   // "MessageObject"
constexpr uint32_t kHoldTime = 0x72C08209u;
constexpr uint32_t kDrainTime = 0x08EC14A8u;
constexpr uint32_t kTextCount = 0xDBA1AC01u;
} // namespace attr

constexpr uint32_t kDefaultIntroScript = 0xD8A5F8E7u;
constexpr uint32_t kDefaultOutroScript = 0x3C193BACu;
constexpr int kMaxTextObjects = 20;   // the parsers collect them in a stack buffer of 20
const ape::xml::Color8 kWhite(0xFF, 0xFF, 0xFF, 0xFF);   // 0x01164CA0

// GUI object at `path` under `scope` (null without a path).
ape::ApeGuiObject* FindGui(const char* path, ape::ApeGuiObject* scope)
{
    return path ? ape::FindGuiObject(path, scope) : nullptr;
}

// The text2D object of a text record: a label (text or type-5 object).
ape::ApeGuiObject* FindLabel(XmlElement* e, ape::ApeGuiObject* scope)
{
    ape::ApeGuiObject* label = FindGui(e->GetString("text2D"), scope);
    if (!label)
        return nullptr;
    const int type = label->GetType();
    return (type == ape::kGuiObject_Text || type == ape::kGuiObject_Type5) ? label : nullptr;
}

// The parent frame, text frame and label of a 3DTextObject element.
struct TextObjectGui {
    ape::ApeGuiObject* frame = nullptr;       // "parentFrame2D"
    ape::ApeGuiObject* textFrame = nullptr;   // "textFrame2D" (inside the frame)
    ape::ApeGuiObject* label = nullptr;       // "text2D" (inside the text frame, else the frame)
};
bool ReadTextObjectGui(XmlElement* e, TextObjectGui& out)
{
    out.frame = FindGui(e->GetString("parentFrame2D"), nullptr);
    if (!out.frame)
        return false;
    out.textFrame = FindGui(e->GetString("textFrame2D"), out.frame);
    out.label = FindLabel(e, out.textFrame ? out.textFrame : out.frame);
    return true;
}

int CountChildren(XmlElement* parent, const char* name)
{
    int count = 0;
    for (XmlElement* e = parent->FirstChildElement(name); e; e = e->NextSiblingElement(name))
        ++count;
    return count;
}

int CountChildren(XmlElement* parent, uint32_t name)
{
    int count = 0;
    for (XmlElement* e = parent->FirstChildElement(name); e; e = e->NextSiblingElement(name))
        ++count;
    return count;
}

template <typename T>
T* AllocArray(int count)
{
    return count > 0 ? static_cast<T*>(std::calloc(size_t(count), sizeof(T))) : nullptr;
}

void ReadColor(XmlElement* e, uint32_t name, uint8_t out[4])
{
    ape::xml::Color8 current(out[0], out[1], out[2], out[3]);
    ape::xml::Color8 color = current;
    e->GetColor(name, &color, &current);
    out[0] = color.r;
    out[1] = color.g;
    out[2] = color.b;
    out[3] = color.a;
}

void ReadFlagBit(XmlElement* e, uint32_t name, uint32_t& word, uint32_t bit)
{
    bool on = (word & bit) != 0;
    e->GetBool(name, &on, on);
    word = on ? (word | bit) : (word & ~bit);
}

void ReadFlagBit8(XmlElement* e, uint32_t name, uint8_t& byte, uint8_t bit)
{
    bool on = (byte & bit) != 0;
    e->GetBool(name, &on, on);
    byte = on ? uint8_t(byte | bit) : uint8_t(byte & ~bit);
}

uint32_t HashAttr(XmlElement* e, uint32_t name, uint32_t fallback)
{
    uint32_t value = fallback;
    e->GetHash(name, &value, fallback);
    return value;
}

// The 0x1C-byte text records of the list kinds (0x800, 0x1000, 0x2000):
// "ID", colour, scene object, frames, label and the player colour bit.
void ReadListFields(XmlElement* e, HudObjectDef::ListFieldDef*& fields, uint32_t& fieldCount)
{
    XmlElement* first = e->FirstChildElement("3DTextObject");
    if (!first)
        return;
    int32_t count = 0;
    if (e->GetInt(attr::kTextCount, &count, 0) != 0)
        count = CountChildren(e, "3DTextObject");
    fieldCount = uint32_t(count > 0 ? count : 0);
    if (count <= 0)
        return;
    fields = AllocArray<HudObjectDef::ListFieldDef>(count);
    int i = 0;
    for (XmlElement* t = first; t && i < count; t = t->NextSiblingElement("3DTextObject"), ++i) {
        uint32_t objectName = 0;
        if (t->GetHash(attr::kTextObjectName, &objectName, 0) != 0)
            continue;
        HudObjectDef::ListFieldDef& field = fields[i];
        t->GetHash(attr::kId, &field.name, 0);
        field.objectName = objectName;
        TextObjectGui gui;
        if (!ReadTextObjectGui(t, gui))
            continue;
        field.object = gui.frame;
        field.object3d = gui.textFrame;
        field.label = gui.label;
        ReadColor(t, attr::kColor, field.color);
        ReadFlagBit(t, attr::kPlayerColor, field.flags, 0x80000000u);
    }
}

// The "Checkboxes" element of the list kinds: the row object and one
// locator per row.
void ReadCheckboxes(XmlElement* e, uint32_t rowCount, uint32_t& rowGeometry, const uint32_t*& rowLocators)
{
    XmlElement* boxes = e->FirstChildElement("Checkboxes");
    if (!boxes)
        return;
    boxes->GetHash(attr::kObject, &rowGeometry, rowGeometry);
    if (int32_t(rowCount) <= 0)
        return;
    auto* locators = AllocArray<uint32_t>(int(rowCount));
    uint32_t i = 0;
    for (XmlElement* box = boxes->FirstChildElement("Checkbox"); box && i < rowCount;
         box = box->NextSiblingElement("Checkbox"), ++i)
        box->GetHash(attr::kObject, &locators[i], 0);
    rowLocators = locators;
}

void FileDef(HudObjectDef* def)
{
    if (def)
        Hud().defs.Add(def->name, def);
}

} // namespace

HudObjectDef::~HudObjectDef()
{
    std::free(textFieldDefs);
    std::free(trackerFields);
    std::free(list.fields);
    std::free(const_cast<uint32_t*>(list.rowLocators));
    std::free(list800.fields);
    std::free(const_cast<uint32_t*>(list800.rowLocators));
    std::free(wheel.pages);
    std::free(wheel.fields);
    if (menu.pages) {
        for (uint32_t i = 0; i < menu.pageCount; ++i)
            std::free(menu.pages[i].fields);
        std::free(menu.pages);
    }
    std::free(timedLines.lines);
}

// The attributes every definition has: name, clip prefix, scene object (and
// whether elements clone it), screen offset, lifetime script, stack slot
// size, priority and corner.
// @func 0x0035C100 hud::ParseDefCommon
void ParseDefCommon(HudObjectDef* def, XmlElement* e)
{
    const char* name = e->GetString("name");
    if (name && *name) {
        ape::StrNCopy(def->defName, name, sizeof def->defName);
        def->name = ape::HashNameNoCase(def->defName);
    }
    const char* prefix = e->GetString("prefix");
    if (prefix && *prefix)
        ape::StrNCopy(def->animPrefix, prefix, sizeof def->animPrefix);
    uint32_t objectName = 0;
    if (e->GetHash(attr::kObject, &objectName, 0) == 0) {
        def->templateObject = ape::world::FindGeomInstance(objectName);
        if (!def->templateObject) {
            std::printf("HUD: object %s not found\n", ape::TempHexString(objectName));
        } else {
            ReadFlagBit8(e, attr::kClone, def->objectFlags, HudObjectDef::kObject_Clone);
            ReadFlagBit8(e, attr::kLod, def->objectFlags, HudObjectDef::kObject_Lod);
        }
    }
    float vec[3] = {def->baseOffset.x, def->baseOffset.y, def->baseOffset.z};
    e->GetVec3(attr::kOffset, vec, vec);
    def->baseOffset = {vec[0], vec[1], vec[2], 0.0f};
    e->GetHash(attr::kLifetimeScript, &def->field_4C, def->field_4C);
    e->GetVec3(attr::kSlotSize, def->offset, def->offset);
    e->GetInt(attr::kSlotPriority, &def->priority, def->priority);
    if (def->priority > 4) {
        std::printf("exceeded max slot priority of %u.  forcing to max.\n", 4u);
        def->priority = 4;
    }
    uint32_t slot = 0;
    e->GetHash(attr::kSlot, &slot, 0);
    switch (slot) {
    case 0x0A78u: def->anchor = kAnchor_TopLeft; break;      // "TL"
    case 0x0A7Eu: def->anchor = kAnchor_TopRight; break;     // "TR"
    case 0x084Au: def->anchor = kAnchor_BottomLeft; break;   // "BL"
    case 0x0850u: def->anchor = kAnchor_BottomRight; break;  // "BR"
    default: def->anchor = kAnchor_None; break;
    }
}

// Common elements: the "3DTextObject" text fields (at most 20), the hold
// time ("DisplayTime") and "StartActive".
// @func 0x0035C3E4 hud::ParseElementDef
HudObjectDef* ParseElementDef(HudObjectDef* def, XmlElement* e)
{
    HudObjectDef::TextFieldDef fields[kMaxTextObjects];
    for (HudObjectDef::TextFieldDef& f : fields) {
        f.introScript = kDefaultIntroScript;
        f.outroScript = kDefaultOutroScript;
    }
    ParseDefCommon(def, e);
    def->textFieldDefs = nullptr;
    def->textFieldCount = 0;
    int count = 0;
    for (XmlElement* t = e->FirstChildElement("3DTextObject"); t && count < kMaxTextObjects;
         t = t->NextSiblingElement("3DTextObject")) {
        HudObjectDef::TextFieldDef& field = fields[count++];
        uint32_t objectName = 0;
        if (t->GetHash(attr::kTextObjectName, &objectName, 0) != 0)
            continue;
        field.objectName = objectName;
        TextObjectGui gui;
        if (!ReadTextObjectGui(t, gui))
            continue;
        field.templateObject = gui.frame;
        field.framePath = gui.textFrame;
        field.labelPath = gui.label;
        ReadColor(t, attr::kColor, field.color);
        t->GetHash(attr::kIntroScript, &field.introScript, field.introScript);
        t->GetHash(attr::kOutroScript, &field.outroScript, field.outroScript);
        ReadFlagBit(t, attr::kCloneText, field.field_1C, HudObjectDef::TextFieldDef::kField_Clone);
        ReadFlagBit(t, attr::kPlayerColor, field.field_1C, HudObjectDef::TextFieldDef::kField_PlayerColor);
    }
    if (count) {
        def->textFieldDefs = AllocArray<HudObjectDef::TextFieldDef>(count);
        std::memcpy(def->textFieldDefs, fields, sizeof(HudObjectDef::TextFieldDef) * size_t(count));
        def->textFieldCount = uint32_t(count);
    }
    e->GetFloat(attr::kDisplayTime, &def->defaultFade, def->defaultFade);
    bool startActive = (def->elementFlags & 1) != 0;
    e->GetBool(attr::kStartActive, &startActive, startActive);
    def->elementFlags = startActive ? 1 : 0;
    return def;
}

namespace {

HudObjectDef* NewElementDef(uint32_t typeMask)
{
    auto* def = new HudObjectDef();
    def->defaultFade = -1.0f;
    def->typeMask |= typeMask;
    return def;
}

} // namespace

// "HudPrompt": a common element of type 0x20000.
// @func 0x0035C8CC hud::CreatePromptDef
HudObjectDef* CreatePromptDef(XmlElement* e)
{
    return ParseElementDef(NewElementDef(kHud_Object | kHud_Type20000), e);
}

// "HudSimple": a plain common element.
// @func 0x00361108 hud::CreateSimpleDef
HudObjectDef* CreateSimpleDef(XmlElement* e)
{
    return ParseElementDef(NewElementDef(kHud_Object), e);
}

// Timed text element: its "MessageObject" lines ("INDEX" field, "Time",
// "TEXT") and the "StartDelay".
// @func 0x00360DE8 hud::CreateSubtitleDef
HudObjectDef* CreateSubtitleDef(XmlElement* e)
{
    HudObjectDef* def = ParseElementDef(NewElementDef(kHud_Object | kHud_Type4000), e);
    const int count = CountChildren(e, attr::kMessageObject);
    def->timedLines.count = count;
    if (count) {
        def->timedLines.lines = AllocArray<HudTimedLine>(count);
        int i = 0;
        for (XmlElement* m = e->FirstChildElement(attr::kMessageObject); m && i < count;
             m = m->NextSiblingElement(attr::kMessageObject), ++i) {
            HudTimedLine& line = def->timedLines.lines[i];
            const char* text = "";
            m->GetStringOrNumber(attr::kText, &text, "");
            int32_t field = 0;
            m->GetInt(attr::kIndex, &field, 0);
            line.field = uint32_t(field);
            m->GetFloat(attr::kTime, &line.time, 0.0f);
            if (text)
                ape::StrCopyBounded(line.text, text, sizeof line.text - 1);
        }
    }
    e->GetFloat(attr::kStartDelay, &def->timedLines.delay, 0.0f);
    return def;
}

// Popup element (type 0x20): "StartActive" shows it from the start.
// @func 0x00361234 hud::CreatePopupDef
HudObjectDef* CreatePopupDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type20;
    ParseDefCommon(def, e);
    bool startShown = def->startShown != 0;
    e->GetBool(attr::kStartActive, &startShown, startShown);
    def->startShown = startShown ? 1 : 0;
    return def;
}

// Prompt element of type 0x40: three scene objects and the "titleText2D",
// "msgText2D" and "acceptMsgText2D" labels.
// @func 0x003602D4 hud::CreateType40Def
HudObjectDef* CreateType40Def(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type40;
    ParseDefCommon(def, e);
    e->GetHash(0xD6A44C96u, &def->promptObjects[0], def->promptObjects[0]);
    e->GetHash(0x85BE0AFFu, &def->promptObjects[1], def->promptObjects[1]);
    e->GetHash(0xA23D0AF7u, &def->promptObjects[2], def->promptObjects[2]);
    def->promptGui[0] = FindGui(e->GetString("titleText2D"), nullptr);
    def->promptGui[1] = FindGui(e->GetString("msgText2D"), nullptr);
    def->promptGui[2] = FindGui(e->GetString("acceptMsgText2D"), nullptr);
    return def;
}

// Tracker list (type 4): 16-byte text records and the marker object.
// @func 0x003604E0 hud::CreateTrackerDef
HudObjectDef* CreateTrackerDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type4;
    ParseDefCommon(def, e);
    if (e->FirstChildElement("3DTextObject")) {
        const int count = CountChildren(e, "3DTextObject");
        def->textFieldCount = uint32_t(count);
        def->trackerFields = AllocArray<HudObjectDef::TrackerFieldDef>(count);
        int i = 0;
        for (XmlElement* t = e->FirstChildElement("3DTextObject"); t && i < count;
             t = t->NextSiblingElement("3DTextObject"), ++i) {
            uint32_t objectName = 0;
            if (t->GetHash(attr::kTextObjectName, &objectName, 0) != 0)
                continue;
            HudObjectDef::TrackerFieldDef& field = def->trackerFields[i];
            field.objectName = objectName;
            TextObjectGui gui;
            if (!ReadTextObjectGui(t, gui))
                continue;
            field.object = gui.frame;
            field.textFrame = gui.textFrame;
            field.label = gui.label;
        }
    }
    e->GetHash(0x99C55183u, &def->trackerMarkerName, def->trackerMarkerName);
    return def;
}

// Screen element (type 2): text records with an "ID" word in place of the
// flags, and whether the screen hides its sub-objects.
// @func 0x003608E8 hud::CreateScreenDef
HudObjectDef* CreateScreenDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type2;
    ParseDefCommon(def, e);
    HudObjectDef::TextFieldDef fields[kMaxTextObjects];
    for (HudObjectDef::TextFieldDef& f : fields) {
        f.introScript = kDefaultIntroScript;
        f.outroScript = kDefaultOutroScript;
    }
    int count = 0;
    for (XmlElement* t = e->FirstChildElement("3DTextObject"); t && count < kMaxTextObjects;
         t = t->NextSiblingElement("3DTextObject")) {
        HudObjectDef::TextFieldDef& field = fields[count++];
        uint32_t objectName = 0;
        if (t->GetHash(attr::kTextObjectName, &objectName, 0) != 0)
            continue;
        field.objectName = objectName;
        TextObjectGui gui;
        if (!ReadTextObjectGui(t, gui))
            continue;
        field.templateObject = gui.frame;
        field.framePath = gui.textFrame;
        field.labelPath = gui.label;
        ReadColor(t, attr::kColor, field.color);
        t->GetHash(attr::kIntroScript, &field.introScript, field.introScript);
        t->GetHash(attr::kOutroScript, &field.outroScript, field.outroScript);
        t->GetHash(attr::kId, &field.field_1C, field.field_1C);
    }
    if (count) {
        def->textFieldDefs = AllocArray<HudObjectDef::TextFieldDef>(count);
        std::memcpy(def->textFieldDefs, fields, sizeof(HudObjectDef::TextFieldDef) * size_t(count));
        def->textFieldCount = uint32_t(count);
    }
    e->GetBool(0x2EC1584Cu, &def->hideSubObjects, def->hideSubObjects);
    return def;
}

// POW meter (type 0x10): the fills, ten pips with their sounds and scripts,
// and the drain timing.
// @func 0x00361374 hud::CreatePowMeterDef
HudObjectDef* CreatePowMeterDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type10;
    ParseDefCommon(def, e);
    HudObjectDef::PowMeterDef& pow = def->pow;
    e->GetHash(0x623DA031u, &pow.fillA, pow.fillA);
    e->GetHash(0xC9566036u, &pow.fillB, pow.fillB);
    e->GetHash(attr::kText, &pow.field_F0, pow.field_F0);
    e->GetHash(0x577E6D38u, &pow.introScript, pow.introScript);
    e->GetHash(0x59E72E8Au, &pow.field_F8, pow.field_F8);
    e->GetFloat(attr::kHoldTime, &pow.holdTime, pow.holdTime);
    e->GetFloat(attr::kDrainTime, &pow.drainTime, pow.drainTime);
    for (int i = 0; i < 10; ++i) {
        char name[0x40];
        std::snprintf(name, sizeof name, "icon%d", i);
        e->GetHash(name, &pow.pips[i], 0);
        std::snprintf(name, sizeof name, "sound%d", i);
        e->GetHash(name, &pow.sounds[i], 0);
        std::snprintf(name, sizeof name, "addBitScript%d", i);
        e->GetHash(name, &pow.pipScripts[i], 0);
    }
    return def;
}

// Health meter (type 8).
// @func 0x003616A4 hud::CreateMeterDef
HudObjectDef* CreateMeterDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type8;
    HudObjectDef::MeterDef& meter = def->meter;
    meter.showFill = 1;
    meter.healthMeter = 1;
    ParseDefCommon(def, e);
    bool flag = meter.useOwner != 0;
    e->GetBool(0xA9CD4232u, &flag, flag);
    meter.useOwner = flag ? 1 : 0;
    e->GetHash(0xB741B705u, &meter.fillA, meter.fillA);
    e->GetHash(0xEDB0120Au, &meter.fillB, meter.fillB);
    e->GetHash(0x41F6E0FEu, &meter.frame, meter.frame);
    flag = meter.showFill != 0;
    e->GetBool(0x12B5C5ADu, &flag, flag);
    meter.showFill = flag ? 1 : 0;
    e->GetHash(0x58F3971Cu, &meter.extra, meter.extra);
    e->GetHash(0xEA6A4861u, &meter.extra2, meter.extra2);
    flag = meter.showExtra != 0;
    e->GetBool(0xB462EDB6u, &flag, flag);
    meter.showExtra = flag ? 1 : 0;
    e->GetFloat(attr::kHoldTime, &meter.holdTime, meter.holdTime);
    e->GetFloat(attr::kDrainTime, &meter.drainTime, meter.drainTime);
    flag = meter.healthMeter != 0;
    e->GetBool(0x46C1AC39u, &flag, flag);
    meter.healthMeter = flag ? 1 : 0;
    return def;
}

namespace {

// The rows, scroll bar objects, checkboxes and texts the job list and the
// 0x1000 kind share.
void ReadListDef(XmlElement* e, HudObjectDef::ListDef& list, bool hasFourthObject)
{
    int32_t value = int32_t(list.rowCount);
    e->GetInt(0xBF27DE6Fu, &value, value);
    list.rowCount = uint32_t(value);
    value = int32_t(list.cursorRow);
    e->GetInt(0x073922B6u, &value, value);
    list.cursorRow = uint32_t(value);
    e->GetHash(0xFC493ADBu, &list.knobName, list.knobName);
    e->GetHash(0x90B6CF40u, &list.barTopName, list.barTopName);
    e->GetHash(0x464245FAu, &list.barBottomName, list.barBottomName);
    if (hasFourthObject)
        e->GetHash(0xA3DAD21Cu, &list.field_A8, list.field_A8);
    ReadCheckboxes(e, list.rowCount, list.rowGeometry, list.rowLocators);
    ReadListFields(e, list.fields, list.fieldCount);
    e->GetFloat(attr::kDisplayTime, &list.defaultFade, list.defaultFade);
    bool startActive = (list.flags & 1) != 0;
    e->GetBool(attr::kStartActive, &startActive, startActive);
    list.flags = startActive ? 1 : 0;
}

} // namespace

// Job list (type 0x2000).
// @func 0x0035DB38 hud::CreateJobListDef
HudObjectDef* CreateJobListDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->list.defaultFade = -1.0f;
    def->typeMask |= kHud_Type2000;
    ParseDefCommon(def, e);
    ReadListDef(e, def->list, false);
    return def;
}

// Type 0x1000: the job list's block with a fourth object.
// @func 0x0035E21C hud::CreateType1000Def
HudObjectDef* CreateType1000Def(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->list.defaultFade = -1.0f;
    def->typeMask |= kHud_Type1000;
    ParseDefCommon(def, e);
    ReadListDef(e, def->list, true);
    return def;
}

// Type 0x800: five objects and a flag before the shared block.
// @func 0x0035E928 hud::CreateType800Def
HudObjectDef* CreateType800Def(XmlElement* e)
{
    auto* def = new HudObjectDef();
    HudObjectDef::List800Def& list = def->list800;
    list.defaultFade = -1.0f;
    def->typeMask |= kHud_Type800;
    ParseDefCommon(def, e);
    int32_t value = int32_t(list.rowCount);
    e->GetInt(0xBF27DE6Fu, &value, value);
    list.rowCount = uint32_t(value);
    value = int32_t(list.cursorRow);
    e->GetInt(0x073922B6u, &value, value);
    list.cursorRow = uint32_t(value);
    const uint32_t names[5] = {0xFC493ADBu, 0x90B6CF40u, 0x464245FAu, 0xA3DAD21Cu, 0x7B9F2997u};
    for (int i = 0; i < 5; ++i)
        e->GetHash(names[i], &list.objectNames[i], list.objectNames[i]);
    e->GetBool(0x379040C0u, &list.flagC5, list.flagC5);
    ReadCheckboxes(e, list.rowCount, list.rowGeometry, list.rowLocators);
    ReadListFields(e, list.fields, list.fieldCount);
    e->GetFloat(attr::kDisplayTime, &list.defaultFade, list.defaultFade);
    bool startActive = (list.flags & 1) != 0;
    e->GetBool(attr::kStartActive, &startActive, startActive);
    list.flags = startActive ? 1 : 0;
    return def;
}

// "HudWheel" (type 0x400): pointer, axis and page angle, the pulse, the
// glow object, the kind, the texts and one page per "Slot" element.
// @func 0x0035F074 hud::CreateWheelDef
HudObjectDef* CreateWheelDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type400;
    ParseDefCommon(def, e);
    HudObjectDef::WheelDef& wheel = def->wheel;
    e->GetHash(0xEC96E888u, &wheel.pointerInstance, wheel.pointerInstance);
    e->GetHash(0x5245F8BAu, &wheel.pointerName, wheel.pointerName);
    float axis[3] = {wheel.axis.x, wheel.axis.y, wheel.axis.z};
    e->GetVec3(0x33ECE109u, axis, axis);
    wheel.axis = {axis[0], axis[1], axis[2], 0.0f};
    e->GetFloat(0x2028F1FAu, &wheel.pageAngle, wheel.pageAngle);
    e->GetFloat(0xEA68DF12u, &wheel.pulsePeriod, wheel.pulsePeriod);
    e->GetFloat(0xC9037FF5u, &wheel.pulseAmplitude, wheel.pulseAmplitude);
    e->GetHash(0x62AA4B26u, &wheel.glowName, wheel.glowName);
    e->GetHash(attr::kType, &wheel.kind, wheel.kind);

    if (e->FirstChildElement("3DTextObject")) {
        const int count = CountChildren(e, "3DTextObject");
        wheel.fieldCount = uint32_t(count);
        wheel.fields = AllocArray<HudObjectDef::WheelFieldDef>(count);
        int i = 0;
        for (XmlElement* t = e->FirstChildElement("3DTextObject"); t && i < count;
             t = t->NextSiblingElement("3DTextObject"), ++i) {
            uint32_t objectName = 0;
            if (t->GetHash(attr::kTextObjectName, &objectName, 0) != 0)
                continue;
            HudObjectDef::WheelFieldDef& field = wheel.fields[i];
            field.objectName = objectName;
            t->GetHash(attr::kId, &field.name, field.name);
            TextObjectGui gui;
            if (!ReadTextObjectGui(t, gui))
                continue;
            field.object = gui.frame;
            field.object3d = gui.textFrame;
            field.label = gui.label;
            ReadColor(t, attr::kColor, field.color);
            ReadFlagBit(t, attr::kPlayerColor, field.flags, 0x80000000u);
        }
    }

    wheel.pageCount = CountChildren(e, attr::kSlot);
    if (wheel.pageCount > 0) {
        wheel.pages = AllocArray<HudObjectDef::WheelPageDef>(wheel.pageCount);
        int i = 0;
        for (XmlElement* s = e->FirstChildElement(attr::kSlot); s && i < wheel.pageCount;
             s = s->NextSiblingElement(attr::kSlot), ++i) {
            HudObjectDef::WheelPageDef& page = wheel.pages[i];
            s->GetHash(0x4CE35A8Du, &page.spokeName, page.spokeName);
            s->GetHash(attr::kObject, &page.availableName, page.availableName);
            s->GetHash(0xFCF7E9DFu, &page.lockedName, page.lockedName);
            s->GetHash(0xBC383E08u, &page.pointerName, page.pointerName);
            s->GetHash(0x8DB5DF87u, &page.item, page.item);
            s->GetHash(0xE4D43099u, &page.field_14, page.field_14);
            if (const char* text = s->GetString(0xF7BE5DF7u))
                ape::StrCopyBounded(page.text, text, sizeof page.text);
        }
    }
    return def;
}

// "HudMenu" (type 0x200): pages ("State": "Name", the two clip set
// indices and their texts), the four map corner points, the map texture
// and colour, two floats, the child object and the two prompt elements.
// @func 0x0035F8FC hud::CreateMenuDef
HudObjectDef* CreateMenuDef(XmlElement* e)
{
    // Values of the clip set attributes (0x00ADCA68): index 0..2.
    static const uint32_t kClipSets[3] = {0x86DF6221u, 0x41u /* "A" */, 0x42u /* "B" */};
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type200;
    ParseDefCommon(def, e);
    HudObjectDef::MenuDef& menu = def->menu;
    const int pageCount = CountChildren(e, "State");
    menu.pageCount = uint32_t(pageCount);
    if (pageCount) {
        menu.pages = AllocArray<HudObjectDef::MenuPageDef>(pageCount);
        int p = 0;
        for (XmlElement* s = e->FirstChildElement("State"); s && p < pageCount; s = s->NextSiblingElement("State"), ++p) {
            HudObjectDef::MenuPageDef& page = menu.pages[p];
            s->GetHash(attr::kName, &page.name, page.name);
            uint32_t value = 0;
            if (s->GetHash(0x04B42877u, &value, 0) == 0) {
                for (uint8_t i = 0; i < 3; ++i) {
                    if (kClipSets[i] == value)
                        page.animA = i;
                }
            }
            if (s->GetHash(0x04B42878u, &value, 0) == 0) {
                for (uint8_t i = 0; i < 3; ++i) {
                    if (kClipSets[i] == value)
                        page.animB = i;
                }
            }
            if (!s->FirstChildElement("3DTextObject"))
                continue;
            const int count = CountChildren(s, "3DTextObject");
            page.fieldCount = uint32_t(count);
            page.fields = AllocArray<HudObjectDef::MenuFieldDef>(count);
            int i = 0;
            for (XmlElement* t = s->FirstChildElement("3DTextObject"); t && i < count;
                 t = t->NextSiblingElement("3DTextObject"), ++i) {
                uint32_t objectName = 0;
                if (t->GetHash(attr::kTextObjectName, &objectName, 0) != 0)
                    continue;
                HudObjectDef::MenuFieldDef& field = page.fields[i];
                field.objectName = objectName;
                TextObjectGui gui;
                if (!ReadTextObjectGui(t, gui))
                    continue;
                field.object = gui.frame;
                field.object3d = gui.textFrame;
                field.label = gui.label;
            }
        }
    }
    const char* const corners[4] = {"TLPoint", "TRPoint", "BLPoint", "BRPoint"};
    for (int i = 0; i < 4; ++i) {
        if (XmlElement* point = e->FirstChildElement(corners[i])) {
            point->GetHash(attr::kName, &menu.corners[i], menu.corners[i]);
            ReadColor(point, 0x5399A8B2u, menu.cornerColors[i]);
        }
    }
    e->GetHash(0xDF617116u, &menu.texture, menu.texture);
    ReadColor(e, 0x4C2E77FDu, menu.mapColor);
    e->GetFloat(0xEA68DF12u, &menu.field_CC, menu.field_CC);
    e->GetFloat(0xC9037FF5u, &menu.field_D0, menu.field_D0);
    e->GetHash(0x62AA4B26u, &menu.childName, menu.childName);
    e->GetHash(0xC5268ED9u, &menu.backButton, menu.backButton);
    e->GetHash(0x5AD97285u, &menu.selectButton, menu.selectButton);
    return def;
}

// The radar's block: hold time, "StartActive", the blip instances, two
// floats and three colours.
// @func 0x0035D708 hud::ParseRadarDef
HudObjectDef* ParseRadarDef(HudObjectDef* def, XmlElement* e)
{
    ParseDefCommon(def, e);
    HudObjectDef::RadarDef& radar = def->radar;
    e->GetFloat(attr::kDisplayTime, &radar.defaultFade, radar.defaultFade);
    bool startActive = (radar.flags & 1) != 0;
    e->GetBool(attr::kStartActive, &startActive, startActive);
    radar.flags = startActive ? 1 : 0;
    e->GetHash(0x5CCAD404u, &radar.playerBlip, radar.playerBlip);
    e->GetHash(0x57FBAF97u, &radar.background, radar.background);
    e->GetHash(0x1395597Cu, &radar.enemyBlip, radar.enemyBlip);
    e->GetHash(0x4853B1E7u, &radar.goalBlip, radar.goalBlip);
    e->GetHash(0x2D45A764u, &radar.goalEdgeBlip, radar.goalEdgeBlip);
    e->GetHash(0x66DDB9C0u, &radar.hideoutBlip, radar.hideoutBlip);
    e->GetHash(0x2D6BFE05u, &radar.followBlip, radar.followBlip);
    e->GetHash(0x429F935Au, &radar.bottleBlip, radar.bottleBlip);
    e->GetHash(0x08B994E1u, &radar.safeBlip, radar.safeBlip);
    e->GetFloat(0x81B883B4u, &radar.field_C0, radar.field_C0);
    e->GetFloat(0xEDF0A684u, &radar.field_C4, radar.field_C4);
    ReadColor(e, 0x894AC0DBu, radar.colors[0]);
    ReadColor(e, 0xAFE4553Du, radar.colors[1]);
    ReadColor(e, 0x62E42FD6u, radar.colors[2]);
    return def;
}

// "HudRadar" (type 0x8000).
// @func 0x0035D958 hud::CreateRadarDef
HudObjectDef* CreateRadarDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->radar.defaultFade = -1.0f;
    def->typeMask |= kHud_Type8000;
    return ParseRadarDef(def, e);
}

namespace {

// TODO(0x0035D4B0 / 0x0035CA00 "HudMap"): the map definition block is not
// ported; the definition only gets the common attributes and its type bit.
HudObjectDef* CreateMapDef(XmlElement* e)
{
    auto* def = new HudObjectDef();
    def->typeMask |= kHud_Type10000;
    ParseDefCommon(def, e);
    return def;
}

// A colour style; kept unless its name is missing or already used.
void ReadColorStyle(XmlElement* e)
{
    auto* style = static_cast<HudColorStyle*>(std::calloc(1, sizeof(HudColorStyle)));
    const uint8_t black[4] = {0, 0, 0, 0xFF};   // 0x01164CA8
    for (int i = 0; i < 2; ++i)
        std::memcpy(style->colors[i], black, 4);
    std::memcpy(style->colors[2], &kWhite, 4);
    style->name = HashAttr(e, attr::kName, 0);
    if (style->name != 0) {
        e->GetHash(0x7A30D7E3u, &style->field_04, 0);
        e->GetHash(0x760CB725u, &style->field_08, 0);
        const ape::xml::Color8 white = kWhite;
        for (int i = 0; i < 3; ++i) {
            const uint32_t names[3] = {0xEAB9B1D2u, 0xEAB9B1D3u, 0x68762E36u};
            ape::xml::Color8 color = white;
            e->GetColor(names[i], &color, &white);
            style->colors[i][0] = color.r;
            style->colors[i][1] = color.g;
            style->colors[i][2] = color.b;
            style->colors[i][3] = color.a;
        }
        e->GetHash(0x31F8E7E5u, &style->field_18, 0);
        if (!*Hud().defBuffers.Find(style->name)) {
            Hud().defBuffers.Add(style->name, style);
            return;
        }
    }
    std::free(style);
}

} // namespace

// @func 0x0036AA04 hud::ColorStyleColorA
ape::xml::Color8 ColorStyleColorA(HudColorStyle* const* found)
{
    const HudColorStyle* style = found ? *found : nullptr;
    if (!style)
        return ape::xml::Color8(0, 0, 0, 0xFF);   // 0x01164CA8
    return ape::xml::Color8(style->colors[0][0], style->colors[0][1], style->colors[0][2], style->colors[0][3]);
}

// @func 0x0036AA68 hud::ColorStyleColorB
ape::xml::Color8 ColorStyleColorB(HudColorStyle* const* found)
{
    const HudColorStyle* style = found ? *found : nullptr;
    if (!style)
        return ape::xml::Color8(0, 0, 0, 0xFF);   // 0x01164CA8
    return ape::xml::Color8(style->colors[1][0], style->colors[1][1], style->colors[1][2], style->colors[1][3]);
}

// @func 0x0036196C hud::LoadHudDefinitions
void LoadHudDefinitions(XmlElement* root)
{
    for (XmlElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement()) {
        switch (e->GetNameHash()) {
        case kind::kSimple: FileDef(CreateSimpleDef(e)); break;
        case kind::kPrompt: FileDef(CreatePromptDef(e)); break;
        case kind::kTracker: FileDef(CreateTrackerDef(e)); break;
        case kind::kPowMeter: FileDef(CreatePowMeterDef(e)); break;
        case kind::kType1000: FileDef(CreateType1000Def(e)); break;
        case kind::kColorStyle: ReadColorStyle(e); break;
        case kind::kScreen: FileDef(CreateScreenDef(e)); break;
        case kind::kMeter: FileDef(CreateMeterDef(e)); break;
        case kind::kWheel: FileDef(CreateWheelDef(e)); break;
        case kind::kRadar: FileDef(CreateRadarDef(e)); break;
        case kind::kType800: FileDef(CreateType800Def(e)); break;
        case kind::kPopup: FileDef(CreatePopupDef(e)); break;
        case kind::kJobList: FileDef(CreateJobListDef(e)); break;
        case kind::kMap: FileDef(CreateMapDef(e)); break;
        case kind::kMenu: FileDef(CreateMenuDef(e)); break;
        case kind::kSubtitle: FileDef(CreateSubtitleDef(e)); break;
        case kind::kType40: FileDef(CreateType40Def(e)); break;
        default: break;
        }
    }
}

} // namespace hud
} // namespace sly
