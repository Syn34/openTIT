// openTIT - <OffscreenMarker> component (0x00257A40, 0x002BA884..0x002BB3F0).
#include "sly/components/OffscreenMarker.h"

#include "ape/core/ApeString.h"

namespace sly {
namespace components {

namespace {
uint32_t g_markerSerial = 0;   // 0x00F64BEC

// A fresh instance of a marker template ("CompOffscreenMarker<n>").
void* InstanceMarker(uint32_t templateName)
{
    if (templateName == 0)
        return nullptr;
    ++g_markerSerial;
    const char* name = ape::TempFormat("CompOffscreenMarker%u", g_markerSerial);
    void* marker = HudCloneObject(templateName, ape::HashNameNoCase(name));
    if (!marker)
        (void)ape::TempHexString(templateName);   // console warning text
    return marker;
}

void AssignEntityRef(entity::Entity*& slot, entity::Entity* value)
{
    if (slot)
        slot->Release(false);
    slot = value;
    if (value && value->IsAlive())
        value->AddRef();
}
} // namespace

void OffscreenMarkerComponent::HideMarker(void* marker)
{
    if (!marker)
        return;
    HudObjectSetActive(marker, false);
    if (HudObjectAnimState(marker) == 0)
        HudObjectShow(marker, false, true, false);
}

// @func 0x00257A40 OffscreenMarkerComponent::OffscreenMarkerComponent
OffscreenMarkerComponent::OffscreenMarkerComponent(const OffscreenMarkerComponentDesc* desc)
    : EntityComponent(desc)
    , onscreenTemplate(desc->hash14)
    , offscreenTemplate(desc->hash18)
    , rangeSq(desc->rangeSq)
    , startEnabled(desc->startEnabled)
    , overrideOnscreen(desc->hasOnscreenColor)
    , overrideOffscreen(desc->hasOffscreenColor)
    , hideImmediately(desc->flag28)
    , onscreenColor(desc->onscreenColor)
    , offscreenColor(desc->offscreenColor)
{
    onscreenMarker = InstanceMarker(onscreenTemplate);
    offscreenMarker = InstanceMarker(offscreenTemplate);
    if (onscreenMarker && overrideOnscreen)
        MarkerSetColorOverride(onscreenMarker, onscreenOverride, true);
    if (offscreenMarker && overrideOffscreen)
        MarkerSetColorOverride(offscreenMarker, offscreenOverride, true);

    AssignEntityRef(target, owner);
    AssignEntityRef(target2, owner);

    enabled = startEnabled;
    if (!startEnabled) {
        HideMarker(onscreenMarker);
        HideMarker(offscreenMarker);
    }
}

// @func 0x002BA884 OffscreenMarkerComponent::~OffscreenMarkerComponent
// @func 0x002BABC4 OffscreenMarkerComponent::~OffscreenMarkerComponent (deleting)
OffscreenMarkerComponent::~OffscreenMarkerComponent()
{
    for (void* marker : { onscreenMarker, offscreenMarker }) {
        if (!marker)
            continue;
        if (HudObjectAnimState(marker) == 0 || hideImmediately)
            HudObjectHideNow(marker);
        else
            HudObjectShow(marker, false, true, true);
        HudObjectSetActive(marker, false);
    }
    onscreenMarker = nullptr;
    offscreenMarker = nullptr;
    if (target)
        target->Release(false);
    target = nullptr;
    if (target2)
        target2->Release(false);
    target2 = nullptr;
    GameWorldForgetColorOverride(onscreenOverride);
    GameWorldForgetColorOverride(offscreenOverride);
    if (onscreenOverride.material) {
        MaterialRelease(onscreenOverride.material);
        onscreenOverride.material = nullptr;
    }
    if (offscreenOverride.material) {
        MaterialRelease(offscreenOverride.material);
        offscreenOverride.material = nullptr;
    }
}

// @func 0x0025773C OffscreenMarkerComponent::SetOption
void OffscreenMarkerComponent::SetOption(bool on)
{
    enabled = on;
    if (!on) {
        HideMarker(onscreenMarker);
        HideMarker(offscreenMarker);
    }
}

} // namespace components
} // namespace sly
