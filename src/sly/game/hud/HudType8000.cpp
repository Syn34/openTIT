// openTIT - HUD element of type 0x8000, see HudType8000.h.
#include "sly/game/hud/HudType8000.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/world/ApeObj.h"

namespace sly {
namespace hud {

namespace {
uint32_t AnimStateOf(uint32_t animState) { return animState >> 1; }
uint32_t WithAnimState(uint32_t animState, uint32_t state) { return (animState & 1u) | (state << 1); }
uint32_t SetBit(uint32_t word, uint32_t bit, bool on) { return on ? (word | bit) : (word & ~bit); }
} // namespace

// @func 0x003686D0 HudType8000::PlayState8000
void HudType8000::PlayState8000(int state, bool loop)
{
    stateWord = SetBit(stateWord, kF_AnimOk, PlayHudClip(animController, clips[state], 1.0f, loop));
    if (animScript) {
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
        stateWord = SetBit(stateWord, kF_ScriptOk, script->SetStateAndUpdate(kHudStateNames[state]));
    }
}

// @func 0x00368770 HudType8000::BeginShow8000
void HudType8000::BeginShow8000()
{
    stateWord |= kF_Shown;
    animState = WithAnimState(animState, 1);
    if (object)
        object->SetVisible(true, true);
    PlayState8000(int(AnimStateOf(animState)), false);
}

// @func 0x00368870 HudType8000::ShowTimed8000
void HudType8000::ShowTimed8000(float fade, bool flag, HudObject* linkedTo)
{
    if (Blocked() || !(stateWord & kF_Shown))
        return;
    if (flag)
        stateWord |= kF_LeaveA;
    const uint32_t state = AnimStateOf(animState);
    fadeTime8000 = fade;
    if (state == 3 || state == 2) {
        // Already up: restart the update animation and the hold time.
        holdTime8000 = fade < 0.0f ? defaultFade8000 : fade;
        animState = WithAnimState(animState, 2);
        PlayState8000(2, false);
    } else if (state == 0 || state == 4) {
        linked8000 = linkedTo;
        if (!linkedTo || !linkedTo->object || !linkedTo->object->IsVisible())
            BeginShow8000();
    }
}

// @func 0x003689D8 HudType8000::ShowElement
void HudType8000::ShowElement(HudType8000* element)
{
    if (!element || element->Blocked())
        return;
    element->stateWord |= kF_Shown;
    element->ShowTimed8000(-1.0f, false, nullptr);
}

// @func 0x00368AC8 HudType8000::Hide8000
void HudType8000::Hide8000(bool a, bool b)
{
    if (Blocked() || !(stateWord & kF_Shown))
        return;
    const uint32_t state = AnimStateOf(animState);
    if (state == 4 || state == 0) {
        if ((!(stateWord & kF_LeaveA) && !a) || state != 0) {
            if (((stateWord & kF_LeaveB) || b) && state == 0 && !*Hud().hideList.Find(RegisteredName()))
                Hud().hideList.Add(RegisteredName(), this);
        } else {
            Refresh();
        }
    } else {
        stateWord = (stateWord & ~(kF_LeaveB | kF_LeaveA)) | (b ? kF_LeaveB : 0u) | (a ? kF_LeaveA : 0u);
        animState = WithAnimState(animState, 4);
        fadeTime8000 = -1.0f;
        PlayState8000(4, false);
        if (object)
            object->SetVisible(true, true);
    }
    linked8000 = nullptr;
}

// @func 0x00368D78 HudType8000::HideElement
void HudType8000::HideElement(HudType8000* element)
{
    if (element)
        element->Hide8000(false, false);
}

// @func 0x003846E8 HudType8000::SetEnabled
void HudType8000::SetEnabled(bool enabled, bool flag)
{
    if (!enabled)
        Hide8000(flag, false);
    else if (!(stateWord & kF_Shown))
        ShowTimed8000(-1.0f, false, nullptr);
}

// @func 0x00384088 HudType8000::UpdateHold
void HudType8000::UpdateHold(float dt)
{
    if (!offsetTween.IsFinished()) {
        offsetTween.Update(dt);
        if (object) {
            ape::Matrix34 local = object->GetLocal();
            local.translation = {offset.x, offset.y, offset.z, local.translation.pad};
            object->SetLocal(local);
            object->MarkDirty();
        }
    }
    if (holdTime8000 > 0.0f) {
        // The hold time only runs once the element has stopped sliding.
        if (offsetTween.IsFinished())
            holdTime8000 -= dt;
        if (holdTime8000 <= 0.0f)
            Hide8000((stateWord & kF_OutroA) != 0, (stateWord & kF_OutroB) != 0);
    }
}

// @func 0x003841B0 HudType8000::FinishIntro
void HudType8000::FinishIntro()
{
    if ((stateWord & kF_AnimOk) && !HudClipDone(animController))
        return;
    auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript);
    if ((stateWord & kF_ScriptOk) && !(script && script->IsFinished(false)))
        return;
    animState = WithAnimState(animState, 3);
    holdTime8000 = fadeTime8000 < 0.0f ? defaultFade8000 : fadeTime8000;
    PlayState8000(3, true);
}

void HudType8000::HideParts()
{
    auto hide = [](ape::world::ApeObj* part) {
        if (part)
            part->SetVisible(false, true);
    };
    for (int i = 0; i < 12; ++i) {
        hide(rowA[i]);
        hide(rowB[i]);
        hide(rowC[i]);
    }
    hide(part188);
    hide(part18C);
    for (ape::world::ApeObj* part : markers)
        hide(part);
    for (ape::world::ApeObj* part : slots)
        hide(part);
    hide(part210);
}

// @func 0x003817E0 HudType8000::SetActive
void HudType8000::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    if (!active)
        HideParts();
}

// @func 0x003842D0 HudType8000::Refresh
void HudType8000::Refresh()
{
    SetActive(false);
    const float origin[3] = {0.0f, 0.0f, 0.0f};
    SetStackOffset(origin, true);
    linked8000 = nullptr;
    holdTime8000 = 0.0f;
    animState &= 1u;
    fadeTime8000 = -1.0f;
    // A persistent element stays shown unless it was asked to leave.
    const bool keepShown = !(stateWord & kF_LeaveA) && (stateWord & kF_Persistent);
    stateWord &= ~(kF_Shown | kF_LeaveA | kF_Bit27 | kF_Bit26 | kF_AnimOk | kF_ScriptOk);
    if (keepShown)
        stateWord |= kF_Shown;
    if ((stateWord & kF_LeaveB) && !*Hud().hideList.Find(RegisteredName()))
        Hud().hideList.Add(RegisteredName(), this);
    HideParts();
}

} // namespace hud
} // namespace sly
