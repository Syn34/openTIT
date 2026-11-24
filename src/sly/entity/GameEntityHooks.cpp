// openTIT - Stubs for GameEntityHooks.h.
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

void EntityShutdown(Entity* entity) { (void)entity; }
void DetachAnimScript(void* script) { (void)script; }
void OrphanAndReleaseAnimScript(void* script) { (void)script; }
int EntityGetState(const Entity* entity, int layer) { (void)entity; (void)layer; return 0; }

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
