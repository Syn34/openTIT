// openTIT - Calls from the vehicle code into systems that are not ported yet.
// VehicleHooks.cpp holds TODO stubs with safe defaults; each declaration names
// the console function or member it stands for.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {
namespace entity { class Entity; class EntityComponent; }
}
namespace ape::world { class ApeObj; }
namespace sly {
namespace objects {

// Word +0x28 bit 12 of the entity's component of type 3. NOTE: the
// component class is not ported; vehicles ignore hits while it is set.
bool VehicleComponent03Blocks(entity::EntityComponent* component);
// Plays a clip through the character component (0x002775A8, blend 1.0 over
// 0.25 s), or through entity virtual slot 50 when there is none.
void VehiclePlayClip(entity::Entity* entity, uint32_t clipHash);

// -- players and pads ---------------------------------------------------------------
// Pad (application player block) that drives game player `player` (game
// world +0x1071 + player * 0x20C80).
uint32_t PlayerPadIndex(uint32_t player);
// Buttons held on a pad (application player block +0xB8, 0x010B1A98 + pad *
// 0x2EC0) and a stick axis (0x0045B160; 0/1 left X/Y).
uint32_t PadButtons(uint32_t pad);
float PadAxis(uint32_t pad, int axis);
// Pad button bits the vehicles read.
enum VehiclePadButton : uint32_t {
    kPad_DpadDown  = 0x0001,
    kPad_DpadUp    = 0x0002,
    kPad_DpadLeft  = 0x0004,
    kPad_DpadRight = 0x0008,
    kPad_Boost     = 0x0100,
    kPad_Fire      = 0x0200,
};
// The power meter of a game player (player block +0x10DC; +0x10D8 holds
// its capacity). Spending power also sets flag 0x40 of the block's word
// +0x1070.
float PlayerPower(uint32_t player);
void PlayerSetPower(uint32_t player, float power);
void PlayerSetPowerCapacity(uint32_t player, float capacity);
void PlayerMarkPowerUsed(uint32_t player);
// Puts the power meter back to the defaults (0x0045C81C on the player block).
void PlayerResetPower(uint32_t player);

// -- components -----------------------------------------------------------------
// Animation driver (type 0x17): requests a state (bits 20-24 of +0x18,
// 0x00276558).
void AnimDriverRequestState(entity::EntityComponent* driver, uint32_t state);
// Skin component (type 0x13): the value +0x4D8 and its refresh (0x002972C0),
// the "firing" flag (bit 30 of +0x4F4) and the target it aims at (0x00296758).
void SkinSetValue4D8(entity::EntityComponent* skin, uint32_t value);
void SkinSetFiring(entity::EntityComponent* skin, bool firing);
void SkinSetTarget(entity::EntityComponent* skin, entity::Entity* target);
// Component of type 1: the entities it lists (0x00271C08 walks them; null
// after the last).
entity::Entity* Component01Next(entity::EntityComponent* component, entity::Entity* previous);

// Character component: pauses an animation layer (0x00254814).
void CharacterPauseLayer(entity::EntityComponent* character, uint32_t layer);

// Sets material constant `constant` to `value` on every material of the
// world object's geometry that uses technique `technique` (the geometry
// instance +0xD0, its meshes and their material lists; 0x0050C018).
void ObjectSetTechniqueConstant(ape::world::ApeObj* object, uint32_t technique, int constant, uint32_t value);

// -- view -----------------------------------------------------------------------
// Screen position (0..1) of a world point on the main render target (game
// world +0x128168, 0x00522750); false when there is no render target.
bool MainViewWorldToScreen(const ape::Vec3& position, float out[2]);

} // namespace objects
} // namespace sly
