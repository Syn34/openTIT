// openTIT - Hideout menu HUD element (type bit 0x200), see HudHideoutMenu.h.
#include "sly/game/hud/HudHideoutMenu.h"

#include "agl/AglResources.h"
#include "agl/AglTexture.h"
#include "sly/game/ObjectHelpers.h"
#include "sly/game/SlyGame.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/level/ApeGuiScript.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"

#include <cstdlib>

namespace sly {
namespace hud {

namespace {

// Menu clips. The transition and closing clips are picked by the page's
// clip set pair (a, b): a == b -> table "same" by a; a == 0 -> table
// "fromNone" by b; else table "from" by a (index 0 is never used).
constexpr uint32_t kClip_Open = 0x8D7B75C2u;
constexpr uint32_t kClip_ClosePlain = 0x8DD33161u;
const uint32_t kTransitionFrom[3] = {0, 0xB2F82A59u, 0xB2F82A5Au};       // 0x00ADC65C
const uint32_t kTransitionFromNone[3] = {0, 0x1AD95738u, 0x1AD95739u};   // 0x00ADC674
const uint32_t kTransitionSame[3] = {0, 0xEB029F3Cu, 0xEB029F3Du};       // 0x00ADC68C
const uint32_t kCloseFrom[3] = {0, 0xBCFA3661u, 0xBCFA3662u};            // 0x00ADC668
const uint32_t kCloseFromNone[3] = {0, 0xB143A780u, 0xB143A781u};        // 0x00ADC680
const uint32_t kCloseSame[3] = {0, 0x1C3AFE84u, 0x1C3AFE85u};            // 0x00ADC698

const uint8_t kNoTint[4] = {0, 0, 0, 0};
constexpr uint32_t kGuiEvent_Shutdown = 0x0E;   // NOTE: event sent with the page switch

// World callback lists of the menu.
constexpr int kCallbackList_6 = 6;
constexpr int kCallbackList_Draw = 8;
constexpr int kCallbackList_0 = 0;

uint32_t PickClip(const HudObjectDef::MenuPageDef& page, const uint32_t* same, const uint32_t* fromNone,
                  const uint32_t* from)
{
    if (page.animA == page.animB)
        return same[page.animA];
    if (page.animA == 0)
        return fromNone[page.animB];
    return from[page.animA];
}

// Text of an element's first label (null without one).
const char* FirstLabelText(HudObject* element)
{
    if (!element->def->textFieldCount || !element->textFields[0].label)
        return nullptr;
    return static_cast<ape::ApeGuiLabel*>(element->textFields[0].label)->GetText();
}

void SetFirstLabelText(HudObject* element, const char* text)
{
    if (!element->def->textFieldCount)
        return;
    if (ape::ApeGuiObject* label = element->textFields[0].label) {
        label->SetText(text, false);
        label->Invalidate();
    }
}

} // namespace

// @func 0x00359E08 HudHideoutMenu::HudHideoutMenu
HudHideoutMenu::HudHideoutMenu(HudObjectDef* def_) : HudObject(0, def_)
{
    const HudObjectDef::MenuDef& info = def->menu;
    menuFlags &= 0x003FFFFFu;
    typeFlags |= kHud_Type200;
    if (info.pageCount) {
        pageFieldObjects = static_cast<ape::world::ApeObj***>(std::calloc(info.pageCount, sizeof(ape::world::ApeObj**)));
        for (uint32_t p = 0; p < info.pageCount; ++p) {
            const HudObjectDef::MenuPageDef& pageDef = info.pages[p];
            if (!pageDef.fieldCount)
                continue;
            auto* objects = static_cast<ape::world::ApeObj**>(std::calloc(pageDef.fieldCount, sizeof(ape::world::ApeObj*)));
            for (uint32_t i = 0; i < pageDef.fieldCount; ++i)
                objects[i] = FindInHierarchy(object, pageDef.fields[i].objectName);
            pageFieldObjects[p] = objects;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (info.corners[i])
            corners[i] = ape::world::FindInstance(info.corners[i], 8);
    }
    // The map spans corner 0 .. corner 3; its size is measured to corner 2.
    if (corners[0] && corners[2] && corners[3]) {
        mapMin = corners[0]->GetWorld().translation;
        mapMax = corners[3]->GetWorld().translation;
        const ape::Vec3& c2 = corners[2]->GetWorld().translation;
        mapSize = {mapMin.x - c2.x, mapMin.y - c2.y, mapMin.z - c2.z, 0.0f};
    }
    agl::AglTexture* texture = agl::FindTexture(info.texture);
    if (texture)
        texture->SetWrap(1, 1, 1);
    mapMaterial = agl::CreateSpriteMaterial(texture);
    if (mapMaterial) {
        // Alpha blended, drawn from both sides.
        mapMaterial->blendEnable = true;
        mapMaterial->blendSrc = agl::gpu::BlendFactor::SrcAlpha;
        mapMaterial->blendEquation = agl::gpu::BlendEquation::Add;
        mapMaterial->blendDst = agl::gpu::BlendFactor::OneMinusSrcAlpha;
        mapMaterial->cullFrontFace = 0;
    }
    if (object) {
        for (ape::world::ApeObj* c = object->GetFirstChild(); c; c = c->GetNextSibling()) {
            if (c->GetNameHash() == info.childName) {
                child = c;
                break;
            }
        }
    }
    backText[0] = '\0';
    selectText[0] = '\0';
}

void HudHideoutMenu::SetPageFieldsVisible(bool visible, bool only2d)
{
    const HudObjectDef::MenuDef& info = def->menu;
    if (page >= info.pageCount)
        return;
    const HudObjectDef::MenuPageDef& pageDef = info.pages[page];
    for (uint32_t i = 0; i < pageDef.fieldCount; ++i) {
        if (ape::ApeGuiObject* field = pageDef.fields[i].object)
            field->SetVisible(only2d ? pageDef.fields[i].object3d != nullptr : visible);
    }
}

// @func 0x00370F70 HudHideoutMenu::~HudHideoutMenu
// @func 0x00371304 HudHideoutMenu::~HudHideoutMenu (deleting)
HudHideoutMenu::~HudHideoutMenu()
{
    delete shutdownScript;
    shutdownScript = nullptr;
    if (g_game) {
        g_game->RemoveCallback(kCallbackList_6, this, nullptr);
        g_game->RemoveCallback(kCallbackList_Draw, this, nullptr);
        g_game->RemoveCallback(kCallbackList_0, this, nullptr);
    }
    if (object)
        object->SetVisible(false, true);
    SetPageFieldsVisible(false, false);
    const HudObjectDef::MenuDef& info = def->menu;
    if (pageFieldObjects) {
        for (uint32_t p = 0; p < info.pageCount; ++p)
            std::free(pageFieldObjects[p]);
        std::free(pageFieldObjects);
        pageFieldObjects = nullptr;
    }
    if (mapMaterial) {
        mapMaterial->Release();
        mapMaterial = nullptr;
    }
}

// @func 0x00370E30 HudHideoutMenu::SetActive
void HudHideoutMenu::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    SetPageFieldsVisible(active, false);
}

// @func 0x003733BC HudHideoutMenu::Refresh
void HudHideoutMenu::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    context = nullptr;
    nextGui = nullptr;
    gui = nullptr;
    animState &= 1u;
    delete shutdownScript;
    shutdownScript = nullptr;
    const uint32_t old = menuFlags;
    menuFlags = old & 0xC8FFFFFFu;
    // A close with the flag set also drops the enable bit.
    if (old & kM_CloseFlag)
        menuFlags = old & 0x08FFFFFFu;
}

void HudHideoutMenu::PlayMenuClip(uint32_t clip)
{
    const bool played = animController && PlayHudClip(animController, clip, 1.0f, false);
    menuFlags = played ? (menuFlags | kM_AnimOk) : (menuFlags & ~kM_AnimOk);
}

void HudHideoutMenu::StartShutdownScript()
{
    shutdownScript = ape::PlayGuiScript("Default_Shutdown", gui, false, false);
}

// @func 0x00371BA8 HudHideoutMenu::BeginOpen
void HudHideoutMenu::BeginOpen()
{
    const HudObjectDef::MenuDef& info = def->menu;
    page = 0;
    menuFlags |= kM_Enabled;
    const bool plain = info.pages[0].animA == 0 && info.pages[0].animB == 0;
    animState = (animState & 1u) | ((plain ? kMenu_OpeningPlain : kMenu_Opening) << 1);
    if (context)
        return;
    // Remember the prompts' own state and texts, then put up ours.
    if (HudObject* back = info.backButton ? Find(info.backButton) : nullptr) {
        const bool shown = back->object && back->object->IsVisible();
        menuFlags = shown ? (menuFlags | kM_BackWasShown) : (menuFlags & ~kM_BackWasShown);
        if (const char* own = FirstLabelText(back))
            ape::StrCopyBounded(backText, own, sizeof backText);
    }
    if (HudObject* select = info.selectButton ? Find(info.selectButton) : nullptr) {
        const bool shown = select->object && select->object->IsVisible();
        menuFlags = shown ? (menuFlags | kM_SelectWasShown) : (menuFlags & ~kM_SelectWasShown);
        if (const char* own = FirstLabelText(select))
            ape::StrCopyBounded(selectText, own, sizeof selectText);
    }
    if (HudObject* back = info.backButton ? Find(info.backButton) : nullptr)
        back->ShowText(-1.0f, "$HIDEOUT_BUTTON_BACK", true, false, nullptr, kNoTint);
    if (HudObject* select = info.selectButton ? Find(info.selectButton) : nullptr)
        select->ShowText(-1.0f, "$HIDEOUT_BUTTON_SELECT", true, false, nullptr, kNoTint);
    PlayMenuClip(kClip_Open);
    if (object)
        object->SetVisible(true, true);
}

// @func 0x00371360 HudHideoutMenu::BeginClose
void HudHideoutMenu::BeginClose()
{
    const HudObjectDef::MenuDef& info = def->menu;
    const HudObjectDef::MenuPageDef& current = info.pages[page];
    const bool plain = current.animA == 0 && current.animB == 0;
    animState = (animState & 1u) | ((plain ? kMenu_ClosingPlain : kMenu_Closing) << 1);
    if (gui) {
        delete shutdownScript;
        shutdownScript = nullptr;
        StartShutdownScript();
    }
    // Prompts that were not up before go away; the others get their texts back.
    auto restore = [](uint32_t name, bool wasShown, const char* text) {
        if (!name)
            return;
        HudObject* element = Find(name);
        if (!element)
            return;
        if (!wasShown) {
            if (!element->Suppressed())
                element->PlayIntro(false, false);
        } else {
            SetFirstLabelText(element, text);
        }
    };
    restore(info.backButton, (menuFlags & kM_BackWasShown) != 0, backText);
    restore(info.selectButton, (menuFlags & kM_SelectWasShown) != 0, selectText);
    // NOTE: the clip set is read from the first page, not the current one.
    PlayMenuClip(plain ? kClip_ClosePlain : PickClip(info.pages[0], kCloseSame, kCloseFromNone, kCloseFrom));
    if (object)
        object->SetVisible(true, true);
    SetPageFieldsVisible(true, true);
    context = nullptr;
}

// @func 0x00371968 HudHideoutMenu::PlayTransition
void HudHideoutMenu::PlayTransition()
{
    animState = (animState & 1u) | (kMenu_Transition << 1);
    if (nextGui) {
        gui = nextGui;
        nextGui = nullptr;
    }
    const HudObjectDef::MenuDef& info = def->menu;
    const HudObjectDef::MenuPageDef& from = info.pages[previousPage];
    const HudObjectDef::MenuPageDef& to = info.pages[page];
    if (from.animA == to.animA && from.animB == to.animB)
        return;
    PlayMenuClip(PickClip(to, kTransitionSame, kTransitionFromNone, kTransitionFrom));
}

// @func 0x00371AE8 HudHideoutMenu::Close
void HudHideoutMenu::Close(bool flag)
{
    if (!CanAct() || (animState >> 1) != kMenu_Open)
        return;
    menuFlags = (menuFlags & ~kM_CloseFlag) | (flag ? kM_CloseFlag : 0u);
    BeginClose();
    menuFlags |= kM_Closed;
}

// @func 0x003735C8 HudHideoutMenu::SetEnabled
void HudHideoutMenu::SetEnabled(bool enabled, bool flag)
{
    if (!enabled) {
        Close(flag);
    } else if (CanAct() && (animState >> 1) == kMenu_Closed) {
        context = nullptr;
        BeginOpen();
    }
}

// @func 0x0037202C HudHideoutMenu::Open
void HudHideoutMenu::Open(bool force, void* openContext)
{
    if ((menuFlags & kM_Suppressed) || (!(menuFlags & kM_Enabled) && !force) || (animState >> 1) != kMenu_Closed)
        return;
    context = openContext;
    BeginOpen();
}

// @func 0x003734B0 HudHideoutMenu::SelectPage
void HudHideoutMenu::SelectPage(uint32_t name)
{
    const HudObjectDef::MenuDef& info = def->menu;
    for (uint32_t i = 0; i < info.pageCount; ++i) {
        if (info.pages[i].name != name)
            continue;
        previousPage = page;
        page = uint16_t(i);
        if ((animState >> 1) != kMenu_Open)
            return;
        animState = (animState & 1u) | (kMenu_Switching << 1);
        if (!gui || shutdownScript)
            return;
        StartShutdownScript();
        ape::SendGuiEvent(kGuiEvent_Shutdown, 0xFFFFFFFFu, gui, 0);
        return;
    }
}

// @func 0x0037359C HudHideoutMenu::CurrentPageName
uint32_t HudHideoutMenu::CurrentPageName() const
{
    if (page >= def->menu.pageCount)
        return 0;
    return def->menu.pages[page].name;
}

// @func 0x003736C4 HudHideoutMenu::RequestClose
bool HudHideoutMenu::RequestClose(bool flag)
{
    if (!CanAct() || (animState >> 1) != kMenu_Open)
        return false;
    menuFlags = (menuFlags & ~kM_CloseFlag) | (flag ? kM_CloseFlag : 0u) | kM_CloseReq;
    return true;
}

// @func 0x0037370C HudHideoutMenu::SetGui
bool HudHideoutMenu::SetGui(const char* path)
{
    const uint32_t state = animState >> 1;
    if (state == kMenu_Transition || state == kMenu_ClosingPlain || state == 4 || state == 6 ||
        state == kMenu_Switching)
        return false;
    ape::ApeGuiObject* found = path ? ape::FindGuiObject(path, nullptr) : nullptr;
    nextGui = found;
    const HudObjectDef::MenuPageDef& current = def->menu.pages[page];
    const bool plain = current.animA == 0 && current.animB == 0;
    if (state == kMenu_Closed || state == kMenu_OpeningPlain) {
        if (found) {
            gui = found;
            nextGui = nullptr;
            delete shutdownScript;
            shutdownScript = nullptr;
        }
        if (plain && !shutdownScript && gui) {
            shutdownScript = ape::PlayGuiScript("Default_Init", gui, false, false);
            ape::SendGuiEvent(0x0C, 0xFFFFFFFFu, gui, 0);
        }
    }
    return true;
}

// @func 0x003738AC HudHideoutMenu::SetPromptsActive
void HudHideoutMenu::SetPromptsActive(bool active)
{
    const HudObjectDef::MenuDef& info = def->menu;
    if (HudObject* back = info.backButton ? Find(info.backButton) : nullptr)
        back->SetActive(active);
    if (HudObject* select = info.selectButton ? Find(info.selectButton) : nullptr)
        select->SetActive(active);
}

// @func 0x003739D0 HudHideoutMenu::TurnChildCallback
void HudHideoutMenu::TurnChildCallback(void* unused, void* context)
{
    (void)unused;
    auto* menu = static_cast<HudHideoutMenu*>(context);
    if (!menu)
        return;
    ape::world::ApeObj* target = menu->child ? menu->child : menu->object;
    if (!target || !menu->object)
        return;
    ape::Matrix34 turn;
    ape::Matrix34RotationY(menu->childAngle * 0.017453292f, turn);
    target->m_world = turn;
    target->m_flags &= ~ape::world::kObjFlag_Dirty;
    target->SetLocal(turn);
    target->MarkDirty();
    menu->object->UpdateWorld(false, false);
}

// @func 0x003742E0 HudHideoutMenu::DrawPageFields
void HudHideoutMenu::DrawPageFields(uint32_t pageIndex)
{
    const HudObjectDef::MenuDef& info = def->menu;
    if (pageIndex >= info.pageCount)
        return;
    const HudObjectDef::MenuPageDef& pageDef = info.pages[pageIndex];
    for (uint32_t i = 0; i < pageDef.fieldCount; ++i) {
        const HudObjectDef::MenuFieldDef& field = pageDef.fields[i];
        const ape::world::ApeObj* anchor = pageFieldObjects[pageIndex] ? pageFieldObjects[pageIndex][i] : nullptr;
        if (!field.object3d || !anchor || !field.object || !field.object->IsVisible())
            continue;
        // The GUI's x and y axes point the other way from the scene's.
        ape::Matrix34 transform = anchor->GetWorld();
        transform.xAxis = {-transform.xAxis.x, -transform.xAxis.y, -transform.xAxis.z, 0.0f};
        transform.yAxis = {-transform.yAxis.x, -transform.yAxis.y, -transform.yAxis.z, 0.0f};
        field.object3d->SetTransform(transform);
        field.object3d->Tick(0.0f);
        field.object3d->Draw();
    }
}

// NOTE: the console also projects three of its points (+0xE0, +0x100,
// +0xF0) to the screen here without using the results.
// @func 0x00374590 HudHideoutMenu::DrawPages
void HudHideoutMenu::DrawPages()
{
    if (page != previousPage)
        DrawPageFields(previousPage);
    DrawPageFields(page);
}

// @func 0x003746D8 HudHideoutMenu::DrawPagesCallback
void HudHideoutMenu::DrawPagesCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudHideoutMenu*>(context)->DrawPages();
}

// TODO(0x003720B0 HudHideoutMenu::Update): the menu state machine, the map
// and the page fields are not ported.
void HudHideoutMenu::Update(float dt)
{
    (void)dt;
}

} // namespace hud
} // namespace sly
