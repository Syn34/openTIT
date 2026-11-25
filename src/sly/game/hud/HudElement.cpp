// openTIT - The common HUD element (vtable 0x00B1F6B0).
#include "sly/game/hud/HudElement.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/level/ApeGuiScript.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"

#include <cstdlib>

namespace sly {
namespace hud {

namespace {

constexpr uint32_t kScript_VisibleOn = 0x152FB9B1u;    // "visibleON"
constexpr uint32_t kScript_VisibleOff = 0x90C77BBDu;   // "visibleOFF"

void SetScriptState(HudObject* element, uint32_t state)
{
    if (element->animScript)
        static_cast<ape::ApeAnimScriptInstance*>(element->animScript)->SetStateAndUpdate(state);
}

} // namespace

// @func 0x0036ABF8 HudElement::~HudElement
// @func 0x0036AF54 HudElement::~HudElement (deleting)
HudElement::~HudElement()
{
    GameWorldRemoveHudCallback(this);
    if (object)
        object->SetVisible(false, true);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* fieldObject = textFields[i].object)
            fieldObject->SetVisible(false);
    }
    SetScriptState(this, kScript_VisibleOff);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (textFields[i].object)
            ape::StopGuiScripts("", textFields[i].object);
    }
    // Field objects the element made itself (definition flag bit 31) go with it.
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if ((def->textFieldDefs[i].field_1C & 0x80000000u) && textFields[i].object)
            delete textFields[i].object;
    }
    std::free(textFields);
    textFields = nullptr;
}

// @func 0x0036AACC HudElement::SetActive
void HudElement::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* fieldObject = textFields[i].object)
            fieldObject->SetVisible(active);
    }
    SetScriptState(this, active ? kScript_VisibleOn : kScript_VisibleOff);
}

// @func 0x0036BCAC HudElement::Refresh
void HudElement::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    linked = nullptr;
    holdTime = 0.0f;
    animState &= 1u;
    fadeTime = -1.0f;
    text[0] = '\0';
    altIdleAnim = 0;
    // Keep the suppression, reasons and the persistent bit; a persistent
    // element stays shown unless it was told to leave.
    const uint32_t old = stateFlags;
    uint32_t flags = old & 0x927FFFFFu;
    if (!(old & kState_OutroA))
        flags = (old & 0xB27FFFFFu) | ((old & kState_Persistent) ? kState_Shown : 0u);
    stateFlags = flags;
    if ((stateFlags & kState_OutroB) && !*Hud().hideList.Find(RegisteredName()))
        Hud().hideList.Add(RegisteredName(), this);
    ape::ApeTArray<HudObject*>& list = Hud().field_C2C0;
    const int at = list.FindLast(this);
    if (at >= 0)
        list.RemoveAt(size_t(at));
}

// @func 0x0036BF38 HudElement::SetEnabled
void HudElement::SetEnabled(bool enabled, bool flag)
{
    if (!enabled) {
        PlayIntro(flag, false);
    } else if (!(stateFlags & kState_Shown)) {
        static const uint8_t kNoTint[4] = {0, 0, 0, 0};
        ShowTimed(-1.0f, nullptr, false, nullptr, kNoTint);
    }
}

// @func 0x0036BAA0 HudElement::TickStack
void HudElement::TickStack(float dt)
{
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
    if (holdTime > 0.0f) {
        // The hold time only runs while the element stands still.
        if (offsetTween.IsFinished())
            holdTime -= dt;
        if (holdTime <= 0.0f)
            PlayIntro((stateFlags & kState_Bit22) != 0, (stateFlags & kState_Bit21) != 0);
    }
}

// @func 0x0036BBC0 HudElement::OnIntroDone
void HudElement::OnIntroDone()
{
    if (stateFlags & kState_AnimOk) {
        anim::AnimLayer* layer = animController ? animController->FindLayer(0) : nullptr;
        if (layer && !layer->IsDone(false))
            return;
    }
    if (stateFlags & kState_ScriptOk) {
        if (!animScript || !static_cast<ape::ApeAnimScriptInstance*>(animScript)->IsFinished(false))
            return;
    }
    animState = (animState & 1u) | (3u << 1);
    holdTime = fadeTime < 0.0f ? defaultFade : fadeTime;
    const bool loop = altIdleAnim ? (stateFlags & kState_Bit19) != 0 : true;
    PlayState(3, loop);
}

} // namespace hud
} // namespace sly
