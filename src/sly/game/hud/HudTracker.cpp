// openTIT - Tracker list HUD element (vtable 0x00B1F838).
#include "sly/game/hud/HudTracker.h"

#include "sly/anim/AnimController.h"
#include "sly/entity/Entity.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"

#include <cstdio>
#include <cstdlib>

namespace sly {
namespace hud {

namespace {

uint32_t ClipName(const char* prefix, const char* suffix)
{
    char clip[0x60];
    std::snprintf(clip, sizeof clip, "%s_%s", prefix, suffix);
    return ape::HashNameNoCase(clip);
}

ape::world::ApeObj* FindSceneChild(ape::world::ApeObj* root, uint32_t name)
{
    if (!root || name == 0)
        return nullptr;
    if (root->GetNameHash() == name)
        return root;
    return anim::FindDescendant(root, name);
}

void SetScriptState(HudObject* element, uint32_t state)
{
    if (element->animScript)
        static_cast<ape::ApeAnimScriptInstance*>(element->animScript)->SetStateAndUpdate(state);
}

} // namespace

// @func 0x0035A3A0 HudTracker::HudTracker
HudTracker::HudTracker(HudObjectDef* def_) : HudObject(0, def_), entries(6)
{
    typeFlags |= kHud_Type4;
    if (object)
        marker = FindSceneChild(object, def->trackerMarkerName);
    if (def->textFieldCount) {
        fieldObjects = static_cast<ape::world::ApeObj**>(std::calloc(def->textFieldCount, sizeof(ape::world::ApeObj*)));
        for (uint32_t i = 0; i < def->textFieldCount; ++i)
            fieldObjects[i] = FindSceneChild(object, def->trackerFields[i].objectName);
    }
    const char* prefix = def->animPrefix[0] ? def->animPrefix : def->defName;
    if (prefix[0]) {
        introAnim = ClipName(prefix, "Intro");
        idleAnim = ClipName(prefix, "Idle");
        outroAnim = ClipName(prefix, "Outro");
    }
}

// @func 0x00370220 HudTracker::~HudTracker
// @func 0x00370498 HudTracker::~HudTracker (deleting)
HudTracker::~HudTracker()
{
    GameWorldRemoveHudCallback(this);
    if (object)
        object->SetVisible(false, true);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* gui = def->trackerFields[i].object)
            gui->SetVisible(false);
    }
    std::free(fieldObjects);
    fieldObjects = nullptr;
}

// @func 0x003704E8 HudTracker::Update
void HudTracker::Update(float dt)
{
    if (!(animState & 1u) || (animState >> 1) == 0)
        return;
    GameWorldAddHudCallback(this, 7);
    GameWorldAddHudCallback(this, 8);
    // Dead entities leave the list (the last entry takes their place).
    for (uint32_t i = 0; i < entries.Count();) {
        const entity::Entity* tracked = entries[i].entity;
        if (tracked && !tracked->IsAlive())
            entries.RemoveAtSwap(i);
        else
            ++i;
    }
    const uint32_t state = animState >> 1;
    if (state == 1) {
        if (HudClipDone(animController)) {
            animState = (animState & 1u) | (2u << 1);
            PlayHudClip(animController, idleAnim, 1.0f, false);
            SetScriptState(this, kHudStateNames[3]);
        }
    } else if (state == 2) {
        if (HudClipDone(animController)) {
            animState = (animState & 1u) | (3u << 1);
            PlayHudClip(animController, outroAnim, 1.0f, false);
            SetScriptState(this, kHudStateNames[4]);
        }
    } else if (state == 3) {
        if (HudClipDone(animController))
            Refresh();
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

// @func 0x00370860 HudTracker::Refresh
void HudTracker::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    if (sticky) {
        animState = (animState & 1u) | (2u << 1);
        SetActive(true);
    } else {
        entries.Clear();
        animState &= 1u;
    }
    animState |= 1u;
}

// @func 0x00370928 HudTracker::BeginFrame
void HudTracker::BeginFrame()
{
    entries.Clear();
}

// @func 0x00370130 HudTracker::SetActive
void HudTracker::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* gui = def->trackerFields[i].object)
            gui->SetVisible(active);
    }
}

// @func 0x00370934 HudTracker::SetEnabled
void HudTracker::SetEnabled(bool, bool) {}

} // namespace hud
} // namespace sly
