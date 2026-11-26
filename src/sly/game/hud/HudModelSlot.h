// openTIT - A 3D model shown by the HUD: an entity spawned from a style
// record's "Entity" definition, viewed through its "Camera" definition and
// playing its "IdleAnim" clip (0x0036A504 / 0x0036A5F0). The binocucom
// component keeps two of these (its scan target previews).
#pragma once

#include <cstdint>

namespace ape { namespace world { class ApeDef; } }
namespace sly { namespace entity { class Entity; } }

namespace sly {
namespace hud {

struct HudColorStyle;

// 0x18 bytes on the console.
struct HudModelSlot {
    const HudColorStyle* style = nullptr;   // +0x00 record the model comes from
    entity::Entity* entity = nullptr;       // +0x04 spawned model (one reference held)
    uint32_t field_08 = 0;                  // +0x08 NOTE: not touched here
    ape::world::ApeDef* camera = nullptr;   // +0x0C camera definition named by the style
    uint32_t styleName = 0;                 // +0x10 the style's name (0 while empty)
    uint32_t field_14 = 0;                  // +0x14

    // Hides and releases the model.
    void Clear();
    // Shows the model of `style` (nothing for null). `clip` overrides the
    // style's idle clip when non-zero.
    void Set(const HudColorStyle* style, uint32_t clip);
};

} // namespace hud
} // namespace sly
