// openTIT - HUD element kinds 0x800, 0x1000 and 0x10000, see
// HudEpisodeElements.h.
#include "sly/game/hud/HudEpisodeElements.h"

#include "sly/game/ObjectHelpers.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSceneSetup.h"

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

// TODO(0x003798D8 HudType800::SetEnabled)
void HudType800::SetEnabled(bool enabled, bool flag)
{
    (void)enabled;
    (void)flag;
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
    field_10C = 0;
    typeFlags |= kHud_Type1000;
    text[0] = '\0';
    if (info.fieldCount) {
        fieldObjects = static_cast<ape::world::ApeObj**>(std::calloc(info.fieldCount, sizeof(ape::world::ApeObj*)));
        for (uint32_t i = 0; i < info.fieldCount; ++i)
            fieldObjects[i] = FindInHierarchy(object, info.fields[i].objectName);
    }
    MakeListClips(def, clips);
    field_190 = 0;
    field_194 = 1;
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

// TODO(0x0037CCA8 HudType1000::Update)
void HudType1000::Update(float dt) { (void)dt; }

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

// TODO(0x0037AE00 HudType1000::RefreshRows)
void HudType1000::RefreshRows() {}

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
