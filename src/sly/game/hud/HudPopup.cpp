// openTIT - Popup HUD element (vtable 0x00B1F7C8).
#include "sly/game/hud/HudPopup.h"

#include "sly/anim/AnimController.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"

#include <cstdio>

namespace sly {
namespace hud {

namespace {

void SetScriptState(HudObject* element, uint32_t state)
{
    if (element->animScript)
        static_cast<ape::ApeAnimScriptInstance*>(element->animScript)->SetStateAndUpdate(state);
}

uint32_t ClipName(const char* prefix, const char* suffix)
{
    char name[0x60];
    std::snprintf(name, sizeof name, "%s_%s", prefix, suffix);
    return ape::HashNameNoCase(name);
}

} // namespace

// @func 0x00359CDC HudPopup::HudPopup
HudPopup::HudPopup(HudObjectDef* def_) : HudObject(0, def_)
{
    active = def->startShown != 0;
    typeFlags |= kHud_Type20;
    const char* prefix = def->animPrefix[0] ? def->animPrefix : def->defName;
    if (prefix[0]) {
        introAnim = ClipName(prefix, "Intro");
        idleAnim = ClipName(prefix, "Idle");
        outroAnim = ClipName(prefix, "Outro");
    }
}

// @func 0x0036EB74 HudPopup::~HudPopup
// @func 0x0036ECDC HudPopup::~HudPopup (deleting)
HudPopup::~HudPopup()
{
    if (object)
        object->SetVisible(false, true);
}

// @func 0x0036ED28 HudPopup::Update
void HudPopup::Update(float dt)
{
    if (!active)
        return;
    const uint32_t state = animState >> 1;
    if (state == 0)
        return;
    if (state == 1) {
        if (HudClipDone(animController)) {
            animState = (animState & 1u) | (2u << 1);
            PlayHudClip(animController, idleAnim, 1.0f, true);
            SetScriptState(this, kHudStateNames[3]);
        }
    } else if (state == 3) {
        if (HudClipDone(animController))
            Refresh();
    }
    if (animController)
        animController->UpdateLayers(dt);
    // Placement as in HudObject::Update.
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

// @func 0x0036EF3C HudPopup::Refresh
void HudPopup::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    animState &= 1u;
    active = def->startShown != 0;
}

// @func 0x0036EAD4 HudPopup::SetActive
void HudPopup::SetActive(bool on)
{
    if (object)
        object->SetVisible(on, true);
    if (on)
        SetScriptState(this, kStateBase + variant);
}

// @func 0x0036F1C8 HudPopup::SetEnabled
void HudPopup::SetEnabled(bool enabled, bool)
{
    if (enabled)
        Show();
    else
        Hide();
}

// @func 0x0036F0D0 HudPopup::Show
void HudPopup::Show()
{
    if (!active || (animState >> 1) != 0)
        return;
    if (object)
        object->SetVisible(true, true);
    variant = 0;
    SetScriptState(this, kStateBase);
    animState = (animState & 1u) | (1u << 1);
    PlayHudClip(animController, introAnim, 1.0f, false);
    SetScriptState(this, kHudStateNames[1]);
}

// @func 0x0036EFC8 HudPopup::Hide
void HudPopup::Hide()
{
    const uint32_t state = animState >> 1;
    if (!active || state == 3 || state == 0)
        return;
    if (object)
        object->SetVisible(true, true);
    SetScriptState(this, kStateBase + variant);
    animState = (animState & 1u) | (3u << 1);
    PlayHudClip(animController, outroAnim, 1.0f, false);
    SetScriptState(this, kHudStateNames[4]);
}

} // namespace hud
} // namespace sly
