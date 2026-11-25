// openTIT - Stubs for GameEntityHooks.h.
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

void EntityShutdown(Entity* entity) { (void)entity; }
void DetachAnimScript(void* script) { (void)script; }
void OrphanAndReleaseAnimScript(void* script) { (void)script; }
void SetAnimScriptPaused(void* script, bool paused) { (void)script; (void)paused; }
ape::world::ApeObj* ComponentRenderObject(EntityComponent* component) { (void)component; return nullptr; }
bool MinigameFlag30() { return false; }
GameCamera* MainGameCamera() { return nullptr; }
uint32_t CharacterDefWord3D4(const EntityDef* def) { (void)def; return 0; }
uint32_t AnimDriverState(EntityComponent* driver) { (void)driver; return 0; }
void AnimDriverStop(EntityComponent* driver, bool flag) { (void)driver; (void)flag; }
void SetComponentWord1C(EntityComponent* component, uint32_t value) { (void)component; (void)value; }
int EntityGetState(const Entity* entity, int layer) { (void)entity; (void)layer; return 0; }
uint8_t* EntityStateData(Entity* entity, int layer) { (void)entity; (void)layer; return nullptr; }

audio::VoiceId PlayEntitySound(audio::SoundName* sound, const ape::Vec3* position, const void* attachment)
{
    (void)sound; (void)position; (void)attachment;
    return 0;
}
void ReleaseVoiceLoop(audio::VoiceId voice) { (void)voice; }
bool GetVoiceParams(audio::VoiceId voice, audio::SoundParams& out) { (void)voice; (void)out; return false; }

void DequeueEntityTransformCallback(Entity* entity) { (void)entity; }
void HavokFree(void* data, uint32_t bytes) { (void)data; (void)bytes; }

} // namespace entity
} // namespace sly
