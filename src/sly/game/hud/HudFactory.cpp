// openTIT - The HUD element factory: one element per loaded definition.
#include "sly/game/hud/HudCostumeWheel.h"
#include "sly/game/hud/HudElement.h"
#include "sly/game/hud/HudEpisodeElements.h"
#include "sly/game/hud/HudHideoutMenu.h"
#include "sly/game/hud/HudJobList.h"
#include "sly/game/hud/HudManager.h"
#include "sly/game/hud/HudPopup.h"
#include "sly/game/hud/HudPrompt.h"
#include "sly/game/hud/HudScreen.h"
#include "sly/game/hud/HudSubtitle.h"
#include "sly/game/hud/HudTracker.h"
#include "sly/game/hud/HudType8000.h"

namespace sly {
namespace hud {

namespace {

// Name of the element cached in HudManagerState::element394 (0x00F64D28).
constexpr uint32_t kElement394 = 0xBC38D74Au;
constexpr uint32_t kCoinCounter = 0xAC86E62Bu;   // "CoinCounter" (0x00F64CDC)

// The element class for a definition: the first type bit that matches, in
// the order the console tests them.
HudObject* CreateElement(HudObjectDef* def)
{
    const uint32_t mask = def->typeMask;
    if (mask & kHud_Object) {
        if (mask & kHud_Type4000)
            return new HudSubtitle(def, &def->timedLines);
        if (mask & kHud_Type20000)
            return new HudArElement(def);
        auto* plain = new HudPlainElement(def);
        plain->typeFlags |= kHud_Object;
        return plain;
    }
    if (mask & kHud_Type2)
        return new HudScreen(def);
    if (mask & kHud_Type4) {
        // Tracker lists are also updated every frame.
        auto* tracker = new HudTracker(def);
        Hud().updateList.Add(def->name, (tracker->typeFlags & kHud_Type4) ? tracker : nullptr);
        return tracker;
    }
    if (mask & kHud_Type40)
        return new HudPrompt(def);
    if (mask & kHud_Type200)
        return new HudHideoutMenu(def);
    if (mask & kHud_Type20)
        return new HudPopup(def);
    if (mask & kHud_Type400)
        return new HudCostumeWheel(def);
    if (mask & kHud_Type800)
        return new HudType800(def);
    if (mask & kHud_Type1000)
        return new HudType1000(def);
    if (mask & kHud_Type2000)
        return new HudJobList(def);
    if (mask & kHud_Type8000)
        return new HudType8000(def);
    if (mask & kHud_Type10000)
        return new HudType10000(def);
    return nullptr;
}

HudObject* FindObjectElement(uint32_t name)
{
    if (name == 0)
        return nullptr;
    HudObject** found = Hud().objects.Find(name);
    HudObject* element = found ? *found : nullptr;
    return (element && (element->typeFlags & kHud_Object)) ? element : nullptr;
}

} // namespace

// @func 0x0035AC7C BuildHudElements
void BuildHudElements()
{
    HudManagerState& hud = Hud();
    for (uint32_t i = 0; i < hud.defs.Count(); ++i) {
        HudObjectDef* def = hud.defs.Data()[i].value;
        if (!def)
            continue;
        if (HudObject* element = CreateElement(def))
            hud.objects.Add(def->name, element);
    }
    hud.element394 = FindObjectElement(kElement394);
    hud.coinCounter = FindObjectElement(kCoinCounter);
    hud.hudSuspended = false;
    hud.flagF64CC8 = false;
}

} // namespace hud
} // namespace sly
