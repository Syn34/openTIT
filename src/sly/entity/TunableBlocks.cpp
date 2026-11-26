// openTIT - Tunable settings blocks (see Character.h).
#include "sly/entity/Character.h"
#include "sly/entity/TunableBlocks.h"

#include "sly/objects/CharacterMoveTuning.h"
#include "sly/tuning/CameraTuning.h"

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

// -- camera settings block ---------------------------------------------------------

// @func 0x00733D90 CameraSettingsBlock::~CameraSettingsBlock
// @func 0x0074EAF4 CameraSettingsBlock::~CameraSettingsBlock (deleting)
CameraSettingsBlock::~CameraSettingsBlock() = default;

// @func 0x0074EAB4 CameraSettingsBlock::Reload
bool CameraSettingsBlock::Reload()
{
    LoadDefaults();
    return true;
}

// -- movement settings ---------------------------------------------------------------

// @func 0x00733D70 MoveSettingsBlock::~MoveSettingsBlock
// @func 0x00737FFC MoveSettingsBlock::~MoveSettingsBlock (deleting)
MoveSettingsBlock::~MoveSettingsBlock() = default;

// @func 0x007379FC MoveSettingsBlock::LoadDefaults
void MoveSettingsBlock::LoadDefaults()
{
    groundAlignToSurface = objects::g_characterMoveGroundAlignToSurface;
    groundTurnInPlace = objects::g_characterMoveGroundTurnInPlace;
    groundIdleTime = objects::g_characterMoveGroundIdleTime;
    groundJoyMinTurn = objects::g_characterMoveGroundJoyMinTurn;
    groundJoyMinWalk = objects::g_characterMoveGroundJoyMinWalk;
    groundJoyMinRun = objects::g_characterMoveGroundJoyMinRun;
    groundJoyMinSprint = objects::g_characterMoveGroundJoyMinSprint;
    groundSpeedmaxDefault = objects::g_characterMoveGroundSpeedmaxDefault;
    groundSpeedmaxSprint = objects::g_characterMoveGroundSpeedmaxSprint;
    groundAnglemaxDefault = objects::g_characterMoveGroundAnglemaxDefault;
    groundTurnMaxDefault = objects::g_characterMoveGroundTurnMaxDefault;
    groundTurnMaxRun = objects::g_characterMoveGroundTurnMaxRun;
    groundTurnMaxSprint = objects::g_characterMoveGroundTurnMaxSprint;
    groundTurnAccelTime = objects::g_characterMoveGroundTurnAccelTime;
    groundWalkAnimSpeed = objects::g_characterMoveGroundWalkAnimSpeed;
    groundRunAnimSpeed = objects::g_characterMoveGroundRunAnimSpeed;
    groundSprintAnimSpeed = objects::g_characterMoveGroundSprintAnimSpeed;
    groundTakedownOffset = { objects::g_characterMoveGroundTakedownOffset.Get()[0], objects::g_characterMoveGroundTakedownOffset.Get()[1], objects::g_characterMoveGroundTakedownOffset.Get()[2], 0.0f };
    groundStrafeAnalogDeg = objects::g_characterMoveGroundStrafeAnalogDeg;
    groundCombatSpeedmax = objects::g_characterMoveGroundCombatSpeedmax;
    groundCombatAnimSpeed = objects::g_characterMoveGroundCombatAnimSpeed;
    groundSneakSpeedmax = objects::g_characterMoveGroundSneakSpeedmax;
    groundSneakAnimSpeed = objects::g_characterMoveGroundSneakAnimSpeed;
    groundCrawlTurnInterp = objects::g_characterMoveGroundCrawlTurnInterp;
    groundCrawlSpeedmaxDefault = objects::g_characterMoveGroundCrawlSpeedmaxDefault;
    groundCrawlAnglemaxDefault = objects::g_characterMoveGroundCrawlAnglemaxDefault;
    groundCrawlTurnMaxDefault = objects::g_characterMoveGroundCrawlTurnMaxDefault;
    carryAttachBone = objects::g_characterCarryAttachBone.Get();
    carryAttachOffset = { objects::g_characterCarryAttachOffset.Get()[0], objects::g_characterCarryAttachOffset.Get()[1], objects::g_characterCarryAttachOffset.Get()[2], 0.0f };
    carryAttachRot = { objects::g_characterCarryAttachRot.Get()[0], objects::g_characterCarryAttachRot.Get()[1], objects::g_characterCarryAttachRot.Get()[2], 0.0f };
    carryThrowOffset = { objects::g_characterCarryThrowOffset.Get()[0], objects::g_characterCarryThrowOffset.Get()[1], objects::g_characterCarryThrowOffset.Get()[2], 0.0f };
    carryThrowDist = objects::g_characterCarryThrowDist;
    carryThrowVelocity = objects::g_characterCarryThrowVelocity;
    carryDamageVelocity = { objects::g_characterCarryDamageVelocity.Get()[0], objects::g_characterCarryDamageVelocity.Get()[1], objects::g_characterCarryDamageVelocity.Get()[2], 0.0f };
    groundAimSpeedmaxRun = objects::g_characterMoveGroundAimSpeedmaxRun;
    groundSkidTimeMax = objects::g_characterMoveGroundSkidTimeMax;
    groundSkidSpeedMin = objects::g_characterMoveGroundSkidSpeedMin;
    groundSkidFrictionScalar = objects::g_characterMoveGroundSkidFrictionScalar;
    groundLeanFactor = objects::g_characterMoveGroundLeanFactor;
    groundLeanAngleMax = objects::g_characterMoveGroundLeanAngleMax;
    jumpPowerMaxDefault = objects::g_characterMoveJumpPowerMaxDefault;
    doublejumpTimeMin = objects::g_characterMoveDoublejumpTimeMin;
    doublejumpTimeMax = objects::g_characterMoveDoublejumpTimeMax;
    doublejumpPowerMax = objects::g_characterMoveDoublejumpPowerMax;
    airJumpTimeMax = objects::g_characterMoveAirJumpTimeMax;
    airSpeedMaxDefault = objects::g_characterMoveAirSpeedMaxDefault;
    airSpeedMaxDoublejump = objects::g_characterMoveAirSpeedMaxDoublejump;
    airAccDefault = objects::g_characterMoveAirAccDefault;
    airAccDoublejump = objects::g_characterMoveAirAccDoublejump;
    airJumpStrafeTime = objects::g_characterMoveAirJumpStrafeTime;
    airAngleMaxDefault = objects::g_characterMoveAirAngleMaxDefault;
    airTurnMaxDefault = objects::g_characterMoveAirTurnMaxDefault;
    airVertInterp = objects::g_characterMoveAirVertInterp;
    climbJoyMin = objects::g_characterMoveClimbJoyMin;
    climbRailSpeedMaxWalk = objects::g_characterMoveClimbRailSpeedMaxWalk;
    climbRailSpeedMaxRun = objects::g_characterMoveClimbRailSpeedMaxRun;
    climbRailSpeedMaxSprint = objects::g_characterMoveClimbRailSpeedMaxSprint;
    climbRailAnimSpeedWalk = objects::g_characterMoveClimbRailAnimSpeedWalk;
    climbRailAnimSpeedRun = objects::g_characterMoveClimbRailAnimSpeedRun;
    climbRailAnimSpeedSprint = objects::g_characterMoveClimbRailAnimSpeedSprint;
    climbPoleOffset = { objects::g_characterMoveClimbPoleOffset.Get()[0], objects::g_characterMoveClimbPoleOffset.Get()[1], objects::g_characterMoveClimbPoleOffset.Get()[2], 0.0f };
    climbPoleSpeedMax = objects::g_characterMoveClimbPoleSpeedMax;
    climbPoleAnimSpeed = objects::g_characterMoveClimbPoleAnimSpeed;
    climbZiplineOffset = { objects::g_characterMoveClimbZiplineOffset.Get()[0], objects::g_characterMoveClimbZiplineOffset.Get()[1], objects::g_characterMoveClimbZiplineOffset.Get()[2], 0.0f };
    climbHookSwingOffset = { objects::g_characterMoveClimbHookSwingOffset.Get()[0], objects::g_characterMoveClimbHookSwingOffset.Get()[1], objects::g_characterMoveClimbHookSwingOffset.Get()[2], 0.0f };
    climbSidleSpeedMax = objects::g_characterMoveClimbSidleSpeedMax;
    climbSidleSpeedHiMin = objects::g_characterMoveClimbSidleSpeedHiMin;
    climbSidleAnimSpeedLo = objects::g_characterMoveClimbSidleAnimSpeedLo;
    climbSidleAnimSpeedHi = objects::g_characterMoveClimbSidleAnimSpeedHi;
    climbJumpPowerMaxRail = objects::g_characterMoveClimbJumpPowerMaxRail;
    climbJumpPowerMaxPoint = objects::g_characterMoveClimbJumpPowerMaxPoint;
    climbJumpPowerMaxPole = objects::g_characterMoveClimbJumpPowerMaxPole;
    climbJumpPowerMaxChain = objects::g_characterMoveClimbJumpPowerMaxChain;
    climbJumpPowerMaxRope = objects::g_characterMoveClimbJumpPowerMaxRope;
    climbJumpPowerMaxZipline = objects::g_characterMoveClimbJumpPowerMaxZipline;
    climbJumpPowerMaxSwing = objects::g_characterMoveClimbJumpPowerMaxSwing;
    climbJumpPowerMaxHang = objects::g_characterMoveClimbJumpPowerMaxHang;
    climbJumpPowerMaxGrind = objects::g_characterMoveClimbJumpPowerMaxGrind;
    climbJumpPowerMaxSpring = objects::g_characterMoveClimbJumpPowerMaxSpring;
    climbJumpPowerMaxSidle = objects::g_characterMoveClimbJumpPowerMaxSidle;
    climbJumpPowerMaxTrack = objects::g_characterMoveClimbJumpPowerMaxTrack;
    climbJumpPowerMaxVehicle = objects::g_characterMoveClimbJumpPowerMaxVehicle;
    climbJumpPowerMaxStealth = objects::g_characterMoveClimbJumpPowerMaxStealth;
    superSpirePowerMaxDefault = objects::g_characterSuperSpirePowerMaxDefault;
    aimDodgeCooldownTime = objects::g_characterAimDodgeCooldownTime;
}

// @func 0x00737FBC MoveSettingsBlock::Reload
bool MoveSettingsBlock::Reload()
{
    LoadDefaults();
    return true;
}

// -- shoulder camera settings ----------------------------------------------------------

// @func 0x00733DA0 ShoulderSettingsBlock::~ShoulderSettingsBlock
// @func 0x00738204 ShoulderSettingsBlock::~ShoulderSettingsBlock (deleting)
ShoulderSettingsBlock::~ShoulderSettingsBlock() = default;

// @func 0x00738198 ShoulderSettingsBlock::LoadDefaults
void ShoulderSettingsBlock::LoadDefaults() { settings.LoadDefaults(); }

// @func 0x007381C4 ShoulderSettingsBlock::Reload
bool ShoulderSettingsBlock::Reload()
{
    LoadDefaults();
    return true;
}

// -- disguise settings ------------------------------------------------------------------

// @func 0x00733DC0 DisguiseSettingsBlock::~DisguiseSettingsBlock
// @func 0x0073829C DisguiseSettingsBlock::~DisguiseSettingsBlock (deleting)
DisguiseSettingsBlock::~DisguiseSettingsBlock() = default;

// @func 0x00738248 DisguiseSettingsBlock::LoadDefaults
void DisguiseSettingsBlock::LoadDefaults() { disguiseExpirationRate = objects::g_characterDisguiseExpirationRate; }

// @func 0x0073825C DisguiseSettingsBlock::Reload
bool DisguiseSettingsBlock::Reload()
{
    LoadDefaults();
    return true;
}

// -- block 0x00B14120 -----------------------------------------------------------------

// @func 0x00733DB0 SettingsBlockB14120::~SettingsBlockB14120
// @func 0x0074EB78 SettingsBlockB14120::~SettingsBlockB14120 (deleting)
SettingsBlockB14120::~SettingsBlockB14120() = default;

// @func 0x00119D8C SettingsBlockB14120::LoadDefaults
void SettingsBlockB14120::LoadDefaults()
{
    stepTimeMax = g_cameraFollowStepTimeMax;
    fOV = g_cameraFollowFOV;
    offsetZ = g_cameraFollowOffsetZ;
    offsetY = g_cameraFollowOffsetY;
    nearClip = g_cameraFollowNearClip;
    colRadius = g_cameraFollowColRadius;
    colFractHS = g_cameraFollowColFractHS;
    colInterp = g_cameraFollowColInterp;
    colConeMin = g_cameraFollowColConeMin;
    colMomentMax = g_cameraFollowColMomentMax;
    colRetract = g_cameraFollowColRetract;
    spring = g_cameraFollowSpring;
    damping = g_cameraFollowDamping;
    springEnd = g_cameraFollowSpringEnd;
    targetXZ = g_cameraFollowTargetXZ;
    targetYUp = g_cameraFollowTargetYUp;
    targetYDown = g_cameraFollowTargetYDown;
    targetYMinSoft = g_cameraFollowTargetYMinSoft;
    targetYMaxSoft = g_cameraFollowTargetYMaxSoft;
    targetYMinHard = g_cameraFollowTargetYMinHard;
    targetYMaxHard = g_cameraFollowTargetYMaxHard;
    targetYMode = g_cameraFollowTargetYMode;
    yShift = g_cameraFollowYShift;
    lookAheadXZ = g_cameraFollowLookAheadXZ;
    lookAheadY = g_cameraFollowLookAheadY;
    rotateScalar = g_cameraFollowRotateScalar;
    rotateInterp = g_cameraFollowRotateInterp;
    gimbalRadius = g_cameraFollowGimbalRadius;
    gimbalMin = g_cameraFollowGimbalMin;
    gimbalMax = g_cameraFollowGimbalMax;
    gimbalShift = g_cameraFollowGimbalShift;
    gimbalOffsetY = g_cameraFollowGimbalOffsetY;
    gimbalOffsetZ = g_cameraFollowGimbalOffsetZ;
    gimbalScalar = g_cameraFollowGimbalScalar;
    gimbalInterpV = g_cameraFollowGimbalInterpV;
    gimbalInterpD = g_cameraFollowGimbalInterpD;
    gimbalInterpReturn = g_cameraFollowGimbalInterpReturn;
    gimbalReturn = g_cameraFollowGimbalReturn;
}

// @func 0x0074EB38 SettingsBlockB14120::Reload
bool SettingsBlockB14120::Reload()
{
    LoadDefaults();
    return true;
}

} // namespace entity
} // namespace sly
