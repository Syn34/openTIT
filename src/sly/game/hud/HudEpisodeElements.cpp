// openTIT - HUD element kinds 0x800, 0x1000 and 0x10000, see
// HudEpisodeElements.h.
#include "sly/game/hud/HudEpisodeElements.h"

#include "sly/game/ObjectHelpers.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudHooks.h"

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

void Hide(ape::world::ApeObj* object)
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
        Hide(rowExtraA[i]);
        rowExtraB[i] = FindInHierarchy(object, kRowExtraB[i]);
        Hide(rowExtraB[i]);
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
        Hide(entryObjects[i]);
    for (int i = 0; i < 6; ++i) {
        Hide(rows[i]);
        Hide(rowExtraA[i]);
        Hide(rowExtraB[i]);
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
        Hide(episodeD[selected]);
    for (ape::world::ApeObj* row : rows)
        Hide(row);
    for (int i = 0; i < 6; ++i) {
        Hide(episodeA[i]);
        Hide(episodeB[i]);
        Hide(episodeC[i]);
        Hide(episodeD[i]);
        Hide(copiesA[i]);
        Hide(copiesB[i]);
        Hide(copiesC[i]);
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

// TODO(0x0037DE50 HudType1000::SetEnabled)
void HudType1000::SetEnabled(bool enabled, bool flag)
{
    (void)enabled;
    (void)flag;
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
