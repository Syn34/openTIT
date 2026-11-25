// openTIT - Tunable settings blocks (see Character.h).
#include "sly/entity/Character.h"

#include "sly/objects/CharacterMoveTuning.h"

namespace sly {
namespace entity {

// @func 0x00733D80 DamageSettings::~DamageSettings
// @func 0x00738154 DamageSettings::~DamageSettings (deleting)
DamageSettings::~DamageSettings() = default;

// Entity.Character.Damage.* tuning.
// @func 0x00738040 DamageSettings::LoadDefaults
void DamageSettings::LoadDefaults()
{
    using namespace objects;
    impactScalarDyn = g_characterDamageImpactScalarDyn;
    impactScalarKey = g_characterDamageImpactScalarKey;
    melee = g_characterDamageMelee;
    blast = g_characterDamageBlast;
    environment = g_characterDamageEnvironment;
    environmentFire = g_characterDamageEnvironmentFire;
    environmentElectric = g_characterDamageEnvironmentElectric;
    environmentPoisonDuration = g_characterDamageEnvironmentPoisonDuration;
    projectile = g_characterDamageProjectile;
    projectileReflect = g_characterDamageProjectileReflect;
    projectileRandomReflect = g_characterDamageProjectileRandomReflect;
    projectileAngle = g_characterDamageProjectileAngle;
    projectileVelScalar = g_characterDamageProjectileVelScalar;
}

// @func 0x00738114 DamageSettings::Reload
bool DamageSettings::Reload()
{
    LoadDefaults();
    return true;
}

} // namespace entity
} // namespace sly
