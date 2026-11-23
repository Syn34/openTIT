// openTIT - RC bomb remote and item collect tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/RemoteItemTuning.h"

namespace sly {

// @func 0x002096F4 StaticInit_RemoteItemTuning
// @func 0x00209240 StaticDestroy_RemoteItemTuning
// Compiler-generated initialiser/destructor pair for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

ape::TuningFloat g_itemTargetingScoreBias("Entity.Item.TargetingScoreBias", 7.5f);
ape::TuningFloat g_itemTargetingScoreBiasThrow("Entity.Item.TargetingScoreBiasThrow", 15.0f);
ape::TuningBool g_itemCarryStatic("Entity.Item.CarryStatic", true);
ape::TuningVec2 g_itemCollectMonetaryEndHudPos("Entity.ItemCollect.MonetaryEndHudPos", 0.89f, 0.16f);
ape::TuningVec2 g_itemCollectMaskEndHudPos("Entity.ItemCollect.MaskEndHudPos", 0.89f, 0.16f);
ape::TuningVec2 g_itemCollectHealthEndHudPos("Entity.ItemCollect.HealthEndHudPos", 0.157f, 0.146f);
ape::TuningFloat g_itemCollectMinApproachScale("Entity.ItemCollect.MinApproachScale", 0.6f);
ape::TuningFloat g_itemCollectHudCollectLifetimeMax("Entity.ItemCollect.HudCollectLifetimeMax", 0.65f);
ape::TuningFloat g_itemCollectHudCollectMoveYOffset("Entity.ItemCollect.HudCollectMoveYOffset", 0.25f);
ape::TuningFloat g_itemCollectHudCollectStartScale("Entity.ItemCollect.HudCollectStartScale", 0.13f);
ape::TuningFloat g_itemCollectHudCollectEndScale("Entity.ItemCollect.HudCollectEndScale", 0.08f);
ape::TuningFloat g_itemCollectHudCollectSplinePt1Percent("Entity.ItemCollect.HudCollectSplinePt1Percent", 0.45f);
ape::TuningFloat g_itemCollectHudCollectSplinePt2Percent("Entity.ItemCollect.HudCollectSplinePt2Percent", 0.75f);
ape::TuningFloat g_itemCollectRBSettleRaycastDist("Entity.ItemCollect.RBSettleRaycastDist", 1.0f);
ape::TuningFloat g_itemCollectAutoCollectInitialCollectDist("Entity.ItemCollect.AutoCollect.InitialCollectDist", 6.0f);
ape::TuningBool g_itemCollectAutoCollectForceOn("Entity.ItemCollect.AutoCollect.ForceOn", true);
ape::TuningFloat g_itemCollectAutoCollectSafetyCollectDist("Entity.ItemCollect.AutoCollect.SafetyCollectDist", 1.5f);
ape::TuningFloat g_itemCollectTreasureCollectScale("Entity.ItemCollect.Treasure.CollectScale", 0.5f);
ape::TuningBool g_itemCollectTreasureAutoCollectEnabled("Entity.ItemCollect.Treasure.AutoCollectEnabled", false);
ape::TuningVec3 g_itemCollectTreasureApproachEndScale("Entity.ItemCollect.Treasure.ApproachEndScale", 0.4f, 0.4f, 0.4f);
ape::TuningFloat g_bRemoteDamageImpactDmg("Entity.BRemote.Damage.ImpactDmg", 0.0f);
ape::TuningFloat g_bRemoteDamageImpactImp("Entity.BRemote.Damage.ImpactImp", 1.0f);
ape::TuningFloat g_bRemoteDamageImpactVelMin("Entity.BRemote.Damage.ImpactVelMin", 0.5f);
ape::TuningFloat g_bRemoteDamageImpactDmgMax("Entity.BRemote.Damage.ImpactDmgMax", 1e+02f);
ape::TuningFloat g_bRemoteDamageMeleeDmg("Entity.BRemote.Damage.MeleeDmg", 0.0f);
ape::TuningFloat g_bRemoteDamageMeleeImp("Entity.BRemote.Damage.MeleeImp", 1.0f);
ape::TuningFloat g_bRemoteDamageBlastDmg("Entity.BRemote.Damage.BlastDmg", 0.0f);
ape::TuningFloat g_bRemoteDamageBlastImp("Entity.BRemote.Damage.BlastImp", 0.01f);
ape::TuningFloat g_bRemoteDamageEnvDmg("Entity.BRemote.Damage.EnvDmg", 0.0f);
ape::TuningFloat g_bRemoteDamageEnvImp("Entity.BRemote.Damage.EnvImp", 1.0f);
ape::TuningFloat g_bRemoteDamageProjDmg("Entity.BRemote.Damage.ProjDmg", 0.0f);
// Indexed by bomb type (explosive, sleep, electric, confusion, shrink).
ape::TuningFloat g_bRemoteBombDamage[5] = {
    { "Entity.BRemote.Bomb.Damage.Explosive", 5e+01f },
    { "Entity.BRemote.Bomb.Damage.Sleep", 1.0f },
    { "Entity.BRemote.Bomb.Damage.Electric", 5e+01f },
    { "Entity.BRemote.Bomb.Damage.Confusion", 1.0f },
    { "Entity.BRemote.Bomb.Damage.Shrink", 1.0f },
};
ape::TuningFloat g_bRemoteBombImpulse("Entity.BRemote.Bomb.Impulse", 15.0f);
ape::TuningVec2 g_bRemoteBombRadius("Entity.BRemote.Bomb.Radius", 2.5f, 2.5f);
ape::TuningFloat g_bRemoteBombSuperScalar("Entity.BRemote.Bomb.SuperScalar", 2.0f);
ape::TuningVec3 g_bRemoteCenterOfMass("Entity.BRemote.CenterOfMass", 0.0f, -0.5f, 0.0f);
ape::TuningFloat g_bRemoteTriggerTimeMax("Entity.BRemote.TriggerTimeMax", 0.1f);
ape::TuningFloat g_bRemoteImpactSFXSpeedMin("Entity.BRemote.ImpactSFXSpeedMin", 1.5f);
ape::TuningFloat g_bRemoteStickySlopeMin("Entity.BRemote.StickySlopeMin", 6e+01f);
ape::TuningFloat g_bRemoteKickSpeed("Entity.BRemote.KickSpeed", 2e+01f);
ape::TuningFloat g_bRemoteKickAngle("Entity.BRemote.KickAngle", 15.0f);
ape::TuningFloat g_bRemoteSeekRadiusMax("Entity.BRemote.SeekRadiusMax", 15.0f);
ape::TuningVec2 g_bRemoteSeekAcceleration("Entity.BRemote.SeekAcceleration", 1e+01f, 9e+01f);
ape::TuningVec3 g_bRemotePodOffset("Entity.BRemote.PodOffset", 1.5f, 2.25f, 0.05f);
ape::TuningVec3 g_bRemoteOscFreq("Entity.BRemote.OscFreq", 0.5f, 0.3f, 0.0f);
ape::TuningVec3 g_bRemoteOscMag("Entity.BRemote.OscMag", 0.1f, 0.25f, 0.0f);
ape::TuningVec3 g_bRemotePodSpringPos("Entity.BRemote.PodSpringPos", 6e+01f, 0.1f, 2.4e+02f);
ape::TuningVec3 g_bRemotePodSpringRot("Entity.BRemote.PodSpringRot", 6e+01f, 0.1f, 4.5e+02f);
ape::TuningVec2 g_bRemotePodAttackOffset("Entity.BRemote.PodAttackOffset", 5.0f, 1.5f);
ape::TuningFloat g_bRemoteFireAngleMax("Entity.BRemote.FireAngleMax", 15.0f);
ape::TuningFloat g_bRemoteReloadTime("Entity.BRemote.ReloadTime", 1.25f);
ape::TuningVec3 g_bRemoteRocketRate("Entity.BRemote.RocketRate", 0.4f, 0.8f, 6e+01f);
ape::TuningVec2 g_bRemoteRange("Entity.BRemote.Range", 15.5f, 7.0f);

} // namespace sly
