// openTIT - HUD object registry.
#include "sly/game/hud/HudManager.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"
#include "sly/anim/AnimController.h"
#include "sly/game/hud/HudHooks.h"

#include <cmath>
#include <cstdio>

namespace sly {
namespace hud {

namespace {

HudObject* FindTyped(uint32_t name, uint32_t type)
{
    if (name == 0)
        return nullptr;
    HudObject** slot = Hud().objects.Find(name);
    HudObject* object = slot ? *slot : nullptr;
    if (!object || !(object->typeFlags & type))
        return nullptr;
    return object;
}

} // namespace

uint32_t g_bottleCounterName = 0;

HudManagerState& Hud()
{
    static HudManagerState state;
    return state;
}

bool ObjectVisible(const ape::world::ApeObj* object)
{
    return object->IsVisible();
}

// @func 0x0036A2FC HudObject::Update
void HudObject::Update(float dt)
{
    if (animController)
        animController->UpdateLayers(dt);
    if (!object || !object->IsVisible())
        return;
    const ape::Vec3& zero = ape::world::kWorldZero;
    const bool atOrigin = baseOffset.x == zero.x && baseOffset.y == zero.y && baseOffset.z == zero.z;
    if (atOrigin || !HudPlacementEnabled())
        return;
    ape::Matrix34 local = object->GetLocal();
    local.translation.x = zero.x + baseOffset.x + offset.x;
    local.translation.y = zero.y + baseOffset.y + offset.y;
    local.translation.z = zero.z + baseOffset.z + offset.z;
    object->SetLocal(local);
    object->MarkDirty();
}

// @func 0x0036A410 HudObject::Refresh
void HudObject::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
}

// @func 0x0036A470 HudObject::SetActive
void HudObject::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
}

// @func 0x003632C8 HudObject::SetFieldText
void HudObject::SetFieldText(const char* text, uint32_t index)
{
    if (!text || index >= def->textFieldCount)
        return;
    if (ape::ApeGuiObject* label = textFields[index].label) {
        label->SetText(text, false);
        label->Invalidate();
    }
}

// @func 0x0036336C HudObject::FieldScale
void HudObject::FieldScale(float out[3], uint32_t index) const
{
    const ape::Vec3& unit = ape::world::kWorldUnitScale;
    out[0] = unit.x;
    out[1] = unit.y;
    out[2] = unit.z;
    if (index >= def->textFieldCount)
        return;
    if (const ape::ApeGuiObject* label = textFields[index].label) {
        const ape::Vec3& scale = label->GetScale();
        out[0] = scale.x;
        out[1] = scale.y;
        out[2] = scale.z;
    }
}

// @func 0x00367530 hud::SetFieldText
void SetFieldText(HudObject* object, const char* text, uint32_t index)
{
    if (object)
        object->SetFieldText(text, index);
}

// @func 0x003675D8 hud::SetFieldTextByName
void SetFieldTextByName(uint32_t name, const char* text, uint32_t index)
{
    if (name == 0)
        return;
    if (HudObject* object = Find(name))
        object->SetFieldText(text, index);
}

namespace {
ape::ApeGuiLabel* FirstLabel(HudObject* object)
{
    if (!object || object->def->textFieldCount == 0)
        return nullptr;
    return static_cast<ape::ApeGuiLabel*>(object->textFields[0].label);
}
} // namespace

// @func 0x003676DC hud::SetFieldAnimMode
void SetFieldAnimMode(HudObject* object, uint32_t modeName)
{
    if (ape::ApeGuiLabel* label = FirstLabel(object))
        label->animMode = uint8_t(ape::ParseGuiAnimPlayMode(modeName) & 0xF);
}

// @func 0x00367758 hud::SetFieldAnimPeriod
void SetFieldAnimPeriod(HudObject* object, float seconds)
{
    ape::ApeGuiLabel* label = FirstLabel(object);
    if (!label)
        return;
    if (seconds < 0.0f)
        seconds = 3.0f;
    // Stored as a rate.
    label->animPeriod = seconds >= 1e-5f ? 1.0f / seconds : 3.4028235e38f;
}

// @func 0x00362BEC HudObject::HideNow
void HudObject::HideNow()
{
    auto& list = Hud().hideList;
    if (*list.Find(RegisteredName()))
        return;
    list.Add(RegisteredName(), this);
}

// @func 0x00365A54 hud::SetAllHidden
void SetAllHidden(bool hidden, bool flag)
{
    HudManagerState& state = Hud();
    state.allHidden = hidden;
    for (uint32_t i = 0; i < state.objects.Count(); ++i)
        state.objects.Data()[i].value->SetHidden(hidden, flag);
    if (!state.allHidden)
        RefreshAnchorStacks();
}

// TODO(0x00363A3C HudObject::ShowText)
void HudObject::ShowText(float fade, const char* text, bool flag)
{
    (void)fade;
    (void)text;
    (void)flag;
}

// TODO(0x00362F1C HudObject::PlayIntro)
void HudObject::PlayIntro(bool a, bool b)
{
    (void)a;
    (void)b;
}

// @func 0x00362B40 HudObject::PlayOutro
void HudObject::PlayOutro(bool a, bool b)
{
    if (Suppressed() || !(stateFlags & kState_Shown) || (stateFlags & kState_Leaving))
        return;
    const uint32_t state = animState >> 1;
    if (state == 0 || state == 4) {
        // Idle (or looping): only an idle element with a reason to leave goes.
        const bool reason = (stateFlags & kState_OutroA) || a || (stateFlags & kState_OutroB) || b;
        if (reason && state == 0) {
            field_A4 = false;
            stateFlags = (stateFlags & ~kState_Bit27) | kState_Leaving;
        }
    } else {
        // Busy: remember the request and leave when the animation allows.
        field_A4 = false;
        stateFlags &= ~(kState_Bit27 | kState_OutroB | kState_OutroA);
        if (b)
            stateFlags |= kState_OutroB;
        if (a)
            stateFlags |= kState_OutroA;
        stateFlags |= kState_Leaving;
    }
}

// @func 0x0036325C HudObject::Show
void HudObject::Show(bool a, bool intro, bool b)
{
    if (Suppressed())
        return;
    if (intro)
        PlayIntro(a, b);
    else
        PlayOutro(a, b);
}

// @func 0x003626E8 HudObject::SetStackOffset
void HudObject::SetStackOffset(const float target[3], bool immediate)
{
    if (!immediate) {
        // Slide there over half a second.
        offsetTween.target = &offset;
        offsetTween.elapsed = 0.0f;
        offsetTween.SetDuration(0.5f);
        offsetTween.from = offset;
        offsetTween.to = {target[0], target[1], target[2], offsetTween.to.pad};
        return;
    }
    const bool moved = std::fabs(offset.x - target[0]) >= 1e-5f || std::fabs(offset.y - target[1]) >= 1e-5f ||
                       std::fabs(offset.z - target[2]) >= 1e-5f;
    if (moved) {
        ape::Matrix34 local = object->GetLocal();
        local.translation.x = target[0];
        local.translation.y = target[1];
        local.translation.z = target[2];
        object->SetLocal(local);
        object->MarkDirty();
    }
    offset.x = target[0];
    offset.y = target[1];
    offset.z = target[2];
    offsetTween.Clear();
}

// @func 0x00362830 hud::AnchorMirror
void AnchorMirror(float out[3], int32_t anchor, const float in[3])
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    switch (anchor) {
    case kAnchor_TopLeft:
        out[1] = -out[1];
        break;
    case kAnchor_TopRight:
        out[0] = -out[0];
        out[1] = -out[1];
        break;
    case kAnchor_BottomLeft:
        out[0] = -out[0];
        break;
    default:
        break;
    }
}

// @func 0x003628B4 HudObject::StackOffset
void HudObject::StackOffset(float out[3]) const
{
    float sum[3] = {0.0f, 0.0f, 0.0f};
    int32_t anchor = kAnchor_None;
    if (def && def->anchor != kAnchor_None) {
        anchor = def->anchor;
        for (HudObject* other : Hud().stacks[anchor]) {
            if (other == this)
                break;
            if (other->def) {
                sum[0] += other->def->offset[0];
                sum[1] += other->def->offset[1];
                sum[2] += other->def->offset[2];
            }
        }
    }
    AnchorMirror(out, anchor, sum);
}

// @func 0x003629D8 HudObject::LeaveStack
void HudObject::LeaveStack()
{
    if (!def || def->anchor == kAnchor_None)
        return;
    ape::ApeTArray<HudObject*>& stack = Hud().stacks[def->anchor];
    for (uint32_t i = 0; i < stack.Count(); ++i) {
        if (stack[i] != this)
            continue;
        // Keep the order: shift the rest down, then move each of them up.
        for (uint32_t j = i; j + 1 < stack.Count(); ++j)
            stack[j] = stack[j + 1];
        stack.PopBack();
        for (uint32_t j = i; j < stack.Count(); ++j) {
            float offset[3];
            stack[j]->StackOffset(offset);
            stack[j]->SetStackOffset(offset, false);
        }
        return;
    }
}

// @func 0x003568E0 hud::Find
HudObject* Find(uint32_t name) { return FindTyped(name, kHud_Object); }
// @func 0x00365E30 hud::FindType2
HudObject* FindType2(uint32_t name) { return FindTyped(name, kHud_Type2); }
// @func 0x00368FA0 hud::FindType4
HudObject* FindType4(uint32_t name) { return FindTyped(name, kHud_Type4); }
// @func 0x00365EAC hud::FindType40
HudObject* FindType40(uint32_t name) { return FindTyped(name, kHud_Type40); }
// @func 0x00365BA4 hud::FindType400
HudObject* FindType400(uint32_t name) { return FindTyped(name, kHud_Type400); }
// @func 0x00368E2C hud::FindType800
HudObject* FindType800(uint32_t name) { return FindTyped(name, kHud_Type800); }
// @func 0x00368EA8 hud::FindType1000
HudObject* FindType1000(uint32_t name) { return FindTyped(name, kHud_Type1000); }
// @func 0x00368F24 hud::FindType2000
HudObject* FindType2000(uint32_t name) { return FindTyped(name, kHud_Type2000); }
// @func 0x00368DB0 hud::FindType4000
HudObject* FindType4000(uint32_t name) { return FindTyped(name, kHud_Type4000); }
// @func 0x00368A48 hud::FindType8000
HudObject* FindType8000(uint32_t name) { return FindTyped(name, kHud_Type8000); }
// @func 0x0035B908 hud::FindType10000
HudObject* FindType10000(uint32_t name) { return FindTyped(name, kHud_Type10000); }
// @func 0x00365B28 hud::FindType20000
HudObject* FindType20000(uint32_t name) { return FindTyped(name, kHud_Type20000); }

// @func 0x00365C24 hud::FindActiveType200
HudObject* FindActiveType200()
{
    HudManagerState& state = Hud();
    if (state.activeType200Name != 0)
        if (HudObject* object = FindTyped(state.activeType200Name, kHud_Type200))
            return object;
    return FindTyped(state.fallbackType200Name, kHud_Type200);
}

// @func 0x00367518 hud::SetSuppressed
void SetSuppressed(HudObject* object, bool suppressed)
{
    if (!object)
        return;
    object->stateFlags = suppressed ? (object->stateFlags | HudObject::kState_Suppressed)
                                    : (object->stateFlags & ~HudObject::kState_Suppressed);
}

// @func 0x0036735C hud::ShowTextByName
void ShowTextByName(float fade, uint32_t name, const char* text, bool flag)
{
    if (HudObject* object = Find(name))
        object->ShowText(fade, text, flag);
}

namespace {
void ShowTyped(uint32_t name, uint32_t type, bool a, bool intro, bool b)
{
    if (HudObject* object = FindTyped(name, type))
        object->Show(a, intro, b);
}
} // namespace

// @func 0x0036742C hud::ShowByName
void ShowByName(uint32_t name, bool a, bool intro, bool b)
{
    ShowTyped(name, kHud_Object, a, intro, b);
}

// @func 0x00367B28 hud::ShowType4000ByName
void ShowType4000ByName(uint32_t name, bool a, bool intro, bool b)
{
    ShowTyped(name, kHud_Type4000, a, intro, b);
}

// @func 0x00367134 hud::CountElementsAtAnchor
int CountElementsAtAnchor(int32_t anchor)
{
    int count = 0;
    for (auto& stack : Hud().stacks)
        for (HudObject* object : stack)
            if (object->def->anchor == anchor)
                ++count;
    return count;
}

// @func 0x00369D00 hud::SetContext
void SetContext(uint32_t value, uint32_t name)
{
    Hud().contextName = name;
    Hud().contextValue = value;
}

// @func 0x00369D14 hud::SetContextByString
void SetContextByString(uint32_t value, const char* name)
{
    Hud().contextName = ape::HashNameNoCase(name);
    Hud().contextValue = value;
}

// @func 0x0036960C hud::RequestScreen
void RequestScreen(bool skip, const char* name)
{
    if (skip)
        return;
    Hud().pendingScreenName = ape::HashNameNoCase(name);
    Hud().pendingScreen = true;
}

// @func 0x00369CAC hud::SetPowMeterPrefix
void SetPowMeterPrefix(const char* prefix)
{
    char name[0x60];
    std::snprintf(name, sizeof(name), "%sPOW", prefix);
    Hud().powMeterName = ape::HashNameNoCase(name);
}

// @func 0x00369D58 hud::SnapMeterA
void SnapMeterA()
{
    if (HudObject* meter = Hud().meterA)
        meter->shownValue = meter->targetValue;
}

// @func 0x00369658 hud::SnapMeterB
void SnapMeterB()
{
    if (HudObject* meter = Hud().meterB)
        meter->shownValue = meter->targetValue;
}

// @func 0x00369500 hud::ResetUpdateFlags
void ResetUpdateFlags()
{
    auto& list = Hud().updateList;
    for (uint32_t i = 0; i < list.Count(); ++i) {
        HudObject* object = list.Data()[i].value;
        object->updateFlags &= 0x7FFFFFFFu;
        object->Refresh();
    }
}

// @func 0x003695A0 hud::AnyUpdatingElementVisible
bool AnyUpdatingElementVisible()
{
    auto& list = Hud().updateList;
    for (uint32_t i = 0; i < list.Count(); ++i) {
        HudObject* object = list.Data()[i].value;
        if (object->object && ObjectVisible(object->object))
            return true;
    }
    return false;
}

} // namespace hud
} // namespace sly
