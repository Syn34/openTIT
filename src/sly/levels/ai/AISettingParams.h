// openTIT - Parameter descriptors of the AI setting classes.
//
// The critter, buddy, enemy and sentry setting classes (AISettingClasses.h)
// register these descriptors; they are globals built by the static
// initialiser 0x00067738 (tagged in AITuning.cpp). `AIPropParam` maps the
// AIProp names used by the AI code to them.
#pragma once

#include "ape/setting/ApeSetting.h"
#include "sly/levels/ai/AIHooks.h"

namespace sly {
namespace levels {
namespace ai {

// Descriptor of an AI setting property, or null for properties registered by
// other modules (proxy points, jump links).
const ape::setting::SettingParam* AIPropParam(AIProp prop);

// Typed accessors of single parameters of a setting record (one small
// out-of-line function per parameter on the console).
namespace critter_setting {
uint32_t SpawnAnimClip(const ape::setting::Setting& setting);
bool SpawnPosIsDst(const ape::setting::Setting& setting);
bool UseFlightAnims(const ape::setting::Setting& setting);
float POIDelayMin(const ape::setting::Setting& setting);
float POIDelayMax(const ape::setting::Setting& setting);
} // namespace critter_setting

namespace enemy_setting {
float OptimalTargetRangeMax(const ape::setting::Setting& setting);
bool Cover(const ape::setting::Setting& setting);
bool Heavy(const ape::setting::Setting& setting);
float LoseTargetTime(const ape::setting::Setting& setting);
float FarAttackRange(const ape::setting::Setting& setting);
float RangedAttackChaseFrequency(const ape::setting::Setting& setting);
bool AttackRequiresLOS(const ape::setting::Setting& setting);
float CounterAttackRange(const ape::setting::Setting& setting);
float OptimalTargetRangeMin(const ape::setting::Setting& setting);
uint32_t CombatAttackFarAnim(const ape::setting::Setting& setting);
uint32_t CombatAttackRangedAnim(const ape::setting::Setting& setting);
bool Patrol(const ape::setting::Setting& setting);
bool AudioAlertInvestigate(const ape::setting::Setting& setting);
float Scale(const ape::setting::Setting& setting);
float PatrolIdleMinTime(const ape::setting::Setting& setting);
float PatrolIdleMaxTime(const ape::setting::Setting& setting);
uint32_t CombatOtherAnim(const ape::setting::Setting& setting);
uint32_t CombatAttackCloseAnim(const ape::setting::Setting& setting);
uint32_t CombatAttackMidAnim(const ape::setting::Setting& setting);
float CloseAttackRange(const ape::setting::Setting& setting);
float CloseAttackAngle(const ape::setting::Setting& setting);
float CloseAttackHeightMin(const ape::setting::Setting& setting);
float CloseAttackHeightMax(const ape::setting::Setting& setting);
float MidAttackRange(const ape::setting::Setting& setting);
float MidAttackAngle(const ape::setting::Setting& setting);
float MidAttackHeightMin(const ape::setting::Setting& setting);
float MidAttackHeightMax(const ape::setting::Setting& setting);
float FarAttackAngle(const ape::setting::Setting& setting);
float FarAttackHeightMin(const ape::setting::Setting& setting);
float FarAttackHeightMax(const ape::setting::Setting& setting);
float RangedAttackRange(const ape::setting::Setting& setting);
float RangedAttackAngle(const ape::setting::Setting& setting);
float RangedAttackHeightMin(const ape::setting::Setting& setting);
float RangedAttackHeightMax(const ape::setting::Setting& setting);
float TurretAttackMaxTime(const ape::setting::Setting& setting);
bool AttackControllerUseTimer(const ape::setting::Setting& setting);
uint32_t CombatMovementAnim(const ape::setting::Setting& setting);
float TurretAttackMinTime(const ape::setting::Setting& setting);
uint32_t IdleAnimClip(const ape::setting::Setting& setting);
} // namespace enemy_setting


} // namespace ai
} // namespace levels
} // namespace sly
