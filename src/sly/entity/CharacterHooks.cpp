// openTIT - Stubs for CharacterHooks.h.
#include "sly/entity/CharacterHooks.h"

#include "sly/game/goals/JobManager.h"

namespace sly {
namespace entity {

uint32_t CharacterCompModel(EntityComponent* component) { (void)component; return 0; }
uint32_t CharacterCompDefaultModel(EntityComponent* component) { (void)component; return 0; }
void CharacterCompSetModel(EntityComponent* component, uint32_t model) { (void)component; (void)model; }
void CharacterCompResetPose(EntityComponent* component) { (void)component; }
void CharacterCompSetAnimController(EntityComponent* component, void* controller) { (void)component; (void)controller; }
uint32_t CharacterCompRenderId(EntityComponent* component) { (void)component; return 0; }
ape::Vec3 CharacterCompAimPosition(EntityComponent* component) { (void)component; return { 0.0f, 0.0f, 0.0f, 0.0f }; }
float CharacterCompTurnRate(EntityComponent* component) { (void)component; return 0.0f; }
float RenderObjectPackedFade(ape::world::ApeObj* object) { (void)object; return 0.0f; }
void RenderObjectSetPackedFade(ape::world::ApeObj* object, float value) { (void)object; (void)value; }

void Component11Refresh(EntityComponent* component) { (void)component; }
void Component10SetAlternate(EntityComponent* component, bool alternate) { (void)component; (void)alternate; }
uint32_t Component15ObjectCount(EntityComponent* component) { (void)component; return 0; }
ape::world::ApeObj* Component15Object(EntityComponent* component, uint32_t index) { (void)component; (void)index; return nullptr; }
bool Component03Busy(EntityComponent* component) { (void)component; return false; }

void ObjectSetTint(float amount, ape::world::ApeObj* object, const uint8_t color[4]) { (void)amount; (void)object; (void)color; }
ape::world::ApeObj* CreateGeomInstance(void* geomDef, bool flag) { (void)geomDef; (void)flag; return nullptr; }
void SetGeomMaterialColor(ape::world::ApeObj* object, const uint8_t color[4]) { (void)object; (void)color; }
void CharacterHudColor(const EntityDef* def, uint8_t out[4])
{
    (void)def;
    out[0] = out[1] = out[2] = out[3] = 0xFF;
}
float EaseTint(float t) { return t; }
void AnimControllerSetFlag(void* controller, uint16_t bit, bool on) { (void)controller; (void)bit; (void)on; }
void AnimControllerSetEventSink(void* controller, Entity* owner) { (void)controller; (void)owner; }

uint8_t PlayerProfileIndex(uint32_t player) { (void)player; return 0; }
bool ProfileAlternateCostume(uint8_t profile, uint32_t costume) { (void)profile; (void)costume; return false; }
uint32_t ProfileSelection(uint8_t profile, uint32_t offset) { (void)profile; (void)offset; return 0; }
bool LevelPlayerHasAbility(uint32_t player, int ability) { (void)player; (void)ability; return false; }
uint32_t PurchasedAbilities(uint32_t character, uint32_t* out, uint32_t max) { (void)character; (void)out; (void)max; return 0; }
bool WorldFlag(uint32_t name) { (void)name; return false; }
bool RenderIdOverrideActive() { return false; }

void EntityPlayDirectorScript(Entity* entity, uint32_t script) { (void)entity; (void)script; }
void EntityAttachSpawned(Entity* parent, SpawnParams& params, Entity* child) { (void)parent; (void)params; (void)child; }
void EntityDefBaseBounds(const EntityDef* def, ape::Aabb& out)
{
    (void)def;
    out.min = { 0.0f, 0.0f, 0.0f, 0.0f };
    out.max = { 0.0f, 0.0f, 0.0f, 0.0f };
}

} // namespace entity
} // namespace sly
