// openTIT - Tuning variables and lookup tables of the entity module.
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/tuning/ApeTuning.h"

namespace sly {
namespace entity {

extern ape::TuningFloat g_entityBlendTimeDefault;          // "Entity.BlendTimeDefault"
extern ape::TuningFloat g_entityCullDistDefault;           // "Entity.CullDistDefault"
extern ape::TuningBool g_debugDisplayEntityHierarchy;      // "Debug.Display.EntityHierarchy"
extern ape::TuningBool g_audioDisableEntitySounds;         // "Audio.Optimization.DisableEntitySounds"
extern ape::TuningBool g_aiSetUpdateEnabledTest;           // "AI.SetUpdateEnabledTest"
extern ape::TuningInt g_entityDamageableQueue;             // "Entity.Damageable.Queue"
extern ape::TuningFloat g_entityProjectileOrientDampening; // "Entity.Projectile.OrientDampening"

// Pair of words registered under a key (third lookup table of the module,
// 0x0103217C). NOTE: what the pairs describe is not known yet.
struct EntityKeyPair {
    uint32_t first;
    uint32_t second;
};
// Copies the pair registered under `key`, if any (0x000F816C).
void LookupEntityKeyPair(uint32_t key, uint32_t& first, uint32_t& second);
// Hash the module hands to the asset loader (0x000F815C). NOTE: meaning unknown.
uint32_t EntityModuleTag();

} // namespace entity
} // namespace sly
