// openTIT - HUD element kinds 0x800, 0x1000 and 0x10000, see
// HudEpisodeElements.h.
#include "sly/game/hud/HudEpisodeElements.h"

#include "sly/game/AppRuntime.h"
#include "sly/game/ObjectHelpers.h"
#include "sly/game/SlyGame.h"
#include "sly/game/Upgrades.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudHooks.h"
#include "sly/anim/AnimController.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSceneSetup.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace sly {
namespace hud {

namespace {

const uint8_t kNoTint[4] = {0, 0, 0, 0};

// Time periods of the episodes (0x00F64E00) and the instance each one shows
// with the 0x800 element (0x00F64E1C).
const uint32_t kEpisodePeriods[7] = {
    0xC04EF95Cu, 0x04318BE6u /* Japan */, 0x002899AFu /* West */, 0x80624694u /* IceAge */,
    0xCB3407A9u /* England */, 0x738D14AAu /* Arabia */, 0xD598818Bu,
};
const uint32_t kPeriodInstances800[7] = {
    0x5E7ED3DBu, 0x451CDC6Eu, 0x451CDE08u, 0x451CDC40u, 0x451CDBD1u, 0x451CDB59u, 0x451CDD19u,
};
constexpr uint32_t kDefaultPeriod = 0x04318BE6u;   // "Japan"
// Extra objects of the 0x800 rows (0x00F64D88 / 0x00F64DA0).
const uint32_t kRowExtraA[6] = {0x58EDE9EBu, 0x58FC016Cu, 0x590A18EDu, 0x5918306Eu, 0x592647EFu, 0x59345F70u};
const uint32_t kRowExtraB[6] = {0x346AB189u, 0x346B25E8u, 0x346B9A47u, 0x346C0EA6u, 0x346C8305u, 0x346CF764u};
// Text fields the 0x800 element starts with "0" and "2".
constexpr uint32_t kField800_Count = 0xF17AEB11u;
constexpr uint32_t kField800_Total = 0x10C350EEu;
// Per-episode instances of the 0x1000 element.
const uint32_t kEpisode1000A[6] = {0xEF3B435Cu, 0xEF3B42B1u, 0xEF3B444Bu, 0xEF3B4283u, 0xEF3B4214u, 0xEF3B419Cu};
const uint32_t kEpisode1000B[6] = {0x04DABCADu, 0x04DABC02u, 0x04DABD9Cu, 0x04DABBD4u, 0x04DABB65u, 0x04DABAEDu};
const uint32_t kEpisode1000C[6] = {0x15AA4488u, 0x15AA43DDu, 0x15AA4577u, 0x15AA43AFu, 0x15AA4340u, 0x15AA42C8u};
const uint32_t kEpisode1000D[6] = {0xA6EC0B80u, 0xA6EC0AD5u, 0xA6EC0C6Fu, 0xA6EC0AA7u, 0xA6EC0A38u, 0xA6EC09C0u};
constexpr uint32_t kCopy1000A = 0xEF3B431Eu;
constexpr uint32_t kCopy1000B = 0x04DABC6Fu;
constexpr uint32_t kCopy1000C = 0x15AA444Au;

// World callback lists.
constexpr int kCallbackList_Draw = 8;
constexpr int kCallbackList_Place = 3;

uint32_t ClipName(const char* prefix, const char* suffix)
{
    char clip[0x60];
    std::snprintf(clip, sizeof clip, "%s_%s", prefix, suffix);
    return ape::HashNameNoCase(clip);
}

// Intro, OnUpdate, Idle, ScrollUp, ScrollDn, Outro in the list clip slots.
void MakeListClips(const HudObjectDef* def, uint32_t clips[7])
{
    for (int i = 0; i < 7; ++i)
        clips[i] = 0;
    const char* prefix = def->animPrefix[0] ? def->animPrefix : def->defName;
    if (!prefix[0])
        return;
    clips[1] = ClipName(prefix, "Intro");
    clips[2] = ClipName(prefix, "OnUpdate");
    clips[3] = ClipName(prefix, "Idle");
    clips[6] = ClipName(prefix, "Outro");
    clips[4] = ClipName(prefix, "ScrollUp");
    clips[5] = ClipName(prefix, "ScrollDn");
}

// A hidden, lit copy of the geometry instance named `name`.
ape::world::ApeObj* CopyGeometry(uint32_t name)
{
    ape::world::ApeObj* source = ape::world::FindGeomInstance(name);
    if (!source)
        return nullptr;
    ape::world::ApeObj* copy = HudCloneObject(source);
    if (copy)
        copy->SetVisible(false, true);
    return copy;
}

ape::world::ApeObj* CopyInstance(ape::world::ApeObj* source)
{
    if (!source)
        return nullptr;
    ape::world::ApeObj* copy = source->Clone(5);
    if (copy)
        ape::world::AddToDynamicObjects(copy, true);
    return copy;
}

void HideObject(ape::world::ApeObj* object)
{
    if (object)
        object->SetVisible(false, true);
}

void ReleaseObject(ape::world::ApeObj*& object)
{
    if (object)
        object->Release();
    object = nullptr;
}

// Persistent elements stay shown; LeaveB queues the element for destruction.
uint32_t RefreshListFlags(uint32_t old)
{
    if (old & kE_LeaveA)
        return old & 0x927FFFFFu;
    return (old & 0xB27FFFFFu) | ((old & kE_Persistent) ? kE_Shown : 0u);
}

void QueueDestroy(HudObject* element)
{
    if (!*Hud().hideList.Find(element->RegisteredName()))
        Hud().hideList.Add(element->RegisteredName(), element);
}

void SetListFieldText(const HudObjectDef::ListFieldDef* fields, uint32_t count, uint32_t name, const char* text)
{
    for (uint32_t i = 0; i < count; ++i) {
        if (fields[i].name != name)
            continue;
        if (ape::ApeGuiObject* label = fields[i].label) {
            label->SetText(text, false);
            label->Invalidate();
        }
        return;
    }
}

// Anim script states of the list states (0x00F64FC4 / 0x00F64FE0): "NONE",
// "Intro", "OnUpdate", "Idle", "ScrollUp", "ScrollDown", "Outro".
const uint32_t kListStateNames[7] = {
    0x0024A738u, 0x04296E4Cu, 0xC2361368u, 0x002237D4u, 0x16AD6628u, 0x20E4C8EFu, 0x048129EBu,
};
constexpr uint32_t kState_Intro = 1;
constexpr uint32_t kState_Update = 2;
constexpr uint32_t kState_Idle = 3;
constexpr uint32_t kState_Outro = 6;

uint32_t SetBit(uint32_t word, uint32_t bit, bool on) { return on ? (word | bit) : (word & ~bit); }

// Clip and script state of `state` for the list kinds.
void PlayListState(HudObject* element, const uint32_t clips[7], uint32_t& flags, uint32_t state)
{
    flags = SetBit(flags, kE_AnimOk, PlayHudClip(element->animController, clips[state], 1.0f, false));
    if (element->animScript) {
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(element->animScript);
        flags = SetBit(flags, kE_ScriptOk, script->SetStateAndUpdate(kListStateNames[state]));
    }
}

void ShowFieldFrames(const HudObjectDef::ListFieldDef* fields, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        if (ape::ApeGuiObject* frame = fields[i].object)
            frame->SetVisible(fields[i].object3d != nullptr);
    }
}

// The intro of the list kinds.
void BeginListShow(HudObject* element, const uint32_t clips[7], uint32_t& flags,
                   const HudObjectDef::ListFieldDef* fields, uint32_t count)
{
    flags |= kE_Shown;
    element->animState = (element->animState & 1u) | (kState_Intro << 1);
    if (element->object)
        element->object->SetVisible(true, true);
    PlayListState(element, clips, flags, kState_Intro);
    ShowFieldFrames(fields, count);
}

// The hide request of the list kinds; false when the element is blocked.
template <typename List>
void HideList(List* element, uint32_t& flags, const uint32_t clips[7], const HudObjectDef::ListFieldDef* fields,
              uint32_t count, bool a, bool b)
{
    const uint32_t state = element->animState >> 1;
    if (state == kState_Outro || state == 0) {
        if ((!(flags & kE_LeaveA) && !a) || state != 0) {
            if (((flags & kE_LeaveB) || b) && state == 0)
                QueueDestroy(element);
        } else {
            element->Refresh();
        }
    } else {
        flags = (flags & ~(kE_LeaveA | kE_LeaveB)) | (a ? kE_LeaveA : 0u) | (b ? kE_LeaveB : 0u);
        element->listFadeTime = -1.0f;
        element->animState = (element->animState & 1u) | (kState_Outro << 1);
        PlayListState(element, clips, flags, kState_Outro);
        if (element->object)
            element->object->SetVisible(true, true);
        ShowFieldFrames(fields, count);
    }
    element->listLinked = nullptr;
}

bool ListBlocked(uint32_t flags) { return (flags & kE_Suppressed) || !(flags & kE_Shown); }

// Job records of the episodes' first and second parts (0x00F64F00 /
// 0x00F64F18; only the first episode has a second part).
const uint32_t kEpisodeJobs[6] = {0xC04EF95Cu, 0x04318BE6u, 0x002899AFu, 0x80624694u, 0xCB3407A9u, 0x738D14AAu};
const uint32_t kEpisodeSecondJobs[6] = {0xD598818Bu, 0, 0, 0, 0, 0};
// The same with the first episode's second part in place of its first
// (0x00F64E88).
const uint32_t kEpisodeLastPartJobs[6] = {0xD598818Bu, 0x04318BE6u, 0x002899AFu, 0x80624694u, 0xCB3407A9u, 0x738D14AAu};
// Episode records whose completion picks the look of the 0x1000 element's
// row copies (0x00F64ED0) and of its row objects (0x00F64EB8).
const uint32_t kEpisodeRowNames[6] = {0x526FD2CAu, 0x01218C2Du, 0xC833F9E9u, 0x43F5F056u, 0xF3F06421u, 0x79DA09E3u};
const uint32_t kEpisodeNames[6] = {0x79DA09E3u, 0x34D2001Au, 0xDF0B25ADu, 0x35FCDDBBu, 0x54066C8Fu, 0x28FE05F8u};
// Locators of the 0x1000 element: the two title slots (0x00F64E38 /
// 0x00F64E40) and the four visible rows (0x00F64E48).
const uint32_t kTitleLocatorsA[2] = {0x73126D76u, 0x73126D77u};
const uint32_t kTitleLocatorsB[2] = {0xF7E60605u, 0xF7E60606u};
const uint32_t kRowLocators[4] = {0xA9E9EA25u, 0xA9E9EA26u, 0xA9E9EA27u, 0xA9E9EA28u};

constexpr uint32_t kField_Description = 0x001FF691u;   // "DESC"
constexpr uint32_t kScript_Purchase = 0x968EF501u;      // "Purchase"
constexpr uint32_t kScript_Unavailable = 0x23200439u;   // NOTE: name unknown; nothing to travel to
constexpr uint32_t kInput_Up = 0x01;
constexpr uint32_t kInput_Down = 0x02;
constexpr uint32_t kInput_Accept = 0x80;
constexpr uint32_t kState_ScrollUp = 4;
constexpr uint32_t kState_ScrollDown = 5;

// Buttons the first local player pressed this frame.
uint32_t PressedButtons()
{
    const AppPlayer& player = GetAppRuntime().players[0];
    return player.inputFlags & player.inputMask;
}

// Gives `target` the world matrix of `locator` as its local one.
void SetLocalFromWorld(ape::world::ApeObj* target, const ape::world::ApeObj* locator)
{
    ape::Matrix34 local = locator->GetWorld();
    target->SetLocal(local);
    target->MarkDirty();
    target->UpdateWorld(false, false);
}


} // namespace

// ===========================================================================
// Type 0x800

// @func 0x00359284 HudType800::HudType800
HudType800::HudType800(HudObjectDef* def_) : HudObject(0, def_)
{
    const HudObjectDef::List800Def& info = def->list800;
    for (int i = 0; i < 4; ++i)
        tint[i] = kNoTint[i];
    listDefaultFade = info.defaultFade;
    const bool persistent = (info.flags & 1) != 0;
    listFlags = (listFlags & 0x0007FFFFu) | (persistent ? (kE_Persistent | kE_Shown) : 0u);
    listFadeTime = -1.0f;
    listHoldTime = 0.0f;
    selected = 0;
    cursorRow = info.cursorRow;
    field_108 = 0;
    typeFlags |= kHud_Type800;
    text[0] = '\0';
    if (info.fieldCount) {
        fieldObjects = static_cast<ape::world::ApeObj**>(std::calloc(info.fieldCount, sizeof(ape::world::ApeObj*)));
        for (uint32_t i = 0; i < info.fieldCount; ++i)
            fieldObjects[i] = FindInHierarchy(object, info.fields[i].objectName);
    }
    MakeListClips(def, clips);
    for (int i = 0; i < 6; ++i) {
        rows[i] = CopyGeometry(info.rowGeometry);
        rowExtraA[i] = FindInHierarchy(object, kRowExtraA[i]);
        HideObject(rowExtraA[i]);
        rowExtraB[i] = FindInHierarchy(object, kRowExtraB[i]);
        HideObject(rowExtraB[i]);
    }
    for (int i = 0; i < 5; ++i)
        objects[i] = FindInHierarchy(object, info.objectNames[i]);
    SetListFieldText(info.fields, info.fieldCount, kField800_Count, "0");
    SetListFieldText(info.fields, info.fieldCount, kField800_Total, "2");
}

void HudType800::ReleaseEntryObjects()
{
    for (uint16_t i = 0; i < entryObjects.Count(); ++i)
        ReleaseObject(entryObjects[i]);
}

// @func 0x00376F88 HudType800::~HudType800
// @func 0x0037726C HudType800::~HudType800 (deleting)
HudType800::~HudType800()
{
    if (g_game) {
        g_game->RemoveCallback(kCallbackList_Draw, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Place, this, nullptr);
    }
    HudType800::SetActive(false);
    std::free(fieldObjects);
    fieldObjects = nullptr;
    ReleaseEntryObjects();
    for (ape::world::ApeObj*& row : rows)
        ReleaseObject(row);
}

// @func 0x00376C98 HudType800::SetActive
void HudType800::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    const HudObjectDef::List800Def& info = def->list800;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(active);
    }
    for (uint16_t i = 0; i < entryObjects.Count(); ++i)
        HideObject(entryObjects[i]);
    for (int i = 0; i < 6; ++i) {
        HideObject(rows[i]);
        HideObject(rowExtraA[i]);
        HideObject(rowExtraB[i]);
    }
    // The running episode's instance goes with the element.
    game::JobManager* jobs = game::GetJobManager();
    const uint32_t period = (jobs && jobs->currentJob) ? jobs->currentJob->field_2D4 : kDefaultPeriod;
    int index = 0;
    for (int i = 0; i < 7; ++i) {
        if (kEpisodePeriods[i] == period) {
            index = i;
            break;
        }
    }
    if (ape::world::ApeObj* instance = ape::world::FindInstance(kPeriodInstances800[index], 0))
        instance->SetVisible(active, true);
}

// @func 0x0037965C HudType800::Refresh
void HudType800::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    listLinked = nullptr;
    listHoldTime = 0.0f;
    listFadeTime = -1.0f;
    text[0] = '\0';
    animState &= 1u;
    listFlags = RefreshListFlags(listFlags);
    if (listFlags & kE_LeaveB)
        QueueDestroy(this);
    selected = 0;
    field_108 = 0;
    cursorRow = def->list800.cursorRow;
    entries.Clear();
    entryFlags.Clear();
    ReleaseEntryObjects();
    entryObjects.Clear();
}

// @func 0x003772C0 HudType800::BeginShow
void HudType800::BeginShow()
{
    BeginListShow(this, clips, listFlags, def->list800.fields, def->list800.fieldCount);
}

// @func 0x00377D18 HudType800::Hide
void HudType800::Hide(bool a, bool b)
{
    if (ListBlocked(listFlags))
        return;
    // The upgrades the list showed are no longer new.
    if (entryFlags.Count() != 0 && entryFlags.Count() == entries.Count()) {
        for (uint16_t i = 0; i < entries.Count(); ++i) {
            if (UpgradeIsNew(entries[i]) && entryFlags[i])
                UpgradeClearNew(entries[i]);
        }
    }
    HideList(this, listFlags, clips, def->list800.fields, def->list800.fieldCount, a, b);
}

// TODO(0x00378540 HudType800::Update)
void HudType800::Update(float dt) { (void)dt; }

// @func 0x003798D8 HudType800::SetEnabled
void HudType800::SetEnabled(bool enabled, bool flag)
{
    if (!enabled)
        Hide(flag, false);
    else if (!(listFlags & kE_Shown))
        ShowTimed(-1.0f, false, nullptr, kNoTint);
}

namespace {

// Icon object of an upgrade by the character it belongs to (0x00F64D68).
struct UpgradeIcon {
    uint32_t character;
    uint32_t object;
};
const UpgradeIcon kUpgradeIcons[4] = {
    {0x00014120u /* "Sly" */, 0x9808BBA9u},
    {0x1D8C2817u /* "Bentley" */, 0x18D13620u},
    {0x88397820u /* "Murray" */, 0x568604F7u},
    {0xDF0611F9u, 0xE56342BFu},   // NOTE: character name unknown
};

// Row text fields (0x00F64DD0): name and price of rows A-F.
const char* const kRowFields[6][2] = {
    {"SlotAName", "SlotAPrice"}, {"SlotBName", "SlotBPrice"}, {"SlotCName", "SlotCPrice"},
    {"SlotDName", "SlotDPrice"}, {"SlotEName", "SlotEPrice"}, {"SlotFName", "SlotFPrice"},
};
constexpr uint32_t kField800_Empty = 0x001FF691u;     // shows the "no upgrades" text
constexpr uint32_t kField800_Cleared = 0x968EF501u;   // emptied with it

} // namespace

// qsort comparator of the upgrade list: new upgrades first, then by index.
// @func 0x00389C5C CompareUpgradesNewFirst
int CompareUpgradesNewFirst(const void* a, const void* b)
{
    const uint32_t ia = *static_cast<const uint32_t*>(a);
    const uint32_t ib = *static_cast<const uint32_t*>(b);
    const bool newA = (UpgradeState(ia) & kUpgrade_New) != 0;
    const bool newB = (UpgradeState(ib) & kUpgrade_New) != 0;
    if (newA == newB)
        return int(ia) - int(ib);
    return newB ? 1 : -1;
}

// @func 0x003780E0 HudType800::ShowTimed
void HudType800::ShowTimed(float fade, bool flag, HudObject* linkedTo, const uint8_t* color)
{
    if (ListBlocked(listFlags))
        return;
    if (flag)
        listFlags |= kE_LeaveA;
    for (int i = 0; i < 4; ++i)
        tint[i] = color[i];
    listFadeTime = fade;
    const uint32_t state = animState >> 1;
    if (state == kState_Idle || state == kState_Update) {
        listHoldTime = fade < 0.0f ? listDefaultFade : fade;
        animState = (animState & 1u) | (kState_Update << 1);
        PlayListState(this, clips, listFlags, kState_Update);
    } else if (state == 0 || state == kState_Outro) {
        listLinked = linkedTo;
        if (!linkedTo || !linkedTo->object || !linkedTo->object->IsVisible())
            BeginShow();
        // NOTE: the console drops the old entry objects without releasing them.
        entries.Clear();
        entryFlags.Clear();
        entryObjects.Clear();
        game::JobManager* jobs = game::GetJobManager();
        if (!jobs || !jobs->currentJob) {
            for (uint32_t i = 0; i < LoadedUpgradeCount(); ++i)
                entries.PushBack(i);
        } else {
            CollectAvailableUpgrades(entries);
            if (entries.Count() != 0)
                std::qsort(entries.Data(), entries.Count(), sizeof(uint32_t), CompareUpgradesNewFirst);
        }
        for (uint16_t i = 0; i < entries.Count(); ++i) {
            entryFlags.PushBack(0);
            ape::world::ApeObj* icon = nullptr;
            if (const UpgradeEntry* upgrade = GetUpgradeEntry(entries[i])) {
                uint32_t objectName = 0;
                for (const UpgradeIcon& entry : kUpgradeIcons) {
                    if (entry.character == upgrade->field_08) {
                        objectName = entry.object;
                        break;
                    }
                }
                if (objectName) {
                    if (ape::world::ApeObj* source = ape::world::FindGeomInstance(objectName))
                        icon = HudCloneObject(source);
                }
            }
            entryObjects.PushBack(icon);
        }
    }
    RefreshRows();
}

// @func 0x0037744C HudType800::RefreshRows
void HudType800::RefreshRows()
{
    const HudObjectDef::List800Def& info = def->list800;
    const uint32_t count = entries.Count();
    for (uint32_t row = 0; row < 6; ++row) {
        const uint32_t nameField = ape::HashNameNoCase(kRowFields[row][0]);
        const uint32_t priceField = ape::HashNameNoCase(kRowFields[row][1]);
        const int32_t offset = int32_t(row) - int32_t(cursorRow);
        const int32_t index = int32_t(selected) + offset;
        if (count != 0 && ((index >= 0 && index < int32_t(count)) || count > 5)) {
            // The list wraps around when it has more entries than rows.
            const uint32_t wrapped = (selected + count + uint32_t(offset)) % count;
            const uint32_t upgradeIndex = entries[wrapped];
            if (const UpgradeEntry* upgrade = GetUpgradeEntry(upgradeIndex)) {
                SetListFieldText(info.fields, info.fieldCount, nameField, upgrade->name);
                const char* price = (UpgradeState(upgradeIndex) & kUpgrade_Purchased)
                                        ? "$HIDEOUT_THIEFNET_SOLD"
                                        : ape::TempFormat("%d", upgrade->field_0C);
                SetListFieldText(info.fields, info.fieldCount, priceField, price);
            }
        } else {
            SetListFieldText(info.fields, info.fieldCount, nameField, "");
            SetListFieldText(info.fields, info.fieldCount, priceField, "");
        }
    }
    if (count == 0) {
        SetListFieldText(info.fields, info.fieldCount, kField800_Empty, "$THIEFNET_NO_UPGRADES_AVAILABLE");
        SetListFieldText(info.fields, info.fieldCount, kField800_Cleared, "");
    }
}

// ===========================================================================
// Type 0x1000

// @func 0x00358B64 HudType1000::HudType1000
HudType1000::HudType1000(HudObjectDef* def_) : HudObject(0, def_)
{
    const HudObjectDef::ListDef& info = def->list;
    for (int i = 0; i < 4; ++i)
        tint[i] = kNoTint[i];
    listDefaultFade = info.defaultFade;
    const bool persistent = (info.flags & 1) != 0;
    listFlags = (listFlags & 0x0007FFFFu) | (persistent ? (kE_Persistent | kE_Shown) : 0u);
    listFadeTime = -1.0f;
    listHoldTime = 0.0f;
    selected = 0;
    field_104 = 0;
    cursorRow = info.cursorRow;
    lastScroll = 0;
    typeFlags |= kHud_Type1000;
    text[0] = '\0';
    if (info.fieldCount) {
        fieldObjects = static_cast<ape::world::ApeObj**>(std::calloc(info.fieldCount, sizeof(ape::world::ApeObj*)));
        for (uint32_t i = 0; i < info.fieldCount; ++i)
            fieldObjects[i] = FindInHierarchy(object, info.fields[i].objectName);
    }
    MakeListClips(def, clips);
    titleB = 0;
    titleA = 1;
    for (ape::world::ApeObj*& row : rows)
        row = CopyGeometry(info.rowGeometry);
    for (int i = 0; i < 6; ++i) {
        episodeA[i] = ape::world::FindInstance(kEpisode1000A[i], 0);
        episodeB[i] = ape::world::FindInstance(kEpisode1000B[i], 0);
        episodeC[i] = ape::world::FindInstance(kEpisode1000C[i], 0);
        episodeD[i] = ape::world::FindInstance(kEpisode1000D[i], 0);
    }
    ape::world::ApeObj* sourceA = ape::world::FindInstance(kCopy1000A, 0);
    ape::world::ApeObj* sourceB = ape::world::FindInstance(kCopy1000B, 0);
    ape::world::ApeObj* sourceC = ape::world::FindInstance(kCopy1000C, 0);
    for (int i = 0; i < 6; ++i) {
        copiesA[i] = CopyInstance(sourceA);
        copiesB[i] = CopyInstance(sourceB);
        copiesC[i] = CopyInstance(sourceC);
    }
    objects[0] = FindInHierarchy(object, info.knobName);
    objects[1] = FindInHierarchy(object, info.barTopName);
    objects[2] = FindInHierarchy(object, info.barBottomName);
    objects[3] = FindInHierarchy(object, info.field_A8);
}

// @func 0x0037A93C HudType1000::~HudType1000
// @func 0x0037AC20 HudType1000::~HudType1000 (deleting)
HudType1000::~HudType1000()
{
    if (g_game) {
        g_game->RemoveCallback(kCallbackList_Draw, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Place, this, nullptr);
    }
    HudType1000::SetActive(false);
    std::free(fieldObjects);
    fieldObjects = nullptr;
    for (ape::world::ApeObj*& row : rows) {
        if (row)
            row->Release();
    }
    for (int i = 0; i < 6; ++i) {
        ReleaseObject(copiesA[i]);
        ReleaseObject(copiesB[i]);
        ReleaseObject(copiesC[i]);
    }
}

// @func 0x0037A58C HudType1000::SetActive
void HudType1000::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    const HudObjectDef::ListDef& info = def->list;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(active);
    }
    if (selected < 6)
        HideObject(episodeD[selected]);
    for (ape::world::ApeObj* row : rows)
        HideObject(row);
    for (int i = 0; i < 6; ++i) {
        HideObject(episodeA[i]);
        HideObject(episodeB[i]);
        HideObject(episodeC[i]);
        HideObject(episodeD[i]);
        HideObject(copiesA[i]);
        HideObject(copiesB[i]);
        HideObject(copiesC[i]);
    }
    if (!active && g_game)
        g_game->RemoveCallback(kCallbackList_Place, this, nullptr);
}

// @func 0x0037DC70 HudType1000::Refresh
void HudType1000::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    listLinked = nullptr;
    listHoldTime = 0.0f;
    animState &= 1u;
    listFadeTime = -1.0f;
    text[0] = '\0';
    listFlags = RefreshListFlags(listFlags);
    if (listFlags & kE_LeaveB)
        QueueDestroy(this);
}

// @func 0x0037CCA8 HudType1000::Update
void HudType1000::Update(float dt)
{
    if (ListBlocked(listFlags))
        return;
    if ((animState >> 1) == 0 && !(listFlags & (kE_ShowReq | kE_HideReq)) && !listLinked)
        return;
    if (object && object->IsVisible() && g_game) {
        g_game->AddCallback(kCallbackList_Draw, reinterpret_cast<void*>(&DrawFieldsCallback), this, nullptr);
        g_game->AddCallback(kCallbackList_Place, reinterpret_cast<void*>(&PlaceRowsCallback), this, nullptr);
    }
    const HudObjectDef::ListDef& info = def->list;

    // Requests made since the last update.
    if (listFlags & kE_ShowReq) {
        const char* current = nullptr;
        if (info.fieldCount && info.fields[0].label)
            current = static_cast<ape::ApeGuiLabel*>(info.fields[0].label)->GetText();
        ShowTimed(-1.0f, current, false, nullptr, kNoTint);
        if (info.fieldCount && info.fields[0].label) {
            info.fields[0].label->SetText(text, false);
            info.fields[0].label->Invalidate();
        }
        text[0] = '\0';
        listFlags &= ~kE_ShowReq;
    } else if (listFlags & kE_HideReq) {
        Hide((listFlags & kE_LeaveA) != 0, (listFlags & kE_LeaveB) != 0);
        listFlags &= ~kE_HideReq;
    }

    // Label colours: the element's tint, else the field's own colour or the
    // player's; geometry elements pass their alpha on.
    if (info.fieldCount) {
        ape::world::ApeObj* geometry =
            (object && object->GetType() == ape::world::kObjType_GeomInstance) ? object : nullptr;
        const bool tinted = tint[3] != 0;
        for (uint32_t i = 0; i < info.fieldCount; ++i) {
            ape::ApeGuiObject* label = info.fields[i].label;
            if (!label)
                continue;
            const HudObjectDef::ListFieldDef& field = info.fields[i];
            ape::xml::Color8 color = label->GetColor();
            if (tinted) {
                color = {tint[0], tint[1], tint[2], tint[3]};
            } else if ((field.flags & 0x80000000u) && PlayerHudColorAvailable()) {
                uint8_t player[4];
                PlayerHudColor(player);
                color = {player[0], player[1], player[2], player[3]};
            } else if (field.color[3]) {
                color = {field.color[0], field.color[1], field.color[2], field.color[3]};
            }
            if (geometry)
                color.a = geometry->m_byteA8;
            label->SetColor(color);
        }
    }

    switch (animState >> 1) {
    case kState_Outro:
        if (ListAnimDone()) {
            HudObject* next = listLinked;
            Refresh();
            if (next)
                BeginShow();
        }
        break;
    case kState_ScrollUp:
    case kState_ScrollDown: {
        // Presses during the animation are kept for the next idle frame.
        const uint32_t pressed = PressedButtons();
        if (pressed & kInput_Up)
            listFlags = (listFlags & ~kE_QueuedDown) | kE_QueuedUp;
        else if (pressed & kInput_Down)
            listFlags = (listFlags & ~kE_QueuedUp) | kE_QueuedDown;
        if (HudClipDone(animController)) {
            lastScroll = animState >> 1;
            animState = (animState & 1u) | (kState_Idle << 1);
        }
        break;
    }
    case kState_Idle: {
        if (!offsetTween.IsFinished()) {
            offsetTween.Update(dt);
            if (object) {
                ape::Matrix34 local = object->GetLocal();
                local.translation.x = offset.x;
                local.translation.y = offset.y;
                local.translation.z = offset.z;
                object->SetLocal(local);
                object->MarkDirty();
            }
        }
        const uint32_t pressed = PressedButtons();
        if (pressed & kInput_Accept) {
            // Travel to the hideout of the episode (its second part first).
            game::JobManager* jobs = game::GetJobManager();
            const game::Job* part1 = jobs->FindJobById(kEpisodeJobs[selected]);
            const game::Job* part2 = jobs->FindJobById(kEpisodeSecondJobs[selected]);
            auto canTravel = [](const game::Job* job) {
                return job && job->ProgressState() >= 1 && job->nameHash != 0 &&
                       job->nameHash != game::RunningLevel();
            };
            const game::Job* target = canTravel(part2) ? part2 : (canTravel(part1) ? part1 : nullptr);
            auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
            if (!target) {
                if (script)
                    script->SetStateAndUpdate(kScript_Unavailable);
                break;
            }
            if (script)
                script->SetStateAndUpdate(kScript_Purchase);
            jobs->SelectJobByValue(target->field_2D4);
            game::GameWorldRequestLevel(target->nameHash);
        } else if (!(pressed & kInput_Up) && !(listFlags & kE_QueuedUp)) {
            if (!(pressed & kInput_Down) && !(listFlags & kE_QueuedDown))
                break;
            if (lastScroll == 0)
                ++field_104;
            listFlags &= ~kE_QueuedDown;
            animState = (animState & 1u) | (kState_ScrollDown << 1);
            PlayScroll(kState_ScrollDown);
            const uint32_t old = selected;
            selected = old != 5 ? old + 1 : 0;
            titleB = selected;
            titleA = old;
            if (lastScroll == kState_ScrollUp)
                cursorRow = cursorRow != 5 ? cursorRow + 1 : 0;
            RefreshRows();
        } else {
            const uint32_t old = selected;
            animState = (animState & 1u) | (kState_ScrollUp << 1);
            listFlags &= ~kE_QueuedUp;
            PlayScroll(kState_ScrollUp);
            if (lastScroll == kState_ScrollUp)
                selected = selected != 0 ? selected - 1 : 5;
            if (lastScroll == kState_ScrollDown || lastScroll == 0) {
                cursorRow = cursorRow != 0 ? cursorRow - 1 : 5;
                selected = selected != 0 ? selected - 1 : 5;
            }
            titleB = old;
            titleA = selected;
            RefreshRows();
        }
        break;
    }
    case kState_Update:
    case kState_Intro:
        if (ListAnimDone()) {
            const bool intro = (animState >> 1) == kState_Intro;
            animState = (animState & 1u) | (kState_Idle << 1);
            listHoldTime = listFadeTime < 0.0f ? listDefaultFade : listFadeTime;
            listFlags = SetBit(listFlags, kE_AnimOk, PlayHudClip(animController, clips[kState_Idle], 1.0f, true));
            if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript))
                listFlags = SetBit(listFlags, kE_ScriptOk, script->SetStateAndUpdate(kListStateNames[kState_Idle]));
            if (intro) {
                for (uint32_t i = 0; i < info.fieldCount; ++i) {
                    if (!info.fields[i].object3d && info.fields[i].object)
                        info.fields[i].object->SetVisible(true);
                }
            }
        }
        break;
    case 0:
        if (listLinked && (!listLinked->object || !listLinked->object->IsVisible()))
            BeginShow();
        break;
    default:
        break;
    }

    // The selected episode's description.
    const game::Job* job = game::GetJobManager()->FindJobById(kEpisodeJobs[selected]);
    SetListFieldText(info.fields, info.fieldCount, kField_Description,
                     (job && job->ProgressState() >= 1) ? job->desc : "$STR_UNKNOWN");
    if (animController)
        animController->UpdateLayers(dt);
    if (!object || !object->IsVisible())
        return;
    const ape::Vec3& zero = ape::world::kWorldZero;
    if ((baseOffset.x == zero.x && baseOffset.y == zero.y && baseOffset.z == zero.z) || !HudPlacementEnabled())
        return;
    ape::Matrix34 local = object->GetLocal();
    local.translation.x = zero.x + baseOffset.x + offset.x;
    local.translation.y = zero.y + baseOffset.y + offset.y;
    local.translation.z = zero.z + baseOffset.z + offset.z;
    object->SetLocal(local);
    object->MarkDirty();
}

bool HudType1000::ListAnimDone() const
{
    if ((listFlags & kE_AnimOk) && !HudClipDone(animController))
        return false;
    if (listFlags & kE_ScriptOk) {
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
        if (!script || !script->IsFinished(false))
            return false;
    }
    return true;
}

void HudType1000::PlayScroll(uint32_t state)
{
    listFlags = SetBit(listFlags, kE_AnimOk, PlayHudClip(animController, clips[state], 2.0f, false));
    if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript))
        listFlags = SetBit(listFlags, kE_ScriptOk, script->SetStateAndUpdate(kListStateNames[state]));
}

// @func 0x0037AC70 HudType1000::BeginShow
void HudType1000::BeginShow()
{
    BeginListShow(this, clips, listFlags, def->list.fields, def->list.fieldCount);
}

// @func 0x0037C7E0 HudType1000::Hide
void HudType1000::Hide(bool a, bool b)
{
    if (ListBlocked(listFlags))
        return;
    HideList(this, listFlags, clips, def->list.fields, def->list.fieldCount, a, b);
}

// @func 0x0037CB10 HudType1000::ShowTimed
void HudType1000::ShowTimed(float fade, const char* text_, bool flag, HudObject* linkedTo, const uint8_t* color)
{
    (void)text_;
    if (ListBlocked(listFlags))
        return;
    if (flag)
        listFlags |= kE_LeaveA;
    for (int i = 0; i < 4; ++i)
        tint[i] = color[i];
    listFadeTime = fade;
    const uint32_t state = animState >> 1;
    if (state == kState_Idle || state == kState_Update) {
        listHoldTime = fade < 0.0f ? listDefaultFade : fade;
        animState = (animState & 1u) | (kState_Update << 1);
        PlayListState(this, clips, listFlags, kState_Update);
    } else if (state == 0 || state == kState_Outro) {
        listLinked = linkedTo;
        if (!linkedTo || !linkedTo->object || !linkedTo->object->IsVisible())
            BeginShow();
    }
    RefreshRows();
}

namespace {


// Row fields of the 0x1000 element.
constexpr uint32_t kField_TitleA = 0x93831D09u;          // "Title A"
constexpr uint32_t kField_TitleB = 0x93831D0Au;          // "Title b"
constexpr uint32_t kField_GameProgress = 0x53D9E41Fu;    // "GAME Progress"
constexpr uint32_t kField_EpisodeProgress = 0x0EE9F108u; // "EPISODE Progress"
constexpr uint32_t kField_Bottles = 0xDE006FF7u;
constexpr uint32_t kField_BottleCount = 0xF1FB3454u;
constexpr uint32_t kField_Treasures = 0x0BBAEF6Eu;
constexpr uint32_t kField_TreasureCount = 0x9B13B20Bu;
constexpr uint32_t kField_Safe = 0xC1CE33D0u;
constexpr uint32_t kField_SafeCount = 0x196765EDu;
constexpr uint32_t kField_Masks = 0xF17AEB11u;
constexpr uint32_t kField_MaskCount = 0x10C350EEu;

const char* const kNotAvailableValue = "$STR_NOT_AVAILABLE_01";
const char* const kNotAvailableTotal = "$STR_NOT_AVAILABLE_02";

// Name shown for episode `index`: unknown until started; the first episode
// uses the level name text.
const char* EpisodeTitle(uint32_t index)
{
    const game::Job* job = game::GetJobManager()->FindJobById(kEpisodeJobs[index]);
    if (!job || job->ProgressState() < 1)
        return "$STR_UNKNOWN";
    if (index == 0)
        return "$ParisLevelName";
    return job->name;
}

const char* CountText(bool available, int value, const char* otherwise)
{
    return available ? ape::TempFormat("%d", value) : otherwise;
}

int Percent(float value) { return static_cast<int>(std::floor(value)); }

} // namespace

// @func 0x0037AE00 HudType1000::RefreshRows
void HudType1000::RefreshRows()
{
    const HudObjectDef::ListFieldDef* fields = def->list.fields;
    const uint32_t fieldCount = def->list.fieldCount;
    auto setText = [&](uint32_t name, const char* text) { SetListFieldText(fields, fieldCount, name, text); };

    setText(kField_TitleA, EpisodeTitle(titleA));
    setText(kField_TitleB, EpisodeTitle(titleB));

    game::JobManager* jobs = game::GetJobManager();
    const game::Job* part1 = jobs->FindJobById(kEpisodeJobs[selected]);
    const game::Job* part2 = jobs->FindJobById(kEpisodeSecondJobs[selected]);
    if (!part1)
        return;
    const bool started = part1->ProgressState() > 0;
    setText(kField_GameProgress,
            ape::TempFormat("$STR_GAME_COMPLETE %d$STR_PERCENTAGE", Percent(jobs->GameCompletion(nullptr))));

    if (!part2 || part2->ProgressState() < 1) {
        setText(kField_EpisodeProgress,
                ape::TempFormat("$STR_EPISODE_COMPLETE %d$STR_PERCENTAGE", Percent(part1->Completion())));
        const bool hasTreasures = started && part1->TreasureCount() != 0;
        setText(kField_Bottles, CountText(hasTreasures, part1->CollectedBottles(), kNotAvailableValue));
        setText(kField_BottleCount, CountText(hasTreasures, game::g_defaultBottleCount, kNotAvailableTotal));
        setText(kField_Treasures, CountText(hasTreasures, part1->CollectedTreasures(), kNotAvailableValue));
        setText(kField_TreasureCount, CountText(hasTreasures, part1->TreasureCount(), kNotAvailableTotal));
        setText(kField_Safe, CountText(hasTreasures, part1->SafeState() == 3 ? 1 : 0, kNotAvailableValue));
        setText(kField_SafeCount, CountText(hasTreasures, 1, kNotAvailableTotal));
        setText(kField_Masks, CountText(started, part1->CollectedMasks(), kNotAvailableValue));
        setText(kField_MaskCount, CountText(started, part1->MaskCount(), kNotAvailableTotal));
        return;
    }

    // Two-part episode: the completion is the mean of both parts and the
    // counts are summed (the safe is the first part's).
    const float completion = (part1->Completion() + part2->Completion()) * 0.5f;
    setText(kField_EpisodeProgress, ape::TempFormat("$STR_EPISODE_COMPLETE %d$STR_PERCENTAGE", Percent(completion)));
    const bool hasTreasures = started && (part1->TreasureCount() + part2->TreasureCount()) != 0;
    setText(kField_Bottles,
            CountText(hasTreasures, part1->CollectedBottles() + part2->CollectedBottles(), kNotAvailableValue));
    setText(kField_BottleCount, CountText(hasTreasures, game::g_defaultBottleCount * 2, kNotAvailableTotal));
    setText(kField_Treasures,
            CountText(hasTreasures, part1->CollectedTreasures() + part2->CollectedTreasures(), kNotAvailableValue));
    setText(kField_TreasureCount,
            CountText(hasTreasures, part1->TreasureCount() + part2->TreasureCount(), kNotAvailableTotal));
    setText(kField_Safe, CountText(hasTreasures, part1->SafeState() == 3 ? 1 : 0, kNotAvailableValue));
    setText(kField_SafeCount, CountText(hasTreasures, 1, kNotAvailableTotal));
    setText(kField_Masks, CountText(started, part1->CollectedMasks() + part2->CollectedMasks(), kNotAvailableValue));
    setText(kField_MaskCount, CountText(started, part1->MaskCount() + part2->MaskCount(), kNotAvailableTotal));
}

// @func 0x0037DED8 HudType1000::DrawFields
void HudType1000::DrawFields()
{
    const HudObjectDef::ListDef& info = def->list;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        ape::ApeGuiObject* field = info.fields[i].object3d;
        const ape::world::ApeObj* anchor = fieldObjects[i];
        if (!field || !anchor || !anchor->IsVisible())
            continue;
        // The GUI's x and y axes point the other way from the scene's.
        ape::Matrix34 transform = anchor->GetWorld();
        transform.xAxis = {-transform.xAxis.x, -transform.xAxis.y, -transform.xAxis.z, 0.0f};
        transform.yAxis = {-transform.yAxis.x, -transform.yAxis.y, -transform.yAxis.z, 0.0f};
        field->SetTransform(transform);
        field->Tick(0.0f);
        field->Draw();
    }
}

// @func 0x0037E0D8 HudType1000::DrawFieldsCallback
void HudType1000::DrawFieldsCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudType1000*>(context)->DrawFields();
}

// @func 0x0037E108 HudType1000::PlaceRows
void HudType1000::PlaceRows()
{
    const HudObjectDef::ListDef& info = def->list;
    game::JobManager* jobs = game::GetJobManager();
    // Episode shown on visible row `row`.
    auto rowEpisode = [this](uint32_t row) { return (selected + row - cursorRow + 6) % 6; };
    auto completed = [jobs](uint32_t episodeName) {
        const game::Episode* episode = jobs->FindEpisode(episodeName);
        return episode && episode->State() == 2;
    };
    auto unlocked = [jobs](uint32_t jobId) {
        const game::Job* job = jobs->FindJobById(jobId);
        return !jobs->currentJob || (job && job->ProgressState() > 0);
    };
    auto place = [](ape::world::ApeObj* target, const ape::world::ApeObj* locator) {
        target->SetVisible(true, true);
        SetLocalFromWorld(target, locator);
    };

    // Row copies: shown for the completed episodes.
    for (uint32_t row = 0; row < 4; ++row) {
        ape::world::ApeObj* geometry = rows[row];
        if (geometry)
            geometry->SetVisible(false, true);
        if (!geometry || row >= info.rowCount)
            continue;
        geometry->SetVisible(completed(kEpisodeRowNames[rowEpisode(row)]), true);
        if (const ape::world::ApeObj* locator = FindInHierarchy(object, info.rowLocators[row]))
            SetLocalFromWorld(geometry, locator);
    }

    for (uint32_t i = 0; i < 6; ++i) {
        for (ape::world::ApeObj* item : {episodeA[i], episodeB[i], episodeC[i], episodeD[i], copiesA[i], copiesB[i],
                                         copiesC[i]}) {
            if (item)
                item->SetVisible(false, true);
        }
    }

    // The two title slots: the unlocked or the locked look of their episode.
    for (uint32_t slot = 0; slot < 2; ++slot) {
        const uint32_t index = slot == 0 ? titleA : titleB;
        if (const ape::world::ApeObj* locator = FindInHierarchy(object, kTitleLocatorsA[slot])) {
            ape::world::ApeObj* target = unlocked(kEpisodeJobs[index]) ? episodeA[index] : copiesA[index];
            if (target)
                place(target, locator);
        }
        if (const ape::world::ApeObj* locator = FindInHierarchy(object, kTitleLocatorsB[slot])) {
            ape::world::ApeObj* target = unlocked(kEpisodeLastPartJobs[index]) ? episodeB[index] : copiesB[index];
            if (target)
                place(target, locator);
        }
    }

    // The visible rows' episode objects.
    for (uint32_t row = 0; row < 4; ++row) {
        const uint32_t index = rowEpisode(row);
        const bool done = completed(kEpisodeNames[index]);
        if (const ape::world::ApeObj* locator = FindInHierarchy(object, kRowLocators[row])) {
            ape::world::ApeObj* target = done ? episodeC[index] : copiesC[index];
            if (target)
                place(target, locator);
        }
    }

    // The knob slides from the bar's top to its bottom with the selection.
    ape::world::ApeObj* knob = objects[0];
    if (knob && objects[1] && objects[2]) {
        const ape::Vec3& top = objects[1]->GetWorld().translation;
        const ape::Vec3& bottom = objects[2]->GetWorld().translation;
        const float t = float(selected) * 0.2f;
        const ape::Vec3 position{top.x + (bottom.x - top.x) * t, top.y + (bottom.y - top.y) * t,
                                 top.z + (bottom.z - top.z) * t, 0.0f};
        ape::Matrix34 world;
        ape::Matrix34FromRotationTranslation(world, knob->GetWorld(), position);
        knob->m_world = world;
        knob->m_flags &= ~ape::world::kObjFlag_Dirty;
    }
    // The selected episode's own object at its locator.
    if (objects[3] && episodeD[selected])
        place(episodeD[selected], objects[3]);
}

// @func 0x0037EC94 HudType1000::PlaceRowsCallback
void HudType1000::PlaceRowsCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudType1000*>(context)->PlaceRows();
}

// @func 0x0037DE50 HudType1000::SetEnabled
void HudType1000::SetEnabled(bool enabled, bool flag)
{
    if (!enabled)
        Hide(flag, false);
    else if (!(listFlags & kE_Shown))
        ShowTimed(-1.0f, nullptr, false, nullptr, kNoTint);
}

// ===========================================================================
// Type 0x10000

// TODO(0x00356BA0 HudType10000::HudType10000): only the base element and the
// type bit.
HudType10000::HudType10000(HudObjectDef* def_) : HudObject(0, def_)
{
    typeFlags |= kHud_Type10000;
}

} // namespace hud
} // namespace sly
