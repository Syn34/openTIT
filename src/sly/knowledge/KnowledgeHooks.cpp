// openTIT - Knowledge hooks: TODO stubs (see KnowledgeHooks.h).
#include "sly/knowledge/KnowledgeHooks.h"

namespace sly {
namespace knowledge {

float FrameTime() { return 1.0f / 30.0f; }
ape::ApeLevelDef* CurrentLevelDef() { return nullptr; }
void SendGameEvent(Entity*, Entity*, uint32_t, uint32_t, bool) {}
void BroadcastGameEvent(Entity*, uint32_t, uint32_t, uint32_t) {}
void EntitySetDirectorState(Entity*, uint32_t) {}
void CharacterPlayAnim(EntityComponent*, const char*) {}
void EntityPlayAnim(Entity*, const char*) {}
void PlayRuleSound(Entity*, audio::SoundName*, uint32_t, uint32_t, bool) {}
void* EntityPlayDirScript(Entity*, uint32_t) { return nullptr; }
void EntityAttachDirScript(Entity*, uint32_t) {}
void EntityStopDirScript(Entity*) {}
bool EntitySurfaceType(Entity*, uint32_t*) { return false; }
void ScriptSetVar(void*, uint32_t, uint32_t) {}
void AIHighlight(Entity*, bool) {}
void AISetAttackInvincible(Entity*, bool) {}
void AISetMaxRotationRate(Entity*, float) {}
void CharacterSetAdditiveAlpha(Entity*, float) {}
void EntitySetMotionTuning(Entity*, uint32_t, uint32_t, uint32_t, uint32_t) {}
void PadRumble(int, float, float) {}
void CharacterPlayFootstep(Entity*, bool, uint32_t) {}
void CharacterFootIK(Entity*, uint32_t, bool) {}
void CharacterSetProjectileParams(Entity*, float, bool, bool) {}
float CharacterAnimRate(EntityComponent*, float speed) { return speed; }
void CharacterSetAnimSpeed(EntityComponent*, float) {}
bool SoundAlertSuppressed(Entity*) { return false; }
void SkinRequestFire(EntityComponent*) {}
bool EntityHasAttachPoint(Entity*, uint32_t) { return true; }
// TODO(0x0001B8C0): the AI manager's attack sighting list lives in sly/levels/ai.
void AIAddMeleeAttack(Entity*, Entity*, uint32_t, const MeleeAttackParams&) {}

} // namespace knowledge
} // namespace sly
