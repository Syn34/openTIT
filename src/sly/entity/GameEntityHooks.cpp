// openTIT - Stubs for GameEntityHooks.h.
#include "sly/entity/GameEntityHooks.h"

#include "sly/components/DeferredReleases.h"

#include "sly/game/SlyGame.h"
#include "sly/objects/Trigger.h"
#include "sly/gameplay/Minigame.h"

namespace sly {
namespace entity {

void EntityShutdown(Entity* entity) { (void)entity; }
void DetachAnimScript(void* script) { (void)script; }
void OrphanAndReleaseAnimScript(void* script) { (void)script; }
void SetAnimScriptPaused(void* script, bool paused) { (void)script; (void)paused; }
ape::world::ApeObj* ComponentRenderObject(EntityComponent* component) { (void)component; return nullptr; }
bool MinigameFlag30()
{
    const gameplay::Minigame* minigame = g_game ? g_game->minigame : nullptr;
    return minigame && (minigame->runFlags & gameplay::Minigame::kRun_Started);
}
GameCamera* MainGameCamera() { return nullptr; }
uint32_t CharacterDefWord3D4(const EntityDef* def) { (void)def; return 0; }
uint32_t AnimDriverState(EntityComponent* driver) { (void)driver; return 0; }
void AnimDriverStop(EntityComponent* driver, bool flag) { (void)driver; (void)flag; }
void SetComponentWord1C(EntityComponent* component, uint32_t value) { (void)component; (void)value; }
// The attached entities are trigger volumes (TriggerBase slot 76 / the
// occupant list).
void TriggerSetEnabled(Entity* trigger, bool enabled)
{
    static_cast<objects::TriggerBase*>(trigger)->SetEnabled(enabled);
}
int TriggerOccupantCount(const Entity* trigger)
{
    return static_cast<const objects::TriggerBase*>(trigger)->OccupantCount();
}
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

namespace sly {
namespace entity {

// TODO(0x00456918): the suspension selection job; nothing changes state.
void EntityJobSelectSuspended(const EntitySuspendJobParams&, std::vector<uint16_t>& suspend,
                              std::vector<uint16_t>& resume)
{
    suspend.clear();
    resume.clear();
}
// TODO(0x00456A60): the deletion selection job; nothing is destroyed.
void EntityJobSelectDead(std::vector<uint16_t>& dead) { dead.clear(); }
// TODO(0x00456BA8 / 0x00456CF0): the update selection jobs. NOTE: every live
// entity is selected until the job's filter is known.
void EntityJobSelectForUpdate(std::vector<Entity*>& out)
{
    for (uint32_t i = 0; i < EntityCount(kList_All); ++i)
        if (Entity* entity = EntityAt(kList_All, i); entity && entity->IsAlive())
            out.push_back(entity);
}
void EntityJobSelectForPostUpdate(std::vector<Entity*>& out) { EntityJobSelectForUpdate(out); }
// TODO(0x0020F400)
void FlushTriggerReports() {}
void EntityPostUpdateDone() { components::FlushDeferredReleases(); }
// TODO(0x0028BDB0): the targeting render callback (0x0028CF58 on the render
// callback list 0x0119582C) and two linear allocators (0x8A0 / 0x1600 bytes,
// 0x00F64A50 / 0x00F64A54).
void EntityModuleLevelStartHooks() { components::ResetDeferredReleases(); }

// TODO(0x0028D7B0)
void InstanceRenderPassDone(void* renderContext) { (void)renderContext; }
// TODO(0x0028E320)
void ListDPassBegin() {}

// TODO(0x004A6DF0)
void QueueObjectRenderRefresh(ape::world::ApeObj* object, bool recurse) { (void)object; (void)recurse; }

} // namespace entity
} // namespace sly
