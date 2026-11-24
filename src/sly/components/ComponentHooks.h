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

// AI brain of a ControlAI component fills the control input (0x000250A0).
void AIBrainFillInput(void* brain, uint8_t* input);
// Anim trees and blend maps by name (0x003482B0 / 0x00348650).
void* FindAnimTree(uint32_t nameHash);
void* FindAnimBlendMap(uint32_t nameHash);
// IK component update (0x002934E0) and vortex pull (0x002B0B10) are not ported.

// <BossGrizz> component parser (forwards to levels::ai::ParseBossGrizzComponentDesc).
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                       uint8_t scope);

} // namespace components
} // namespace sly
