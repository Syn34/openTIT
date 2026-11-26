// openTIT - Screen HUD element (vtable 0x00B1F720).
#include "sly/game/hud/HudScreen.h"

#include "sly/anim/AnimController.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

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

} // namespace

// @func 0x0035A5B0 HudScreen::HudScreen
HudScreen::HudScreen(HudObjectDef* def_) : HudObject(0, def_)
{
    typeFlags |= kHud_Type2;
    const char* prefix = def->animPrefix[0] ? def->animPrefix : def->defName;
    if (prefix[0]) {
        introAnim = ClipName(prefix, "Intro");
        idleAnim = ClipName(prefix, "Idle");
        outroAnim = ClipName(prefix, "Outro");
    }
    if (def->textFieldCount) {
        fields = static_cast<HudTextField*>(std::calloc(def->textFieldCount, sizeof(HudTextField)));
        for (uint32_t i = 0; i < def->textFieldCount; ++i) {
            const HudObjectDef::TextFieldDef& info = def->textFieldDefs[i];
            fields[i].sceneObject = FindSceneChild(object, info.objectName);
            fields[i].object = info.templateObject;
            fields[i].field_08 = info.framePath;
            fields[i].label = info.labelPath;
        }
    }
    for (int i = 0; i < 6; ++i)
        subObjects[i] = FindSceneChild(object, kSubObjects[i]);
}

// @func 0x0036C400 HudScreen::~HudScreen
// @func 0x0036C574 HudScreen::~HudScreen (deleting)
HudScreen::~HudScreen()
{
    std::free(fields);
    fields = nullptr;
    GameWorldRemoveHudCallback(this);
}

// @func 0x0036C5BC HudScreen::Update
void HudScreen::Update(float dt)
{
    const uint32_t state = animState >> 1;
    if (state == 0)
        return;
    GameWorldAddHudCallback(this, 9);
    if (state == 4) {
        if (HudClipDone(animController)) {
            // A locked screen resets anyway once its outro ended.
            const bool wasLocked = locked;
            locked = false;
            Refresh();
            locked = wasLocked;
        }
    } else if (state == 2) {
        animState = (animState & 1u) | (3u << 1);
        if (animScript)
            static_cast<ape::ApeAnimScriptInstance*>(animScript)->SetStateAndUpdate(kHudStateNames[3]);
    } else if (state == 1) {
        if (HudClipDone(animController))
            animState = (animState & 1u) | (2u << 1);
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

// @func 0x0036C818 HudScreen::Refresh
void HudScreen::Refresh()
{
    if (locked)
        return;
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    animState = 1;
}

// @func 0x0036CA0C HudScreen::SetActive
void HudScreen::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    HideSubObjects();
}

// @func 0x0036CA70 HudScreen::SetEnabled
void HudScreen::SetEnabled(bool, bool) {}

// @func 0x0036C898 HudScreen::HideSubObjects
void HudScreen::HideSubObjects()
{
    if (!def->hideSubObjects)
        return;
    for (ape::world::ApeObj* sub : subObjects) {
        if (sub)
            sub->SetVisible(false, true);
    }
}

namespace {
void SetState(HudObject* element, uint32_t state)
{
    if (element->animScript)
        static_cast<ape::ApeAnimScriptInstance*>(element->animScript)->SetStateAndUpdate(state);
}
} // namespace

// @func 0x0036CA78 HudScreen::Open
void HudScreen::Open()
{
    const uint32_t state = animState >> 1;
    if (state == 1 || state == 3)
        return;
    if (object)
        object->SetVisible(true, true);
    animState = (animState & 1u) | (1u << 1);
    PlayHudClip(animController, introAnim, 1.0f, false);
    SetState(this, kHudStateNames[1]);
}

// @func 0x0036CB58 HudScreen::Close
void HudScreen::Close()
{
    if ((animState >> 1) == 4)
        return;
    if (object)
        object->SetVisible(true, true);
    animState = (animState & 1u) | (4u << 1);
    PlayHudClip(animController, outroAnim, 1.0f, false);
    SetState(this, kHudStateNames[4]);
}

// @func 0x0036CC28 HudScreen::ShowIdle
void HudScreen::ShowIdle()
{
    animState = (animState & 1u) | (3u << 1);
    SetActive(true);
    SetState(this, kHudStateNames[3]);
}

// @func 0x0036CDA8 HudScreen::ClearLabels
void HudScreen::ClearLabels()
{
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* label = def->textFieldDefs[i].labelPath) {
            label->SetText("", false);
            label->Invalidate();
        }
    }
}

// @func 0x0036CCA0 HudScreen::SetLabelText
void HudScreen::SetLabelText(const char* text, uint32_t key)
{
    if (!text || key == 0)
        return;
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (def->textFieldDefs[i].field_1C != key)
            continue;
        if (ape::ApeGuiObject* label = def->textFieldDefs[i].labelPath) {
            label->SetText(text, false);
            label->Invalidate();
        }
        return;
    }
}

// @func 0x0036CEA0 HudScreen::Draw
void HudScreen::Draw()
{
    if (object->IsVisible()) {
        HideSubObjects();
        HudQueueObjectDraw2(object, false);
        for (ape::world::ApeObj* child = object->GetFirstChild(); child; child = child->GetNextSibling()) {
            if (child->IsVisible())
                HudQueueObjectDraw2(child, true);
        }
    }
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        const HudTextField& field = fields[i];
        if (!field.field_08 || !field.sceneObject || !field.sceneObject->IsVisible())
            continue;
        // The GUI's x and y axes point the other way from the scene's.
        ape::Matrix34 transform = field.sceneObject->GetWorld();
        transform.xAxis = {-transform.xAxis.x, -transform.xAxis.y, -transform.xAxis.z, 0.0f};
        transform.yAxis = {-transform.yAxis.x, -transform.yAxis.y, -transform.yAxis.z, 0.0f};
        field.field_08->SetTransform(transform);
        field.field_08->Tick(0.0f);
        field.field_08->Draw();
    }
}

// @func 0x0036D0EC HudScreen::DrawCallback
void HudScreen::DrawCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudScreen*>(context)->Draw();
}

} // namespace hud
} // namespace sly
