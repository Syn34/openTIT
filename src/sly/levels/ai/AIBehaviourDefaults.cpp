// openTIT - One-line virtual overrides and destructors of the AI behaviours
// (see AIBehaviours.h). The behaviours' destructors only run the controller
// base destructor.
#include "sly/levels/ai/AIBehaviours.h"

namespace sly {
namespace levels {
namespace ai {

// ---------------------------------------------------------------------------
// CritterAI (vtable 0x00B0E028)

// @func 0x00738998 CritterAI::~CritterAI
// @func 0x007389D0 CritterAI::~CritterAI (deleting)
CritterAI::~CritterAI() = default;

// @func 0x00738A98 CritterAI::AgentRadius
float CritterAI::AgentRadius()
{
    return PropBool(settings, AIProp::Critter_UseAvoidance) ? PropFloat(settings, AIProp::Critter_AvoidanceRadius)
                                                            : 0.1f;
}

// @func 0x00738AF0 CritterAI::Slot16
float CritterAI::Slot16() { return 0.1f; }

// @func 0x00738B00 CritterAI::Slot17
int CritterAI::Slot17() { return 1; }

// @func 0x00738B08 CritterAI::Slot18
uint32_t CritterAI::Slot18() { return 0xFFFFFFFFu; }

// Engaged while charging or attacking the player.
// NOTE: states 10/11 are not named in CritterAI::State.
// @func 0x00738A2C CritterAI::IsEngaged
bool CritterAI::IsEngaged() { return state == 0x0B || state == 0x0A; }

// @func 0x00738B18 CritterAI::WritesEntityTransform
bool CritterAI::WritesEntityTransform() { return true; }

// @func 0x00738A54 CritterAI::HasNoSpawnClip
bool CritterAI::HasNoSpawnClip() { return PropHash(settings, AIProp::Critter_SpawnAnimClip) == 0; }

// @func 0x00738B20 CritterAI::RootMotionScale
float CritterAI::RootMotionScale() { return PropFloat(settings, AIProp::Critter_SpeedScale); }

// ---------------------------------------------------------------------------
// PlayerAI (vtable 0x00B0E1B8)

// @func 0x00738C0C PlayerAI::~PlayerAI
// @func 0x00738C44 PlayerAI::~PlayerAI (deleting)
PlayerAI::~PlayerAI() = default;

// @func 0x00738CA0 PlayerAI::TickEntity
void PlayerAI::TickEntity(float) {}

// NOTE: follow modes 1 and 3; their meaning is not decoded.
// @func 0x00738CE0 PlayerAI::Slot24
bool PlayerAI::Slot24() { return mode == 3 || mode == 1; }

// @func 0x00738CA4 PlayerAI::IsAlly
bool PlayerAI::IsAlly() { return true; }

// NOTE: false in states 1, 4, 12 and 13.
// @func 0x00738CAC PlayerAI::Slot36
bool PlayerAI::Slot36() { return state != 0x0D && state != 0x0C && state != 1 && state != 4; }

// @func 0x00738D08 PlayerAI::NavFilter
int PlayerAI::NavFilter() { return 8; }

// ---------------------------------------------------------------------------
// PatternBossAI (vtable 0x00B0E2F0)

// @func 0x00738D10 PatternBossAI::~PatternBossAI
// @func 0x00738D48 PatternBossAI::~PatternBossAI (deleting)
PatternBossAI::~PatternBossAI() = default;

// @func 0x00738DA4 PatternBossAI::TickEntity
void PatternBossAI::TickEntity(float) {}

// @func 0x00738DA8 PatternBossAI::IsCharacterDead
bool PatternBossAI::IsCharacterDead() { return false; }

// @func 0x00738DF4 PatternBossAI::NavFilter
int PatternBossAI::NavFilter() { return 2; }

// ---------------------------------------------------------------------------
// RacerAI (vtable 0x00B0E428)

// @func 0x00738DFC RacerAI::TickEntity
void RacerAI::TickEntity(float) {}

// @func 0x00738E0C RacerAI::NavFilter
int RacerAI::NavFilter() { return 2; }

// Racers are moved by their vehicle.
// @func 0x00738E08 RacerAI::UpdateMovement
void RacerAI::UpdateMovement(float) {}

// @func 0x00738E00 RacerAI::SetRacePosition
void RacerAI::SetRacePosition(float position) { racePosition = position; }

// ---------------------------------------------------------------------------
// EnemyAI (vtable 0x00B0E5B8)

// @func 0x00738EF0 EnemyAI::TickEntity
void EnemyAI::TickEntity(float) {}

// @func 0x00738F00 EnemyAI::AgentRadius
float EnemyAI::AgentRadius() { return PropBool(settings, AIProp::Enemy_Heavy) ? 1.0f : 0.5f; }

// @func 0x00738F50 EnemyAI::Slot16
float EnemyAI::Slot16() { return PropBool(settings, AIProp::Enemy_Heavy) ? 3.0f : 1.0f; }

// @func 0x00738F98 EnemyAI::Slot17
int EnemyAI::Slot17() { return 2; }

// @func 0x00738FA0 EnemyAI::Slot18
uint32_t EnemyAI::Slot18() { return 10; }

// 120 degrees, in radians.
// @func 0x00738FB0 EnemyAI::TurnInPlaceAngle
float EnemyAI::TurnInPlaceAngle() { return 2.0943952f; }

// Not while engaged or for "NoTarget" enemies; in states 0x29..0x3A only
// while helpless.
// @func 0x00739024 EnemyAI::CanBeSneakedUpOn
bool EnemyAI::CanBeSneakedUpOn()
{
    if ((flags0 & kF0_Bit0) || PropBool(settings, AIProp::Enemy_NoTarget))
        return false;
    if ((uint32_t)state > 0x28 && (uint32_t)state < 0x3B && !IsHelpless())
        return false;
    return true;
}

// @func 0x007390CC EnemyAI::Slot27
bool EnemyAI::Slot27() { return state == 0x2B; }

// @func 0x007390E0 EnemyAI::Slot28
bool EnemyAI::Slot28() { return state == 0x2C; }

// @func 0x007390F4 EnemyAI::Slot29
void EnemyAI::Slot29() { enemyFlagsC |= kEC_Bit5; }

// @func 0x00738FC0 EnemyAI::IsListedActive
bool EnemyAI::IsListedActive() { return true; }

// @func 0x00738FEC EnemyAI::HasNoSpawnClip
bool EnemyAI::HasNoSpawnClip() { return PropHash(settings, AIProp::Enemy_SpawnAnimClip) == 0; }

// @func 0x00738FC8 EnemyAI::IsHeavy
bool EnemyAI::IsHeavy() { return PropBool(settings, AIProp::Enemy_Heavy); }

// @func 0x00739110 EnemyAI::Slot46
int EnemyAI::Slot46() { return field_1A8 > 0.0f; }

// ---------------------------------------------------------------------------
// SentryAI (vtable 0x00B0E750) and its ice cave variant (0x00B0EDA0)

// @func 0x00739214 SentryAI::~SentryAI
// @func 0x0073924C SentryAI::~SentryAI (deleting)
SentryAI::~SentryAI() = default;

// @func 0x007392B0 SentryAI::Slot16
float SentryAI::Slot16() { return 100.0f; }

// @func 0x007392C0 SentryAI::Slot17
int SentryAI::Slot17() { return 8; }

// @func 0x007392C8 SentryAI::Slot18
uint32_t SentryAI::Slot18() { return 0; }

// @func 0x007392D0 SentryAI::CanPitch
bool SentryAI::CanPitch() { return true; }

// @func 0x00739878 SentryAIVariant::~SentryAIVariant
// @func 0x007398B0 SentryAIVariant::~SentryAIVariant (deleting)
SentryAIVariant::~SentryAIVariant() = default;

// ---------------------------------------------------------------------------
// Bosses (vtables 0x00B0E888, 0x00B0E9C0, 0x00B0EAF8, 0x00B0EC30)

// @func 0x00739614 BismarqueAI::AllowsInteraction
bool BismarqueAI::AllowsInteraction() { return true; }

// @func 0x0073960C BismarqueAI::IsBoss
bool BismarqueAI::IsBoss() { return true; }

// Bismarque ignores the generic hit handling.
// @func 0x00739608 BismarqueAI::OnDamaged
void BismarqueAI::OnDamaged(float, uint32_t, const AIDamageInfo*) {}

// @func 0x0073961C BismarqueAI::NavFilter
int BismarqueAI::NavFilter() { return 4; }

// @func 0x00739624 ElJefeAI::~ElJefeAI
// @func 0x0073965C ElJefeAI::~ElJefeAI (deleting)
ElJefeAI::~ElJefeAI() = default;

// @func 0x007396B8 ElJefeAI::SetActive
void ElJefeAI::SetActive(bool) {}

// @func 0x007396BC ElJefeAI::TickEntity
void ElJefeAI::TickEntity(float) {}

// @func 0x007396C0 ElJefeAI::IsBoss
bool ElJefeAI::IsBoss() { return true; }

// @func 0x007396C8 MsDecibelAI::~MsDecibelAI
// @func 0x00739700 MsDecibelAI::~MsDecibelAI (deleting)
MsDecibelAI::~MsDecibelAI() = default;

// @func 0x0073975C MsDecibelAI::TickEntity
void MsDecibelAI::TickEntity(float) {}

// NOTE: engaged in fight states 4-6.
// @func 0x00739760 MsDecibelAI::IsEngaged
bool MsDecibelAI::IsEngaged() { return state == 5 || state == 4 || state == 6; }

// @func 0x00739790 MsDecibelAI::IsBoss
bool MsDecibelAI::IsBoss() { return true; }

// @func 0x00739798 ParadoxAI::~ParadoxAI
// @func 0x007397D0 ParadoxAI::~ParadoxAI (deleting)
ParadoxAI::~ParadoxAI() = default;

// @func 0x0073982C ParadoxAI::TickEntity
void ParadoxAI::TickEntity(float) {}

// @func 0x00739830 ParadoxAI::IsEngaged
bool ParadoxAI::IsEngaged() { return true; }

// @func 0x00739838 ParadoxAI::IsBoss
bool ParadoxAI::IsBoss() { return true; }

} // namespace ai
} // namespace levels
} // namespace sly
