// openTIT - Vehicle hooks (TODO stubs, see VehicleHooks.h).
#include "sly/objects/VehicleHooks.h"

namespace sly {
namespace objects {

bool VehicleComponent03Blocks(entity::EntityComponent*) { return false; }
void VehiclePlayClip(entity::Entity*, uint32_t) {}

uint32_t PlayerPadIndex(uint32_t player) { return player; }
uint32_t PadButtons(uint32_t pad) { (void)pad; return 0; }
float PadAxis(uint32_t pad, int axis) { (void)pad; (void)axis; return 0.0f; }
float PlayerPower(uint32_t player) { (void)player; return 0.0f; }
void PlayerSetPower(uint32_t player, float power) { (void)player; (void)power; }
void PlayerSetPowerCapacity(uint32_t player, float capacity) { (void)player; (void)capacity; }
void PlayerMarkPowerUsed(uint32_t player) { (void)player; }
void PlayerResetPower(uint32_t player) { (void)player; }
void CharacterPauseLayer(entity::EntityComponent* character, uint32_t layer) { (void)character; (void)layer; }

void AnimDriverRequestState(entity::EntityComponent* driver, uint32_t state) { (void)driver; (void)state; }
void SkinSetValue4D8(entity::EntityComponent* skin, uint32_t value) { (void)skin; (void)value; }
void SkinSetFiring(entity::EntityComponent* skin, bool firing) { (void)skin; (void)firing; }
void SkinSetTarget(entity::EntityComponent* skin, entity::Entity* target) { (void)skin; (void)target; }
entity::Entity* Component01Next(entity::EntityComponent* component, entity::Entity* previous)
{
    (void)component;
    (void)previous;
    return nullptr;
}

void ObjectSetTechniqueConstant(ape::world::ApeObj* object, uint32_t technique, int constant, uint32_t value)
{
    (void)object; (void)technique; (void)constant; (void)value;
}

bool MainViewWorldToScreen(const ape::Vec3& position, float out[2])
{
    (void)position;
    out[0] = out[1] = 0.5f;
    return false;
}

} // namespace objects
} // namespace sly
