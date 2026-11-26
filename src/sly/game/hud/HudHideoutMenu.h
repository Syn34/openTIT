// openTIT - HUD element of definition type bit 0x200 (vtable 0x00B1F870,
// 0x270 bytes; built by 0x00359E08): the hideout menu. It has pages (each
// with its own fields and a pair of transition clip sets), a sprite map
// framed by four level instances, and borrows two prompt elements for its
// "back" and "select" texts while it is open (restoring their own texts when
// it closes).
//
// States (animState >> 1): 0 closed, 1/2 opening (with / without a page
// clip), 3 page transition, 5 open, 7 switching page, 8/9 closing.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace ape { class ApeGuiObject; class GuiScriptPlayer; }
namespace ape::world { class ApeObj; }
namespace agl { class AglMaterial; }

namespace sly {
namespace hud {

class HudHideoutMenu : public HudObject {
public:
    // Flag bits (+0x168, the upper half word).
    enum MenuFlags : uint32_t {
        kM_SelectWasShown = 0x00400000,   // bit 22: the select prompt was up when the menu opened
        kM_BackWasShown   = 0x00800000,   // bit 23: the back prompt was up when the menu opened
        kM_Closed         = 0x01000000,   // bit 24: a close ran
        kM_AnimOk         = 0x04000000,   // bit 26: the last menu clip started
        kM_Suppressed     = 0x08000000,   // bit 27
        kM_CloseReq       = 0x10000000,   // bit 28: close at the next update
        kM_OpenReq        = 0x20000000,   // bit 29: open at the next update
        kM_CloseFlag      = 0x40000000,   // bit 30: handed to Refresh: do not stay enabled
        kM_Enabled        = 0x80000000u,  // bit 31: the menu may open
    };
    enum MenuState : uint32_t {
        kMenu_Closed = 0, kMenu_Opening = 1, kMenu_OpeningPlain = 2, kMenu_Transition = 3,
        kMenu_Open = 5, kMenu_Switching = 7, kMenu_Closing = 8, kMenu_ClosingPlain = 9,
    };

    // 0x00359E08.
    explicit HudHideoutMenu(HudObjectDef* def);
    // 0x00370F70 / 0x00371304.
    ~HudHideoutMenu() override;
    // Slot 2 (0x003720B0): requests, label alpha, the child's pulse and
    // the open / switch / close state machine.
    void Update(float dt) override;
    // Slot 3 (0x003733BC).
    void Refresh() override;
    // Slot 5 (0x00370E30).
    void SetActive(bool active) override;
    // Slot 6 (0x003735C8).
    void SetEnabled(bool enabled, bool flag) override;

    // 0x00371BA8: opens on the first page and takes over the prompts.
    void BeginOpen();
    // 0x00371360: plays the closing clip and gives the prompts back.
    void BeginClose();
    // 0x00371968: plays the transition clips from the previous page to the
    // current one (state 3).
    void PlayTransition();
    // 0x00371AE8: closes an open menu.
    void Close(bool flag);
    // 0x0037202C: opens a closed menu (when enabled or `force`).
    void Open(bool force, void* context);
    // 0x003734B0: switches to the page named `name`.
    void SelectPage(uint32_t name);
    // 0x0037359C: name of the current page (0 out of range).
    uint32_t CurrentPageName() const;
    // 0x003736C4: asks an open menu to close at the next update.
    bool RequestClose(bool flag);
    // 0x0037370C: the GUI the menu's scripts run on (by path). While closed
    // or opening it becomes the GUI at once (restarting "Default_Init" when
    // the first page has no clip set); false while the menu is busy.
    bool SetGui(const char* path);
    // 0x003738AC: activates or deactivates the two prompt elements.
    void SetPromptsActive(bool active);
    // 0x003742E0: draws the text frames of page `page` at their objects.
    void DrawPageFields(uint32_t pageIndex);
    // 0x00374590: draws the current page's fields (and the previous page's
    // during a switch). World callback 0x003746D8.
    void DrawPages();
    static void DrawPagesCallback(void* unused, void* context);
    // 0x003739D0 (world callback): turns the child object (else the
    // element's object) about Y by `childAngle` degrees.
    static void TurnChildCallback(void* unused, void* context);

    ape::world::ApeObj*** pageFieldObjects = nullptr;   // +0xA4 per page, per field
    void* context = nullptr;              // +0xA8 given to Open
    agl::AglMaterial* mapMaterial = nullptr;   // +0xAC
    ape::world::ApeObj* corners[4] = {};  // +0xB0
    ape::world::ApeObj* child = nullptr;  // +0xC0
    ape::Vec3 field_D0{};                 // +0xD0
    ape::Vec3 field_E0{};                 // +0xE0
    ape::Vec3 field_F0{};                 // +0xF0
    ape::Vec3 field_100{};                // +0x100
    ape::Vec3 field_110{};                // +0x110
    ape::Vec3 mapMin{};                   // +0x120 corner 0
    ape::Vec3 mapMax{};                   // +0x130 corner 3
    ape::Vec3 mapSize{};                  // +0x140 corner 0 - corner 2
    ape::ApeGuiObject* nextGui = nullptr; // +0x150 becomes `gui` at the next transition
    ape::ApeGuiObject* gui = nullptr;     // +0x154 GUI the shutdown script runs on
    ape::GuiScriptPlayer* shutdownScript = nullptr;   // +0x158
    float pulseTimer = 0.0f;              // +0x15C
    float childAngle = 0.0f;              // +0x160 degrees (TurnChildCallback)
    uint16_t page = 0;                    // +0x164
    uint16_t previousPage = 0;            // +0x166
    uint32_t menuFlags = 0;               // +0x168 MenuFlags
    char backText[0x80] = {};             // +0x16A the back prompt's own text
    char selectText[0x80] = {};           // +0x1EA the select prompt's own text

private:
    bool CanAct() const { return !(menuFlags & kM_Suppressed) && (menuFlags & kM_Enabled); }
    void PlayMenuClip(uint32_t clip);
    void SetPageFieldsVisible(bool visible, bool only2d);
    void StartShutdownScript();
    // The menu's GUI script player is still running steps or actions.
    bool ScriptRunning() const;
    // Starts "Default_Init" on the menu's GUI unless a script runs.
    void StartInitScript();
    bool MenuAnimDone() const;
};

} // namespace hud
} // namespace sly
