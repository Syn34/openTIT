// openTIT - HUD object registry.
#include "sly/game/hud/HudManager.h"

#include "ape/core/ApeString.h"

#include <cstdio>

namespace sly {
namespace hud {

namespace {

HudObject* FindTyped(uint32_t name, uint32_t type)
{
    if (name == 0)
        return nullptr;
    HudObject** slot = Hud().objects.Find(name);
    HudObject* object = slot ? *slot : nullptr;
    if (!object || !(object->typeFlags & type))
        return nullptr;
    return object;
}

} // namespace

HudManagerState& Hud()
{
    static HudManagerState state;
    return state;
}

bool GuiIsPlaying(const void* gui)
{
    // TODO: GUI instance flags (+0xA4 bit 15) once the GUI module exists.
    (void)gui;
    return false;
}

// TODO(0x00363A3C HudObject::ShowText)
void HudObject::ShowText(float fade, const char* text, bool flag)
{
    (void)fade;
    (void)text;
    (void)flag;
}

// TODO(0x00362F1C HudObject::PlayIntro)
void HudObject::PlayIntro(bool a, bool b)
{
    (void)a;
    (void)b;
}

// TODO(0x00362B40 HudObject::PlayOutro)
void HudObject::PlayOutro(bool a, bool b)
{
    (void)a;
    (void)b;
}

// @func 0x003568E0 hud::Find
HudObject* Find(uint32_t name) { return FindTyped(name, kHud_Object); }
// @func 0x00365E30 hud::FindType2
HudObject* FindType2(uint32_t name) { return FindTyped(name, kHud_Type2); }
// @func 0x00368FA0 hud::FindType4
HudObject* FindType4(uint32_t name) { return FindTyped(name, kHud_Type4); }
// @func 0x00365EAC hud::FindType40
HudObject* FindType40(uint32_t name) { return FindTyped(name, kHud_Type40); }
// @func 0x00365BA4 hud::FindType400
HudObject* FindType400(uint32_t name) { return FindTyped(name, kHud_Type400); }
// @func 0x00368E2C hud::FindType800
HudObject* FindType800(uint32_t name) { return FindTyped(name, kHud_Type800); }
// @func 0x00368EA8 hud::FindType1000
HudObject* FindType1000(uint32_t name) { return FindTyped(name, kHud_Type1000); }
// @func 0x00368F24 hud::FindType2000
HudObject* FindType2000(uint32_t name) { return FindTyped(name, kHud_Type2000); }
// @func 0x00368DB0 hud::FindType4000
HudObject* FindType4000(uint32_t name) { return FindTyped(name, kHud_Type4000); }
// @func 0x00368A48 hud::FindType8000
HudObject* FindType8000(uint32_t name) { return FindTyped(name, kHud_Type8000); }
// @func 0x0035B908 hud::FindType10000
HudObject* FindType10000(uint32_t name) { return FindTyped(name, kHud_Type10000); }
// @func 0x00365B28 hud::FindType20000
HudObject* FindType20000(uint32_t name) { return FindTyped(name, kHud_Type20000); }

// @func 0x00365C24 hud::FindActiveType200
HudObject* FindActiveType200()
{
    HudManagerState& state = Hud();
    if (state.activeType200Name != 0)
        if (HudObject* object = FindTyped(state.activeType200Name, kHud_Type200))
            return object;
    return FindTyped(state.fallbackType200Name, kHud_Type200);
}

// @func 0x00367518 hud::SetSuppressed
void SetSuppressed(HudObject* object, bool suppressed)
{
    if (!object)
        return;
    object->stateFlags = suppressed ? (object->stateFlags | HudObject::kState_Suppressed)
                                    : (object->stateFlags & ~HudObject::kState_Suppressed);
}

// @func 0x0036735C hud::ShowTextByName
void ShowTextByName(float fade, uint32_t name, const char* text, bool flag)
{
    if (HudObject* object = Find(name))
        object->ShowText(fade, text, flag);
}

namespace {
void ShowTyped(uint32_t name, uint32_t type, bool a, bool intro, bool b)
{
    HudObject* object = FindTyped(name, type);
    if (!object || object->Suppressed())
        return;
    if (intro)
        object->PlayIntro(a, b);
    else
        object->PlayOutro(a, b);
}
} // namespace

// @func 0x0036742C hud::ShowByName
void ShowByName(uint32_t name, bool a, bool intro, bool b)
{
    ShowTyped(name, kHud_Object, a, intro, b);
}

// @func 0x00367B28 hud::ShowType4000ByName
void ShowType4000ByName(uint32_t name, bool a, bool intro, bool b)
{
    ShowTyped(name, kHud_Type4000, a, intro, b);
}

// @func 0x00367134 hud::CountNamedItems
int CountNamedItems(uint32_t name)
{
    int count = 0;
    for (auto& list : Hud().namedLists)
        for (HudNamedItem* item : list)
            if (item->target->name == name)
                ++count;
    return count;
}

// @func 0x00369D00 hud::SetContext
void SetContext(uint32_t value, uint32_t name)
{
    Hud().contextName = name;
    Hud().contextValue = value;
}

// @func 0x00369D14 hud::SetContextByString
void SetContextByString(uint32_t value, const char* name)
{
    Hud().contextName = ape::HashNameNoCase(name);
    Hud().contextValue = value;
}

// @func 0x0036960C hud::RequestScreen
void RequestScreen(bool skip, const char* name)
{
    if (skip)
        return;
    Hud().pendingScreenName = ape::HashNameNoCase(name);
    Hud().pendingScreen = true;
}

// @func 0x00369CAC hud::SetPowMeterPrefix
void SetPowMeterPrefix(const char* prefix)
{
    char name[0x60];
    std::snprintf(name, sizeof(name), "%sPOW", prefix);
    Hud().powMeterName = ape::HashNameNoCase(name);
}

// @func 0x00369D58 hud::SnapMeterA
void SnapMeterA()
{
    if (HudObject* meter = Hud().meterA)
        meter->shownValue = meter->targetValue;
}

// @func 0x00369658 hud::SnapMeterB
void SnapMeterB()
{
    if (HudObject* meter = Hud().meterB)
        meter->shownValue = meter->targetValue;
}

// @func 0x00369500 hud::ResetUpdateFlags
void ResetUpdateFlags()
{
    auto& list = Hud().updateList;
    for (uint32_t i = 0; i < list.Count(); ++i) {
        HudObject* object = list.Data()[i].value;
        object->updateFlags &= 0x7FFFFFFFu;
        object->Refresh();
    }
}

// @func 0x003695A0 hud::AnyUpdatingElementVisible
bool AnyUpdatingElementVisible()
{
    auto& list = Hud().updateList;
    for (uint32_t i = 0; i < list.Count(); ++i) {
        HudObject* object = list.Data()[i].value;
        if (object->gui && GuiIsPlaying(object->gui))
            return true;
    }
    return false;
}

} // namespace hud
} // namespace sly
