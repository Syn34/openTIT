// openTIT - Tuning variables and lookup tables of the entity module.
#include "sly/entity/EntityTuning.h"

namespace sly {
namespace entity {

// @func 0x000F86DC StaticInit_EntityModule
// @func 0x000F81E4 StaticDestroy_EntityModule
// Compiler-generated initialiser/finaliser of the entity module's globals. On
// the console the same translation unit also holds the definition map
// (0x01013B04, EntityDefs.cpp), the component template map (0x01013B1C), the
// four entity lists (0x01013B34.., Entity.cpp) and the deferred event queue
// (0x01013B84), plus four zeroed lock words (0x01021D80..0x01021F00) and three zeroed
// words at 0x01032198 whose users are not known.

ape::TuningFloat g_entityBlendTimeDefault("Entity.BlendTimeDefault", 0.25f);
ape::TuningFloat g_entityCullDistDefault("Entity.CullDistDefault", 70.0f);
ape::TuningBool g_debugDisplayEntityHierarchy("Debug.Display.EntityHierarchy", false);
ape::TuningBool g_audioDisableEntitySounds("Audio.Optimization.DisableEntitySounds", false);
ape::TuningBool g_aiSetUpdateEnabledTest("AI.SetUpdateEnabledTest", true);
ape::TuningInt g_entityDamageableQueue("Entity.Damageable.Queue", 1);
ape::TuningFloat g_entityProjectileOrientDampening("Entity.Projectile.OrientDampening", 10.0f);

namespace {

// Three small key tables (0x0103214C, 0x01032164, 0x0103217C), each created
// with room for 16 entries and its own comparator on the console.
ape::ApeTSortedMap<void*> s_entityTableA(16);
ape::ApeTSortedMap<void*> s_entityTableB(16);
ape::ApeTSortedMap<EntityKeyPair*> s_entityKeyPairs(16);

} // namespace

// @func 0x000F816C LookupEntityKeyPair
void LookupEntityKeyPair(uint32_t key, uint32_t& first, uint32_t& second)
{
    if (key == 0)
        return;
    if (EntityKeyPair* pair = *s_entityKeyPairs.Find(key)) {
        first = pair->first;
        second = pair->second;
    }
}

// @func 0x000F815C EntityModuleTag
uint32_t EntityModuleTag()
{
    return 0x99B7AAF2u;
}

} // namespace entity
} // namespace sly
