// openTIT - <OffscreenMarker> component (type 0x1E, vtable 0x00B1CF28, 0x50
// bytes): two HUD markers following the owner, one used while it is on
// screen and one while it is off screen, each optionally recoloured.
#pragma once

#include "sly/components/ComponentHooks.h"
#include "sly/components/Components.h"

namespace sly {
namespace components {

class OffscreenMarkerComponent : public EntityComponent {
public:
    explicit OffscreenMarkerComponent(const OffscreenMarkerComponentDesc* desc);
    ~OffscreenMarkerComponent() override;
    // TODO(0x002BB3F0 OffscreenMarkerComponent::PostUpdate): places the
    // markers (screen projection; not ported).
    // Enables the markers; disabling hides them (slot 11).
    void SetOption(bool enabled) override;

    uint32_t onscreenTemplate = 0;     // 0x18
    uint32_t offscreenTemplate = 0;    // 0x1C
    float rangeSq = -1.0f;             // 0x20
    void* onscreenMarker = nullptr;    // 0x24 HUD element
    void* offscreenMarker = nullptr;   // 0x28
    entity::Entity* target = nullptr;  // 0x2C referenced
    entity::Entity* target2 = nullptr; // 0x30 referenced (same entity)
    // Flag word 0x34: bit 31 enabled, 29/28 colour overrides, 27 the
    // description's flag28 (makes the destructor hide at once).
    bool startEnabled = true;
    bool overrideOnscreen = false;
    bool overrideOffscreen = false;
    bool hideImmediately = false;
    ape::xml::Color8 onscreenColor{};  // 0x35
    ape::xml::Color8 offscreenColor{}; // 0x39
    MarkerColorOverride onscreenOverride;   // 0x40
    MarkerColorOverride offscreenOverride;  // 0x48

private:
    void HideMarker(void* marker);
};

} // namespace components
} // namespace sly
