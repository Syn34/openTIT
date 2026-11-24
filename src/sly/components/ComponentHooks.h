// openTIT - Calls out of the component code into systems that are not
// ported yet. Each is a TODO stub until its owner exists.
#pragma once

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }
namespace sly { namespace entity { struct ComponentDesc; } }

namespace sly {
namespace components {

// Physics world of the running level (game world 0x00F65CA0 +0x1281D4).
void* LevelPhysicsWorld();

// <AI> component parser (0x00017420, sly/levels range, not ported).
entity::ComponentDesc* ParseAIComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
// <BossGrizz> component parser (0x00063D54, not ported yet).
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                       uint8_t scope);

} // namespace components
} // namespace sly
