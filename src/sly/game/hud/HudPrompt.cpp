// openTIT - "Press to continue" HUD element (vtable 0x00B1F800).
#include "sly/game/hud/HudPrompt.h"

#include "sly/anim/AnimController.h"
#include "sly/game/hud/HudHooks.h"
#include "sly/game/hud/HudScreen.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"

namespace sly {
namespace hud {

uint32_t g_promptScreenName = 0;

namespace {

ape::world::ApeObj* FindSceneChild(ape::world::ApeObj* root, uint32_t name)
{
    if (!root || name == 0)
        return nullptr;
    if (root->GetNameHash() == name)
        return root;
    return anim::FindDescendant(root, name);
}

} // namespace

// @func 0x0035A258 HudPrompt::HudPrompt
HudPrompt::HudPrompt(HudObjectDef* def_) : HudObject(0, def_)
{
    sound = FindHudSound(kConfirmSound);
    typeFlags |= kHud_Type40;
    if (object) {
        for (int i = 0; i < 3; ++i)
            sceneObjects[i] = FindSceneChild(object, def->promptObjects[i]);
    }
    for (int i = 0; i < 3; ++i)
        guiObjects[i] = def->promptGui[i];
}

// @func 0x0036F1FC HudPrompt::~HudPrompt
// @func 0x0036F3EC HudPrompt::~HudPrompt (deleting)
HudPrompt::~HudPrompt()
{
    audio::SoundNameRelease(sound);
    GameWorldRemoveHudCallback(this);
    for (ape::ApeGuiObject* gui : guiObjects) {
        if (gui)
            gui->SetVisible(false);
    }
}

// @func 0x0036F438 HudPrompt::Update
void HudPrompt::Update(float dt)
{
    if ((animState >> 1) == 0)
        return;
    GameWorldAddHudCallback(this, 8);
    HudObject* found = FindType2(g_promptScreenName);
    auto* screen = static_cast<HudScreen*>(found);
    if (step < 1) {
        if (step >= 0 && ConfirmButtonPressed()) {
            PlayHudSound(sound);
            if (screen) {
                screen->Open();
                screen->ClearLabels();
            }
            step = 1;
        }
    } else if (step < 2) {
        if (!screen || HudClipDone(screen->animController)) {
            GameWorldContinue();
            step = 2;
            SetActive(false);
        }
    }
    if ((animState >> 1) == 1 && HudClipDone(animController)) {
        animState = (animState & 1u) | (2u << 1);
        PlayHudClip(animController, idleClip, 1.0f, true);
        if (animScript)
            static_cast<ape::ApeAnimScriptInstance*>(animScript)->SetStateAndUpdate(kHudStateNames[3]);
        for (ape::ApeGuiObject* gui : guiObjects) {
            if (gui)
                gui->SetVisible(true);
        }
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

// @func 0x0036F8C4 HudPrompt::Refresh
void HudPrompt::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    for (ape::ApeGuiObject* gui : guiObjects) {
        if (gui)
            gui->SetVisible(false);
    }
    step = 0;
    animState = 1;
}

// @func 0x0036F9CC HudPrompt::SetActive
void HudPrompt::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    for (ape::ApeGuiObject* gui : guiObjects) {
        if (gui)
            gui->SetVisible(active);
    }
}

// @func 0x0036FAC8 HudPrompt::SetEnabled
void HudPrompt::SetEnabled(bool, bool) {}

// @func 0x0036FAD0 HudPrompt::Show
void HudPrompt::Show(const char* text_)
{
    constexpr uint32_t kScript_Intro = 0x04296E4Cu;   // "Intro"
    if ((animState >> 1) == 1)
        return;
    step = 0;
    animState = (animState & 1u) | (1u << 1);
    if (object)
        object->SetVisible(true, true);
    for (ape::ApeGuiObject* gui : guiObjects) {
        if (gui)
            gui->SetVisible(true);
    }
    if (guiObjects[1] && guiObjects[1]->GetType() == ape::kGuiObject_Text) {
        guiObjects[1]->SetText(text_, false);
        guiObjects[1]->Invalidate();
    }
    PlayHudClip(animController, introClip, 1.0f, false);
    if (animScript)
        static_cast<ape::ApeAnimScriptInstance*>(animScript)->SetStateAndUpdate(kScript_Intro);
}

namespace {

void DrawAt(ape::ApeGuiObject* gui, const ape::world::ApeObj* anchor)
{
    // The GUI's x and y axes point the other way from the scene's.
    ape::Matrix34 transform = anchor->GetWorld();
    transform.xAxis = {-transform.xAxis.x, -transform.xAxis.y, -transform.xAxis.z, 0.0f};
    transform.yAxis = {-transform.yAxis.x, -transform.yAxis.y, -transform.yAxis.z, 0.0f};
    gui->SetTransform(transform);
    gui->Tick(0.0f);
    gui->Draw();
    gui->Invalidate();
}

} // namespace

// @func 0x0036FCD0 HudPrompt::Draw
void HudPrompt::Draw()
{
    if (guiObjects[0] && sceneObjects[0])
        DrawAt(guiObjects[0], sceneObjects[0]);
    if (guiObjects[1] && sceneObjects[1])
        DrawAt(guiObjects[1], sceneObjects[1]);
    // NOTE: the console tests the first GUI object for the third pair.
    if (guiObjects[0] && sceneObjects[2] && guiObjects[2])
        DrawAt(guiObjects[2], sceneObjects[2]);
}

// @func 0x00370104 HudPrompt::DrawCallback
void HudPrompt::DrawCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudPrompt*>(context)->Draw();
}

} // namespace hud
} // namespace sly
