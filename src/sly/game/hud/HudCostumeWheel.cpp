// openTIT - Costume / upgrade wheel HUD element (type bit 0x400), see
// HudCostumeWheel.h.
#include "sly/game/hud/HudCostumeWheel.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"
#include "sly/game/AppRuntime.h"
#include "sly/game/ObjectHelpers.h"
#include "sly/tuning/HudTuning.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace sly {
namespace hud {

namespace {

constexpr uint32_t kGlowObject = 0x6D1B6072u;
constexpr uint32_t kField_Info = 0x00225CAEu;      // "Info"
constexpr uint32_t kField_Warning = 0x04293944u;   // NOTE: name unknown; costume warnings
constexpr float kPageOn = 180.0f;                  // turn angle of the selected page
// World callback lists of the wheel.
constexpr int kCallbackList_Draw = 8;
constexpr int kCallbackList_Pointer = 4;
constexpr int kCallbackList_Pages = 0;

uint32_t ClipName(const char* prefix, const char* suffix)
{
    char clip[0x60];
    std::snprintf(clip, sizeof clip, "%s_%s", prefix, suffix);
    return ape::HashNameNoCase(clip);
}

uint32_t SetBit(uint32_t word, uint32_t bit, bool on) { return on ? (word | bit) : (word & ~bit); }

// Script states of the wheel itself.
constexpr uint32_t kScript_Blocked = 0xF674815Bu;   // NOTE: name unknown; no other page can be chosen
// Script state per page (pages 0..4). NOTE: names unknown.
const uint32_t kPageStates[5] = {0x04B55E03u, 0x04B55E04u, 0x04B55E05u, 0x04B55E06u, 0x04B55E07u};
constexpr uint32_t kInput_Left = 0x04;
constexpr uint32_t kInput_Right = 0x08;

// The clip of the default layer is done (`ignoreLoop`: a looping clip counts
// as done after one pass); true without a controller (0x004A5D40).
bool ClipDone(anim::AnimController* controller, bool ignoreLoop)
{
    anim::AnimLayer* layer = controller ? controller->FindLayer(0) : nullptr;
    return layer ? layer->IsDone(ignoreLoop) : true;
}

uint32_t PressedButtons()
{
    const AppPlayer& player = GetAppRuntime().players[0];
    return player.inputFlags & player.inputMask;
}

} // namespace

// @func 0x003599C4 HudCostumeWheel::HudCostumeWheel
HudCostumeWheel::HudCostumeWheel(HudObjectDef* def_) : HudObject(0, def_)
{
    const HudObjectDef::WheelDef& info = def->wheel;
    page = 0;
    previousPage = 0;
    wheelHoldTime = 0.0f;
    wheelLinked = nullptr;
    pointerTarget = info.pointerName;
    wheelFlags = (wheelFlags & 0xC07FFFFFu) | kW_Shown;
    typeFlags |= kHud_Type400;
    for (uint32_t& clip : clips)
        clip = 0;
    const char* prefix = def->animPrefix[0] ? def->animPrefix : def->defName;
    if (prefix[0]) {
        clips[1] = ClipName(prefix, "Intro");
        clips[2] = ClipName(prefix, "OnUpdate");
        clips[3] = ClipName(prefix, "Idle");
        clips[4] = ClipName(prefix, "Outro");
    }
    pointer = ape::world::FindInstance(info.pointerInstance, 0);
    glow = FindInHierarchy(object, kGlowObject);
    if (info.fieldCount) {
        fieldObjects = static_cast<ape::world::ApeObj**>(std::calloc(info.fieldCount, sizeof(ape::world::ApeObj*)));
        for (uint32_t i = 0; i < info.fieldCount; ++i)
            fieldObjects[i] = FindInHierarchy(object, info.fields[i].objectName);
    }
    if (info.pageCount > 0)
        angles = static_cast<PageAngle*>(std::calloc(size_t(info.pageCount), sizeof(PageAngle)));
    glowObject = FindInHierarchy(object, info.glowName);
}

// @func 0x003747F4 HudCostumeWheel::~HudCostumeWheel
// @func 0x00374A8C HudCostumeWheel::~HudCostumeWheel (deleting)
HudCostumeWheel::~HudCostumeWheel()
{
    std::free(fieldObjects);
    fieldObjects = nullptr;
    if (g_game) {
        g_game->RemoveCallback(kCallbackList_Pointer, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Pages, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Draw, this, nullptr);
    }
    if (object)
        object->SetVisible(false, true);
    const HudObjectDef::WheelDef& info = def->wheel;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(false);
    }
    std::free(angles);
    angles = nullptr;
}

// @func 0x00374704 HudCostumeWheel::SetActive
void HudCostumeWheel::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    const HudObjectDef::WheelDef& info = def->wheel;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(active);
    }
}

// @func 0x003767C8 HudCostumeWheel::Refresh
void HudCostumeWheel::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    wheelLinked = nullptr;
    animState &= 1u;
    wheelHoldTime = 0.0f;
    // A persistent wheel stays shown; the requests and animation bits go.
    wheelFlags = (wheelFlags & 0x88FFFFFFu) | ((wheelFlags & kW_Persistent) ? kW_Shown : 0u);
    if (angles) {
        for (int i = 0; i < def->wheel.pageCount; ++i) {
            angles[i].current = 0.0f;
            angles[i].target = 0.0f;
        }
    }
    if (pointer)
        pointer->SetVisible(false, true);
    if (g_game) {
        g_game->RemoveCallback(kCallbackList_Pointer, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Pages, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Draw, this, nullptr);
    }
}

// @func 0x00376954 HudCostumeWheel::SetEnabled
void HudCostumeWheel::SetEnabled(bool enabled, bool flag)
{
    (void)flag;
    if (!enabled)
        Hide();
    else if (!(wheelFlags & kW_Shown))
        Show();
}

bool HudCostumeWheel::ItemAvailable(uint32_t item) const
{
    const uint32_t kind = def->wheel.kind;
    if (kind != kKind_Costume)
        return item == 0 || UpgradeUnlocked(kind, item);
    // A job that ties the player to a costume only offers that one.
    game::JobManager* jobs = game::GetJobManager();
    if (!jobs->currentJob || CostumeChoiceFree())
        return jobs->IsJobValueFlagged(item);
    return jobs->currentJob->field_32C == item && jobs->IsJobValueFlagged(item);
}

void HudCostumeWheel::PlayWheelState(uint32_t state, bool loop)
{
    wheelFlags = SetBit(wheelFlags, kW_AnimOk, PlayHudClip(animController, clips[state], 1.0f, loop));
    if (animScript) {
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
        wheelFlags = SetBit(wheelFlags, kW_ScriptOk, script->SetStateAndUpdate(kHudStateNames[state]));
    }
}

void HudCostumeWheel::SetFieldText(uint32_t fieldName, const char* value)
{
    const HudObjectDef::WheelDef& info = def->wheel;
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

void HudCostumeWheel::SetFieldsVisible()
{
    const HudObjectDef::WheelDef& info = def->wheel;
    for (uint32_t i = 0; i < info.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = info.fields[i].object)
            field->SetVisible(info.fields[i].object3d != nullptr);
    }
}

// @func 0x00367C20 HudCostumeWheel::BeginShow
void HudCostumeWheel::BeginShow()
{
    wheelFlags |= kW_Shown;
    animState = (animState & 1u) | (1u << 1);
    if (object)
        object->SetVisible(true, true);
    PlayWheelState(1, false);

    // Start on the page of the item in use.
    const HudObjectDef::WheelDef& info = def->wheel;
    uint32_t inUse = 0;
    if (info.kind == kKind_Costume)
        inUse = game::RunningLevelJobValue();
    else
        inUse = EquippedUpgrade(info.kind);
    page = 0;
    for (int i = 0; i < info.pageCount; ++i) {
        if (info.pages[i].item == inUse)
            page = i;
    }
    const HudObjectDef::WheelPageDef& current = info.pages[page];
    pointerTarget = current.pointerName;
    if (!ItemAvailable(current.item)) {
        SetFieldText(kField_Info, "");
        if (info.kind == kKind_Costume)
            SetFieldText(kField_Warning, "$STR_COST_NOT_AVAILABLE");
        angles[page].current = 0.0f;
        angles[page].target = 0.0f;
    } else {
        SetFieldText(kField_Info, current.text);
        SetFieldText(kField_Warning, "");
        angles[page].current = kPageOn;
        angles[page].target = kPageOn;
    }
}

// @func 0x00368360 HudCostumeWheel::Show
void HudCostumeWheel::Show()
{
    if ((wheelFlags & kW_Suppressed) || !(wheelFlags & kW_Shown))
        return;
    const uint32_t state = animState >> 1;
    if (state == 3 || state == 2) {
        wheelHoldTime = 0.0f;
        animState = (animState & 1u) | (2u << 1);
        PlayWheelState(2, false);
    } else if (state == 0 || state == 4) {
        BeginShow();
    }
    wheelFlags = SetBit(wheelFlags, kW_InputWasHeld, AppInputHeld());
    SetAppInputHeld(true);
    wheelFlags &= ~kW_Hiding;
}

// @func 0x003684B8 HudCostumeWheel::Hide
void HudCostumeWheel::Hide()
{
    if ((wheelFlags & kW_Suppressed) || !(wheelFlags & kW_Shown))
        return;
    const uint32_t state = animState >> 1;
    if (state != 4 && state != 0) {
        animState = (animState & 1u) | (4u << 1);
        PlayWheelState(4, false);
        if (object)
            object->SetVisible(true, true);
        SetFieldsVisible();
    }
    wheelLinked = nullptr;
    wheelFlags |= kW_Hiding;
    pointerTarget = def->wheel.pages[page].pointerName;
    SetAppInputHeld((wheelFlags & kW_InputWasHeld) != 0);
}

int HudCostumeWheel::FindAvailablePage(int from, int step) const
{
    const HudObjectDef::WheelDef& info = def->wheel;
    const int count = info.pageCount;
    auto next = [count, step](int p) { return step > 0 ? (p == count - 1 ? 0 : p + 1) : (p == 0 ? count - 1 : p - 1); };
    int target = next(from);
    while (target != from) {
        if (ItemAvailable(info.pages[target].item))
            break;
        target = next(target);
    }
    return target;
}

// @func 0x00374AF0 HudCostumeWheel::Update
void HudCostumeWheel::Update(float dt)
{
    if ((wheelFlags & kW_Suppressed) || !(wheelFlags & kW_Shown))
        return;
    if ((animState >> 1) == 0 && !(wheelFlags & kW_ShowReq) && !(wheelFlags & kW_HideReq) && !wheelLinked)
        return;
    if (wheelFlags & kW_ShowReq) {
        Show();
        wheelFlags &= ~kW_ShowReq;
    } else if (wheelFlags & kW_HideReq) {
        Hide();
        wheelFlags &= ~kW_HideReq;
    }
    if (g_game) {
        g_game->AddCallback(kCallbackList_Pointer, reinterpret_cast<void*>(&UpdatePointerCallback), this, nullptr);
        g_game->AddCallback(kCallbackList_Pages, reinterpret_cast<void*>(&TurnGlowCallback), this, nullptr);
        g_game->AddCallback(kCallbackList_Draw, reinterpret_cast<void*>(&DrawFieldsCallback), this, nullptr);
    }
    const HudObjectDef::WheelDef& info = def->wheel;

    // Each page turns towards its target angle.
    const float turn = dt * g_hudWheelSlotRotSpeed.Get();
    for (int i = 0; i < info.pageCount; ++i) {
        PageAngle& a = angles[i];
        if (std::fabs(a.current - a.target) < 1e-5f)
            continue;
        if (a.target < a.current)
            a.current = std::max(a.current - turn, a.target);
        else if (a.current < a.target)
            a.current = std::min(a.current + turn, a.target);
    }

    if (info.fieldCount) {
        ape::world::ApeObj* geometry =
            (object && object->GetType() == ape::world::kObjType_GeomInstance) ? object : nullptr;
        // The pulse swings between -amplitude and +amplitude (a triangle
        // phase through a cosine).
        const float t = pulseTimer / info.pulsePeriod;
        pulseTimer += dt;
        const int whole = int(t);
        const float phase = (whole & 1) ? 1.0f - (t - float(whole)) : t - float(whole);
        const float amplitude = info.pulseAmplitude;
        pulse = (std::cos(phase * 3.1415927f) - 1.0f) * -0.5f * (amplitude + amplitude) - amplitude;
        for (uint32_t i = 0; i < info.fieldCount; ++i) {
            ape::ApeGuiObject* label = info.fields[i].label;
            if (!label)
                continue;
            const HudObjectDef::WheelFieldDef& field = info.fields[i];
            ape::xml::Color8 color = label->GetColor();
            if ((field.flags & 0x80000000u) && PlayerHudColorAvailable()) {
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

    auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
    auto finished = [this, script]() {
        if ((wheelFlags & kW_AnimOk) && !ClipDone(animController, false))
            return false;
        return !(wheelFlags & kW_ScriptOk) || (script && script->IsFinished(false));
    };
    switch (animState >> 1) {
    case 4:   // outro
        if (finished()) {
            HudObject* next = wheelLinked;
            Refresh();
            if (next)
                BeginShow();
        }
        break;
    case 3: { // idle: left / right pick the next page that can be chosen
        if (ClipDone(animController, true)) {
            previousPage = page;
            const uint32_t pressed = PressedButtons();
            bool moved = false;
            int target = page;
            if (ItemAvailable(info.pages[page].item)) {
                if (pressed & kInput_Left) {
                    moved = true;
                    target = FindAvailablePage(page, -1);
                } else if (pressed & kInput_Right) {
                    moved = true;
                    target = FindAvailablePage(page, +1);
                }
            }
            if (target == page) {
                if (moved && script)
                    script->SetStateAndUpdate(kScript_Blocked);
            } else if (!ItemAvailable(info.pages[target].item)) {
                if (moved) {
                    if (script)
                        script->SetStateAndUpdate(kScript_Blocked);
                    const uint32_t item = info.pages[target].item;
                    if (info.kind == kKind_Costume && game::GetJobManager()->IsJobValueFlagged(item))
                        SetFieldText(kField_Warning, "$STR_COSTUME_SPOKE_WARNING");
                }
            } else {
                page = target;
                pointerTarget = info.pages[target].pointerName;
                angles[previousPage].target = 0.0f;
                angles[target].target = kPageOn;
                SetFieldText(kField_Info, info.pages[target].text);
                if (target < 5 && script)
                    script->SetStateAndUpdate(kPageStates[target]);
            }
        }
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
        if (wheelHoldTime > 0.0f) {
            if (offsetTween.IsFinished())
                wheelHoldTime -= dt;
            if (wheelHoldTime <= 0.0f)
                Hide();
        }
        break;
    }
    case 2:
    case 1:
        if (finished()) {
            wheelHoldTime = 0.0f;
            animState = (animState & 1u) | (3u << 1);
            PlayWheelState(3, true);
        }
        break;
    case 0:
        if (wheelLinked && (!wheelLinked->object || !wheelLinked->object->IsVisible()))
            BeginShow();
        break;
    default:
        break;
    }

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

namespace {

bool SimpleItemAvailable(uint32_t kind, uint32_t item)
{
    if (kind == HudCostumeWheel::kKind_Costume)
        return game::GetJobManager()->IsJobValueFlagged(item);
    return item == 0 || UpgradeUnlocked(kind, item);
}

void SetWorld(ape::world::ApeObj* object, const ape::Matrix34& world)
{
    object->m_world = world;
    object->m_flags &= ~ape::world::kObjFlag_Dirty;
}

} // namespace

// @func 0x00375C60 HudCostumeWheel::UpdatePointer
void HudCostumeWheel::UpdatePointer()
{
    constexpr float kDegToRad = 0.017453292f;
    const HudObjectDef::WheelDef& info = def->wheel;
    if (pointer) {
        if (!ItemAvailable(info.pages[page].item)) {
            pointer->SetVisible(false, true);
        } else {
            // Page `page` sits pageAngle * (page - (count - 1) / 2) round the axis.
            const int slot = page + (1 - info.pageCount) / 2;
            ape::Matrix34 turn;
            ape::MatrixAxisAngle(-(float(slot) * info.pageAngle) * kDegToRad, turn, info.axis);
            ape::Matrix34 world = pointer->GetLocal();
            if (const ape::world::ApeObj* follow = FindInHierarchy(object, pointerTarget))
                world = follow->GetWorld();
            if (pointerTarget == info.pages[page].pointerName)
                ape::Matrix34PreMultiply(world, turn);
            pointer->SetLocal({{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f},
                               {0.0f, 0.0f, 0.0f, 0.0f}});
            pointer->MarkDirty();
            SetWorld(pointer, world);
            pointer->SetVisible(true, true);
            pointer->MarkDirty();
        }
    }
    if (glow) {
        glow->SetVisible(false, true);
        glow->SetVisible(true, false);
        for (int i = 0; i < info.pageCount; ++i) {
            const HudObjectDef::WheelPageDef& pageDef = info.pages[i];
            if (ape::world::ApeObj* spoke = FindInHierarchy(object, pageDef.spokeName))
                spoke->SetVisible(true, false);
            const bool available = SimpleItemAvailable(info.kind, pageDef.item);
            if (ape::world::ApeObj* shown = FindInHierarchy(object, pageDef.availableName))
                shown->SetVisible(available, true);
            if (ape::world::ApeObj* locked = FindInHierarchy(object, pageDef.lockedName))
                locked->SetVisible(!available, false);
        }
    }
    // Spokes of pages that can be chosen turn by their page angle about Y;
    // the page's available object follows.
    const ape::Vec3 yAxis{0.0f, 1.0f, 0.0f, 0.0f};
    for (int i = 0; i < info.pageCount; ++i) {
        const HudObjectDef::WheelPageDef& pageDef = info.pages[i];
        if (!SimpleItemAvailable(info.kind, pageDef.item))
            continue;
        ape::world::ApeObj* spoke = FindInHierarchy(object, pageDef.spokeName);
        if (!spoke)
            continue;
        ape::Matrix34 turn;
        ape::MatrixAxisAngle(angles[i].current * kDegToRad, turn, yAxis);
        ape::Matrix34 world = spoke->GetWorld();
        ape::Matrix34PreMultiply(world, turn);
        SetWorld(spoke, world);
        if (ape::world::ApeObj* shown = FindInHierarchy(object, pageDef.availableName))
            SetWorld(shown, world);
    }
}

// @func 0x00376C4C HudCostumeWheel::UpdatePointerCallback
void HudCostumeWheel::UpdatePointerCallback(void* unused, void* context)
{
    (void)unused;
    static_cast<HudCostumeWheel*>(context)->UpdatePointer();
}

// @func 0x00376598 HudCostumeWheel::TurnGlow
void HudCostumeWheel::TurnGlow()
{
    ape::world::ApeObj* target = glowObject ? glowObject : object;
    if (!target || !object)
        return;
    ape::Matrix34 turn;
    ape::Matrix34RotationY(pulse * 0.017453292f, turn);
    SetWorld(target, turn);
    target->SetLocal(turn);
    target->MarkDirty();
    object->UpdateWorld(false, false);
}

// @func 0x00376C70 HudCostumeWheel::TurnGlowCallback
void HudCostumeWheel::TurnGlowCallback(void* unused, void* context)
{
    (void)unused;
    static_cast<HudCostumeWheel*>(context)->TurnGlow();
}

// @func 0x00376998 HudCostumeWheel::CurrentItem
uint32_t HudCostumeWheel::CurrentItem() const
{
    return def->wheel.pages[page].item;
}

// @func 0x003769C4 HudCostumeWheel::PageItem
uint32_t HudCostumeWheel::PageItem(int index) const
{
    if (index < 0 || index >= def->wheel.pageCount)
        return 0;
    return def->wheel.pages[index].item;
}

// @func 0x00376A08 HudCostumeWheel::PageCount
int HudCostumeWheel::PageCount() const
{
    return def->wheel.pageCount;
}

// @func 0x00376A20 HudCostumeWheel::DrawFields
void HudCostumeWheel::DrawFields()
{
    const HudObjectDef::WheelDef& info = def->wheel;
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

// @func 0x00376C20 HudCostumeWheel::DrawFieldsCallback
void HudCostumeWheel::DrawFieldsCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudCostumeWheel*>(context)->DrawFields();
}

} // namespace hud
} // namespace sly
