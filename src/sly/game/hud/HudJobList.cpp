// openTIT - Job list HUD element (type bit 0x2000), see HudJobList.h.
#include "sly/game/hud/HudJobList.h"

#include "sly/anim/AnimController.h"
#include "sly/game/AppRuntime.h"
#include "sly/game/AudioSystem.h"
#include "sly/game/ObjectHelpers.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"

#include <cstdio>
#include <cstdlib>

namespace sly {
namespace hud {

namespace {

// Anim script states of the list states (0x00F64FFC).
const uint32_t kListStateNames[7] = {
    0x0024A738u,   // "NONE"
    0x04296E4Cu,   // "Intro"
    0xC2361368u,   // "OnUpdate"
    0x002237D4u,   // "Idle"
    0x16AD6628u,   // "ScrollUp"
    0x20E4C8EFu,   // "ScrollDown"
    0x048129EBu,   // "Outro"
};
// Field names of the six rows (0x00F64F30): "SlotAName".."SlotFName".
const uint32_t kRowFieldNames[6] = {
    0xCF25170Eu, 0xCF332E8Fu, 0xCF414610u, 0xCF4F5D91u, 0xCF5D7512u, 0xCF6B8C93u,
};
constexpr uint32_t kField_Description = 0x001FF691u;   // "DESC"
// Script states played when the accept button picks an entry.
constexpr uint32_t kScript_Purchase = 0x968EF501u;      // "Purchase": the job starts
constexpr uint32_t kScript_Unavailable = 0x23200439u;   // NOTE: name unknown; the entry cannot start
// Default label tint: opaque white (0x01164CA4).
const uint8_t kWhite[4] = {0xFF, 0xFF, 0xFF, 0xFF};
// World callback lists the element draws and places itself on.
constexpr int kCallbackList_Draw = 8;
constexpr int kCallbackList_Place = 3;
// Pad bits of the local player's input block.
constexpr uint32_t kInput_Up = 0x01;
constexpr uint32_t kInput_Down = 0x02;
constexpr uint32_t kInput_Accept = 0x80;

uint32_t ClipName(const char* prefix, const char* suffix)
{
    char clip[0x60];
    std::snprintf(clip, sizeof clip, "%s_%s", prefix, suffix);
    return ape::HashNameNoCase(clip);
}

// Buttons the first local player pressed this frame (application player
// block +0xB8 & +0xBC).
uint32_t PressedButtons()
{
    const AppPlayer& player = GetAppRuntime().players[0];
    return player.inputFlags & player.inputMask;
}

uint32_t SetBit(uint32_t word, uint32_t bit, bool on) { return on ? (word | bit) : (word & ~bit); }

void SetGeometryFromWorld(ape::world::ApeObj* geometry, const ape::world::ApeObj* locator)
{
    ape::Matrix34 local = locator->GetWorld();
    geometry->SetLocal(local);
    geometry->MarkDirty();
    geometry->UpdateWorld(false, false);
}

} // namespace

// @func 0x00358760 HudJobList::HudJobList
HudJobList::HudJobList(HudObjectDef* def_) : HudObject(0, def_)
{
    const HudObjectDef::ListDef& info = def->list;
    for (uint8_t& c : tint)
        c = kWhite[0];
    listDefaultFade = info.defaultFade;
    const bool persistent = (info.flags & 1) != 0;
    listFlags = (listFlags & 0x0007FFFFu) | (persistent ? (kL_Persistent | kL_Shown) : 0u);
    listFadeTime = -1.0f;
    listHoldTime = 0.0f;
    selected = 0;
    cursorRow = info.cursorRow;
    lastScroll = 0;
    typeFlags |= kHud_Type2000;
    text[0] = '\0';
    if (info.fieldCount) {
        fieldObjects = static_cast<ape::world::ApeObj**>(std::calloc(info.fieldCount, sizeof(ape::world::ApeObj*)));
        for (uint32_t i = 0; i < info.fieldCount; ++i)
            fieldObjects[i] = FindInHierarchy(object, info.fields[i].objectName);
    }
    const char* prefix = def->animPrefix[0] ? def->animPrefix : def->defName;
    if (prefix[0]) {
        clips[kList_Intro] = ClipName(prefix, "Intro");
        clips[kList_Update] = ClipName(prefix, "OnUpdate");
        clips[kList_Idle] = ClipName(prefix, "Idle");
        clips[kList_Outro] = ClipName(prefix, "Outro");
        clips[kList_ScrollUp] = ClipName(prefix, "ScrollUp");
        clips[kList_ScrollDown] = ClipName(prefix, "ScrollDn");
    }
    // Every row gets its own hidden copy of the row geometry.
    for (ape::world::ApeObj*& row : rows) {
        row = nullptr;
        if (ape::world::ApeObj* source = ape::world::FindGeomInstance(info.rowGeometry)) {
            row = HudCloneObject(source);
            row->SetVisible(false, true);
        }
    }
    knob = FindInHierarchy(object, info.knobName);
    barTop = FindInHierarchy(object, info.barTopName);
    barBottom = FindInHierarchy(object, info.barBottomName);
}

// @func 0x0037EE20 HudJobList::~HudJobList
// @func 0x0037F150 HudJobList::~HudJobList (deleting)
HudJobList::~HudJobList()
{
    if (g_game) {
        g_game->RemoveCallback(kCallbackList_Draw, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Place, this, nullptr);
    }
    if (object)
        object->SetVisible(false, true);
    const HudObjectDef::ListDef& info = def->list;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(false);
    }
    for (ape::world::ApeObj* row : rows) {
        if (row)
            row->SetVisible(false, true);
    }
    std::free(fieldObjects);
    fieldObjects = nullptr;
    for (ape::world::ApeObj*& row : rows) {
        if (row)
            row->Release();
        row = nullptr;
    }
}

// @func 0x0037ECC0 HudJobList::SetActive
void HudJobList::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    const HudObjectDef::ListDef& info = def->list;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(active);
    }
    if (!active) {
        for (ape::world::ApeObj* row : rows) {
            if (row)
                row->SetVisible(false, true);
        }
    }
}

void HudJobList::PlayListState(uint32_t state, float speed, bool loop)
{
    listFlags = SetBit(listFlags, kL_AnimOk, PlayHudClip(animController, clips[state], speed, loop));
    if (animScript) {
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
        listFlags = SetBit(listFlags, kL_ScriptOk, script->SetStateAndUpdate(kListStateNames[state]));
    }
}

bool HudJobList::ListAnimDone() const
{
    if ((listFlags & kL_AnimOk) && !HudClipDone(animController))
        return false;
    if (listFlags & kL_ScriptOk) {
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
        if (!script || !script->IsFinished(false))
            return false;
    }
    return true;
}

// @func 0x0037F1A0 HudJobList::BeginShow
void HudJobList::BeginShow()
{
    listFlags |= kL_Shown;
    animState = (animState & 1u) | (kList_Intro << 1);
    if (object)
        object->SetVisible(true, true);
    PlayListState(kList_Intro, 1.0f, false);
    // Fields drawn in 3D keep their 2D object hidden.
    const HudObjectDef::ListDef& info = def->list;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(info.fields[i].object3d != nullptr);
    }
}

void HudJobList::SetFieldText(uint32_t fieldName, const char* value)
{
    if (fieldName == 0 || !value)
        return;
    const HudObjectDef::ListDef& info = def->list;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (info.fields[i].name != fieldName)
            continue;
        if (ape::ApeGuiObject* label = info.fields[i].label) {
            label->SetText(value, false);
            label->Invalidate();
        }
        return;
    }
}

// @func 0x0037F32C HudJobList::RefreshRowTexts
void HudJobList::RefreshRowTexts()
{
    const uint32_t count = uint32_t(entries.Count());
    if (count == 0)
        return;
    for (uint32_t row = 0; row < 6; ++row) {
        // Row `cursorRow` shows the selection; the list wraps around when it
        // has more entries than rows.
        const uint32_t index = selected + row - cursorRow;
        if (index < count || count > 5) {
            const uint32_t entry = (index + count) % count;
            if (entry < count)
                SetFieldText(kRowFieldNames[row], entries[entry].name);
        } else {
            SetFieldText(kRowFieldNames[row], "");
        }
    }
}

// @func 0x0037F5E8 HudJobList::Hide
void HudJobList::Hide(bool a, bool b)
{
    if (Blocked())
        return;
    const uint32_t state = animState >> 1;
    if (state == kList_Outro || state == kList_Hidden) {
        if ((!(listFlags & kL_LeaveA) && !a) || state != kList_Hidden) {
            if (((listFlags & kL_LeaveB) || b) && state == kList_Hidden && !*Hud().hideList.Find(RegisteredName()))
                Hud().hideList.Add(RegisteredName(), this);
        } else {
            Refresh();
        }
    } else {
        listFlags = (listFlags & ~(kL_LeaveA | kL_LeaveB)) | (a ? kL_LeaveA : 0u) | (b ? kL_LeaveB : 0u);
        listFadeTime = -1.0f;
        animState = (animState & 1u) | (kList_Outro << 1);
        PlayListState(kList_Outro, 1.0f, false);
        if (object)
            object->SetVisible(true, true);
        const HudObjectDef::ListDef& info = def->list;
        for (uint32_t i = 0; i < info.fieldCount; ++i) {
            if (ape::ApeGuiObject* field = info.fields[i].object)
                field->SetVisible(info.fields[i].object3d != nullptr);
        }
    }
    listLinked = nullptr;
}

// @func 0x0037F918 HudJobList::ShowTimed
void HudJobList::ShowTimed(float fade, const char* text_, bool flag, HudObject* linkedTo, const uint8_t* color)
{
    (void)text_;
    if (Blocked())
        return;
    if (flag)
        listFlags |= kL_LeaveA;
    for (int i = 0; i < 4; ++i)
        tint[i] = color[i];
    listFadeTime = fade;
    const uint32_t state = animState >> 1;
    if (state == kList_Idle || state == kList_Update) {
        listHoldTime = fade < 0.0f ? listDefaultFade : fade;
        animState = (animState & 1u) | (kList_Update << 1);
        PlayListState(kList_Update, 1.0f, false);
    } else if (state == kList_Hidden || state == kList_Outro) {
        listLinked = linkedTo;
        if (!linkedTo || !linkedTo->object || !linkedTo->object->IsVisible())
            BeginShow();
    }
    RefreshRowTexts();
}

// @func 0x00380C70 HudJobList::SetEnabled
void HudJobList::SetEnabled(bool enabled, bool flag)
{
    if (!enabled)
        Hide(flag, false);
    else if (!(listFlags & kL_Shown))
        ShowTimed(-1.0f, nullptr, false, nullptr, kWhite);
}

// @func 0x00380A8C HudJobList::Refresh
void HudJobList::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    listLinked = nullptr;
    listHoldTime = 0.0f;
    animState &= 1u;
    listFadeTime = -1.0f;
    text[0] = '\0';
    // A persistent element stays shown unless it was asked to leave.
    const uint32_t old = listFlags;
    uint32_t flags = old & 0x927FFFFFu;
    if (!(old & kL_LeaveA))
        flags = (old & 0xB27FFFFFu) | ((old & kL_Persistent) ? kL_Shown : 0u);
    listFlags = flags;
    if ((listFlags & kL_LeaveB) && !*Hud().hideList.Find(RegisteredName()))
        Hud().hideList.Add(RegisteredName(), this);
}

// @func 0x0037FAB0 HudJobList::Update
void HudJobList::Update(float dt)
{
    if (Blocked())
        return;
    if ((animState >> 1) == kList_Hidden && !(listFlags & kL_ShowReq) && !(listFlags & kL_HideReq) && !listLinked)
        return;
    if (g_game) {
        g_game->AddCallback(kCallbackList_Draw, reinterpret_cast<void*>(&DrawFieldsCallback), this, nullptr);
        g_game->AddCallback(kCallbackList_Place, reinterpret_cast<void*>(&PlaceRowsCallback), this, nullptr);
    }
    const HudObjectDef::ListDef& info = def->list;

    // Requests made since the last update.
    if (listFlags & kL_ShowReq) {
        const char* current = nullptr;
        if (info.fieldCount && info.fields[0].label)
            current = static_cast<ape::ApeGuiLabel*>(info.fields[0].label)->GetText();
        ShowTimed(-1.0f, current, false, nullptr, kWhite);
        if (info.fieldCount && info.fields[0].label) {
            info.fields[0].label->SetText(text, false);
            info.fields[0].label->Invalidate();
        }
        text[0] = '\0';
        listFlags &= ~kL_ShowReq;
    } else if (listFlags & kL_HideReq) {
        Hide((listFlags & kL_LeaveA) != 0, (listFlags & kL_LeaveB) != 0);
        listFlags &= ~kL_HideReq;
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

    const uint32_t count = uint32_t(entries.Count());
    const uint32_t last = count ? count - 1 : 0;
    switch (animState >> 1) {
    case kList_Outro:
        if (ListAnimDone()) {
            HudObject* next = listLinked;
            Refresh();
            if (next)
                BeginShow();
        }
        break;
    case kList_ScrollUp:
    case kList_ScrollDown: {
        // Presses during the animation are kept for the next idle frame.
        const uint32_t pressed = PressedButtons();
        if (pressed & kInput_Up)
            listFlags = (listFlags & ~kL_QueuedDown) | kL_QueuedUp;
        else if (pressed & kInput_Down)
            listFlags = (listFlags & ~kL_QueuedUp) | kL_QueuedDown;
        if (HudClipDone(animController)) {
            lastScroll = animState >> 1;
            animState = (animState & 1u) | (kList_Idle << 1);
        }
        break;
    }
    case kList_Idle: {
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
            RefreshRowTexts();
            const Entry& entry = entries[selected];
            if (!entry.available || entry.episodeName == 0) {
                if (animScript)
                    static_cast<ape::ApeAnimScriptInstance*>(animScript)->SetStateAndUpdate(kScript_Unavailable);
                break;
            }
            game::JobManager* jobs = game::GetJobManager();
            const uint32_t index = jobs->EpisodeIndex(entry.episodeName);
            if (index == 0xFFFFFFFFu)   // not found
                break;
            if (jobs->currentJob)
                game::SetProgressField14C(jobs->currentJob->field_2D4);
            jobs->SelectJobByValue(entry.jobValue);
            game::SetProgressFlag149(true);
            jobs->StartJob(index);
            goals::Goal* goal = game::PlayerGoalAt(0);
            audio::StopMusic(true);
            if (goal)
                jobs->PlayGoal(goal, true, false);
            if (animScript)
                static_cast<ape::ApeAnimScriptInstance*>(animScript)->SetStateAndUpdate(kScript_Purchase);
        } else if ((pressed & kInput_Up) || (listFlags & kL_QueuedUp)) {
            listFlags &= ~kL_QueuedUp;
            const uint32_t old = selected;
            if (lastScroll == kList_ScrollUp) {
                if (selected != 0)
                    --selected;
                else if (count > 5)
                    selected = last;
            }
            if (lastScroll == kList_ScrollDown || lastScroll == kList_Hidden) {
                if (selected != 0)
                    --selected;
                else if (count > 5)
                    selected = last;
                if (selected != old)
                    cursorRow = cursorRow != 0 ? cursorRow - 1 : last;
            }
            if (selected != old) {
                animState = (animState & 1u) | (kList_ScrollUp << 1);
                RefreshRowTexts();
                PlayListState(kList_ScrollUp, 2.0f, false);
            }
        } else if ((pressed & kInput_Down) || (listFlags & kL_QueuedDown)) {
            listFlags &= ~kL_QueuedDown;
            const uint32_t old = selected;
            if (selected != last)
                ++selected;
            else if (count > 5)
                selected = 0;
            if (lastScroll == kList_ScrollUp && selected != old)
                cursorRow = cursorRow != last ? cursorRow + 1 : 0;
            if (selected != old) {
                animState = (animState & 1u) | (kList_ScrollDown << 1);
                RefreshRowTexts();
                PlayListState(kList_ScrollDown, 2.0f, false);
            }
        }
        break;
    }
    case kList_Update:
    case kList_Intro:
        if (ListAnimDone()) {
            const bool intro = (animState >> 1) == kList_Intro;
            animState = (animState & 1u) | (kList_Idle << 1);
            listHoldTime = listFadeTime < 0.0f ? listDefaultFade : listFadeTime;
            PlayListState(kList_Idle, 1.0f, true);
            if (intro) {
                for (uint32_t i = 0; i < info.fieldCount; ++i) {
                    if (!info.fields[i].object3d && info.fields[i].object)
                        info.fields[i].object->SetVisible(true);
                }
            }
        }
        break;
    case kList_Hidden:
        if (listLinked && (!listLinked->object || !listLinked->object->IsVisible()))
            BeginShow();
        break;
    default:
        break;
    }

    if (selected < count && entries[selected].description)
        SetFieldText(kField_Description, entries[selected].description);
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

// @func 0x00380CF8 HudJobList::DrawFields
void HudJobList::DrawFields()
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

// @func 0x00380EF8 HudJobList::DrawFieldsCallback
void HudJobList::DrawFieldsCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudJobList*>(context)->DrawFields();
}

// @func 0x00380F28 HudJobList::PlaceRows
void HudJobList::PlaceRows()
{
    if (!object || !object->IsVisible())
        return;
    const HudObjectDef::ListDef& info = def->list;
    const uint32_t count = uint32_t(entries.Count());
    if (count != 0) {
        for (uint32_t row = 0; row < 6; ++row) {
            ape::world::ApeObj* geometry = rows[row];
            if (!geometry)
                continue;
            geometry->SetVisible(false, true);
            if (row >= info.rowCount)
                continue;
            const uint32_t index = selected + row - cursorRow;
            if (index >= count && count <= 5)
                continue;
            // The row's copy shows whether its entry can be started.
            const Entry& entry = entries[(index + count) % count];
            geometry->SetVisible(entry.available, true);
            if (const ape::world::ApeObj* locator = FindInHierarchy(object, info.rowLocators[row]))
                SetGeometryFromWorld(geometry, locator);
        }
    }
    // The knob slides from the bar's top to its bottom with the selection.
    if (knob && barTop && barBottom && count != 0) {
        const ape::Vec3& top = barTop->GetWorld().translation;
        const ape::Vec3& bottom = barBottom->GetWorld().translation;
        const float t = float(selected) * (1.0f / (float(count) - 1.0f));
        const ape::Vec3 position{top.x + (bottom.x - top.x) * t, top.y + (bottom.y - top.y) * t,
                                 top.z + (bottom.z - top.z) * t, 0.0f};
        ape::Matrix34 world;
        ape::Matrix34FromRotationTranslation(world, knob->GetWorld(), position);
        knob->m_world = world;
        knob->m_flags &= ~ape::world::kObjFlag_Dirty;
    }
}

// @func 0x003812F4 HudJobList::PlaceRowsCallback
void HudJobList::PlaceRowsCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudJobList*>(context)->PlaceRows();
}

} // namespace hud
} // namespace sly
