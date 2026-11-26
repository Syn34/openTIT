// openTIT - Knowledge hooks: TODO stubs (see KnowledgeHooks.h).
#include "sly/knowledge/KnowledgeHooks.h"
#include "sly/game/GameMessages.h"
#include "sly/levels/ai/AIController.h"

namespace sly {
namespace knowledge {

float FrameTime() { return 1.0f / 30.0f; }
ape::ApeLevelDef* CurrentLevelDef() { return nullptr; }
void SendGameEvent(Entity* sender, Entity* receiver, uint32_t event, uint32_t data, bool immediate)
{
    PostGameMessage(sender, receiver, event, data, immediate);
}
void BroadcastGameEvent(Entity* sender, uint32_t group, uint32_t event, uint32_t data)
{
    PostToListeners(sender, group, event, data);
}
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
void AIAddMeleeAttack(Entity* attacker, Entity* target, uint32_t bone, const MeleeAttackParams& params)
{
    levels::ai::AIManager* manager = levels::ai::GetAIManager();
    if (!manager)
        return;
    levels::ai::AIAttackDesc desc;
    desc.damage = params.field_3C;
    desc.push = params.field_40;
    desc.radius = params.field_30;
    desc.offset = params.offset;
    desc.field_34 = params.field_34;
    desc.reaction = params.field_38;
    manager->AddAttack(attacker, target, bone, desc);
}

} // namespace knowledge
} // namespace sly
