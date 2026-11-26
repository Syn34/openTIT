// openTIT - Weather trigger volumes.
#include "sly/objects/TriggerWeather.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

namespace sly {
namespace objects {

namespace {
constexpr uint32_t kWeatherFilter = 0x11;   // collision filter of weather volumes
}

WeatherTriggerDefDesc::WeatherTriggerDefDesc(const EntityDefDesc& loader)
    : TriggerDefDesc(kDefClass_WeatherTrigger | kDefClass_Trigger, loader, "TriggerWeather_")
{
}

// @func 0x002220C8 WeatherTriggerDefDesc::Parse
void WeatherTriggerDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                  const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc::Parse(root, elem, loader, parent);
    uint32_t defaultScript = 0;
    float defaultBlend = 15.0f;
    if (parent && parent->IsClass(kDefClass_WeatherTrigger)) {
        const WeatherTriggerDef* from = static_cast<const WeatherTriggerDef*>(parent);
        // NOTE: the console hashes an empty string here, so a parent's script
        // is never inherited (only the blend time is).
        if (from->script)
            defaultScript = ape::HashNameNoCase("");
        defaultBlend = from->blendTime;
    }
    elem->GetHash(kAttr_WeatherScript, &script, defaultScript);
    elem->GetFloat(kAttr_WeatherBlendTime, &blendTime, defaultBlend);
}

// @func 0x00221F3C WeatherTriggerDef::WeatherTriggerDef
WeatherTriggerDef::WeatherTriggerDef(const WeatherTriggerDefDesc& desc)
    : TriggerDef(desc), blendTime(desc.blendTime)
{
    if (ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0))
        script = library->FindScript(desc.script);
}

// @func 0x002221D8 CreateWeatherTriggerDef
EntityDef* CreateWeatherTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                   const EntityDefDesc& loader, const EntityDef* parent)
{
    WeatherTriggerDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    return new WeatherTriggerDef(desc);
}

// @func 0x002222F0 WeatherTriggerDef::Create
Entity* WeatherTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kDefClass_WeatherTrigger) != 0))
        return nullptr;
    TriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    WeatherTrigger* trigger = new WeatherTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

WeatherTrigger::WeatherTrigger(const TriggerInstanceDesc& desc) : TriggerBase(desc, 0x20, true)
{
    if (m_volume)
        VolumeSetCollisionFilter(m_volume, kWeatherFilter);
}

// @func 0x002225B0 WeatherTrigger::Release
bool WeatherTrigger::Release(bool immediate)
{
    if (immediate) {
        for (auto& plane : m_scripts)
            for (auto& row : plane)
                for (void* script : row)
                    if (script)
                        ScriptInstanceSetPaused(script, true);
    }
    return TriggerBase::Release(immediate);
}

// @func 0x00223C60 WeatherTrigger::IsDebugDisplayed
bool WeatherTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerWeather.Get());
}

} // namespace objects
} // namespace sly
