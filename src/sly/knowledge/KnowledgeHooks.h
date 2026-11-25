// openTIT - Calls of the knowledge callbacks into systems outside this
// module, as TODO stubs with safe defaults. Each names the console function,
// field or virtual slot it stands for.
#pragma once

#include <cstdint>

namespace ape { class ApeLevelDef; }
namespace sly {
namespace audio { struct SoundName; }
namespace entity { class Entity; class EntityComponent; }

namespace knowledge {

using entity::Entity;
using entity::EntityComponent;

// Frame time of the game clock (0x011DE098, clock 0x011DE094).
float FrameTime();

// Level definition of the running level (game world +0x1281C0).
ape::ApeLevelDef* CurrentLevelDef();

// Game events: 0x004705F8 (to one entity) and 0x00470660 (to the listeners
// of a group).
void SendGameEvent(Entity* sender, Entity* receiver, uint32_t event, uint32_t data, bool immediate);
void BroadcastGameEvent(Entity* sender, uint32_t group, uint32_t event, uint32_t data);

// Entity virtual slot 32 (+0x80) with (state, 0). NOTE: Entity.h calls the
// slot SetTimeScale; the knowledge result names it the director state.
void EntitySetDirectorState(Entity* entity, uint32_t state);
// Plays an animation by name on a character component (0x0029817C, blend
// 1.0 / 0.25) or on a plain entity (virtual slot 51, +0xCC).
void CharacterPlayAnim(EntityComponent* character, const char* anim);
void EntityPlayAnim(Entity* entity, const char* anim);

// Plays a sound for a rule: 2D (0x0044A374) without an entity, else on the
// entity (0x000DB348) with the attach type and part name; the request's
// buses are reset afterwards (0x00440AC4 / 0x00440A40).
void PlayRuleSound(Entity* entity, audio::SoundName* sound, uint32_t attachType, uint32_t part,
                   bool useSurface);

// Director scripts: 0x000DCD20 (play, returns the script instance), 0x00129CB8
// (attach to the character), 0x00129C18 (stop the character's script).
void* EntityPlayDirScript(Entity* entity, uint32_t script);
void EntityAttachDirScript(Entity* entity, uint32_t script);
void EntityStopDirScript(Entity* entity);
// Surface the character stands on: component of slot 16 (+0x40) whose
// +0x1A0 bits 28-30 are 1 gives +0x198.
bool EntitySurfaceType(Entity* entity, uint32_t* surface);
// Sets a script variable's value (ApeAnimScriptInstance::FindVar 0x004BB4B4,
// value at +0x14).
void ScriptSetVar(void* script, uint32_t var, uint32_t value);

// AI highlight of a character (0x0001CC30).
void AIHighlight(Entity* entity, bool on);
// Field +0x131 bit 4 of the entity's listener (the AI controller).
void AISetAttackInvincible(Entity* entity, bool on);
// +0x4C of the controller (+0x18) of the AI component (slot 17, +0x44).
void AISetMaxRotationRate(Entity* entity, float rate);
// +0x17C of the character component (slot 15).
void CharacterSetAdditiveAlpha(Entity* entity, float alpha);
// Entity +0xE0/+0xE4/+0xE8/+0xF0 and flag 0x800 of +0xB0.
void EntitySetMotionTuning(Entity* entity, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
// Rumble on the controller record 0x010B1A90 + index * 0x2EC0 (0x00464488).
void PadRumble(int controller, float strength, float duration);
// Footstep effect (0x0012BD94).
void CharacterPlayFootstep(Entity* entity, bool rightFoot, uint32_t attachType);
// Foot IK object (+0x24 of the character component) (0x0035327C).
void CharacterFootIK(Entity* entity, uint32_t foot, bool enabled);
// Character +0x210 / +0x214 / +0x215.
void CharacterSetProjectileParams(Entity* entity, float scalar, bool reflect, bool reflectRandom);
// Character animation speed: 0x00276B40 (rate of the current clip) and
// 0x00276E4C (set the speed).
float CharacterAnimRate(EntityComponent* character, float speed);
void CharacterSetAnimSpeed(EntityComponent* character, float speed);
// The noise is ignored while the character hides (+0x29C bit 7 set, bit 6
// clear, slot 8 states 0, 0, 3).
bool SoundAlertSuppressed(Entity* entity);
// Skin component +0x4F4 bit 30: fire the weapon.
void SkinRequestFire(EntityComponent* skin);
// Locator (object def +0xD0 +0x6C, 0x005848CC) or bone (0x000D14DC) named
// `name` exists on the entity.
bool EntityHasAttachPoint(Entity* entity, uint32_t name);

} // namespace knowledge
} // namespace sly
