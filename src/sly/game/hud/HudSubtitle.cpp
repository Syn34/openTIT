// openTIT - Timed text HUD element (vtable 0x00B1F988).
#include "sly/game/hud/HudSubtitle.h"

#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeObj.h"

namespace sly {
namespace hud {

HudSubtitle::HudSubtitle(HudObjectDef* def_, const HudTimedLines* lines) : HudElement(def_), timed(lines)
{
    typeFlags |= kHud_Type4000;
}

// @func 0x00381320 HudSubtitle::~HudSubtitle
// @func 0x00381460 HudSubtitle::~HudSubtitle (deleting)
HudSubtitle::~HudSubtitle()
{
    GameWorldRemoveHudCallback(this);
    if (object)
        object->SetVisible(false, true);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* fieldObject = textFields[i].object)
            fieldObject->SetVisible(false);
    }
    if (animScript)
        static_cast<ape::ApeAnimScriptInstance*>(animScript)->SetStateAndUpdate(0x90C77BBDu);   // "visibleOFF"
}

// @func 0x003814E0 HudSubtitle::Update
void HudSubtitle::Update(float dt)
{
    const bool intro = (animState >> 1) == 1;
    if (intro)
        delayTimer += dt;
    if (timed->delay < delayTimer) {
        SetActive(true);
        HudElement::Update(dt);
    } else if (intro) {
        SetActive(false);
    }
    // Write the lines whose time has come, one per update.
    if ((animState >> 1) == 1 && timed->delay < delayTimer) {
        const float before = lineTime;
        lineTime = before + dt;
        const int32_t next = lineIndex + 1;
        if (next < timed->count && timed->lines[next].time < before + dt) {
            lineIndex = next;
            const HudTimedLine& line = timed->lines[next];
            if (line.field < def->textFieldCount)
                SetFieldText(line.text, line.field);
        }
    }
    if ((animState >> 1) == 1)
        SetMinigameLinePending(lineIndex + 1 < timed->count);
    if ((animState >> 1) == 0)
        SetActive(false);
}

// @func 0x00381758 HudSubtitle::Refresh
void HudSubtitle::Refresh()
{
    PlayIntro(false, false);
    HudElement::Refresh();
    lineIndex = -1;
    animState &= 1u;
    lineTime = 0.0f;
    delayTimer = 0.0f;
    holdTime = 0.0f;
    SetActive(false);
}

} // namespace hud
} // namespace sly
