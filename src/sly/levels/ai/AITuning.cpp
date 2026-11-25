// openTIT - Tuning variables of the level AI and boss module (see AITuning.h).
//
// Values were extracted from the disassembly of the static initialiser (Ghidra
// loses most float arguments).
#include "sly/levels/ai/AITuning.h"

namespace sly {
namespace levels {
namespace ai {

// @func 0x00067738 StaticInit_AITuning
// @func 0x00065424 StaticDestroy_AITuning
// The matching static destructor runs the destructors of the same globals in
// reverse order (tuning variables leave the registry, the setting parameters
// get their base vtable back, the pattern arrays are released).
// Compiler-generated initialiser for the globals of the module, in console
// order. Besides the tuning variables below it constructs:
//   * the AI setting class parameter descriptors (critter 0x00F94784..,
//     buddy 0x00F94AB4.., enemy 0x00F94B14.., sentry 0x00F952AC..), which are
//     the globals of AISettingParams.cpp;
//   * the pattern boss state pattern arrays 0x00F9BC78..0x00F9BCC0
//     (AIStatePatterns, AIComponent.cpp);
//   * a zero vector 0x00F9CCF8, the NaN "unset" float 0x00F94780 and an
//     empty 12-byte record 0x00F9C01C (0x007372DC; owner unknown);
//   * the two fixed frames at the end of this file.
// The colour defaults copied from the engine colour table (0x01164CA0..: white,
// transparent, black, red 0x01164CAC, green 0x01164CB0, blue, purple, orange,
// grey, yellow 0x01164CC4, pink, cyan) are written out as values.
// NOTE: most anim/bone hash defaults are names not present in the executable
// and stay as hashes ("Tower1" 0x93D8EE68, "Sequential" 0x13234CFF, "Butt"
// 0x001F4A13 are the known ones).

ape::TuningBool g_cheatUndetectable("Cheat.Undetectable", false);  // 00F9BBE0

ape::TuningBool g_aiLoadBalance("AI.LoadBalance", true);  // 00F9BC2C

ape::TuningFloat g_bismarqueHealth("Entity.Bismarque.Health", 600.0f);  // 00F954A0
ape::TuningFloat g_bismarqueTurnAnimMinAngle("Entity.Bismarque.TurnAnimMinAngle", 90.0f);  // 00F954EC
ape::TuningFloat g_bismarqueMovementAnimChoiceSplineLookAheadTime("Entity.Bismarque.Movement.AnimChoiceSplineLookAheadTime", 0.5f);  // 00F9561C
ape::TuningFloat g_bismarqueTargetPointDistanceWeight("Entity.Bismarque.Movement.TargetPointDistanceWeight", -0.1f);  // 00F95538
ape::TuningFloat g_bismarqueTargetPointAngleWeight("Entity.Bismarque.Movement.TargetPointAngleWeight", -1.0f);  // 00F95584
ape::TuningFloat g_bismarqueTargetPointDeltaOrientationWeight("Entity.Bismarque.Movement.TargetPointDeltaOrientationWeight", -1.0f);  // 00F955D0
ape::TuningFloat g_bismarqueMovementPathSwitchPointIterations("Entity.Bismarque.Movement.PathSwitchPointIterations", 16.0f);  // 00F95668
ape::TuningFloat g_bismarqueMovementDeltaPathSwitchPointDistance("Entity.Bismarque.Movement.DeltaPathSwitchPointDistance", 0.5f);  // 00F956B4
ape::TuningFloat g_bismarqueStunTime("Entity.Bismarque.StunTime", 10.0f);  // 00F95700
ape::TuningFloat g_bismarqueBeatdownSmackDamage("Entity.Bismarque.BeatdownSmackDamage", 20.0f);  // 00F9574C
ape::TuningFloat g_bismarqueThrownEntityDamage("Entity.Bismarque.ThrownEntityDamage", 10.0f);  // 00F95798
ape::TuningFloat g_bismarqueNormalMeleeDamage("Entity.Bismarque.NormalMeleeDamage", 20.0f);  // 00F957E4
ape::TuningFloat g_bismarqueRepelPlayerGroundForce("Entity.Bismarque.RepelPlayer.GroundForce", 20.0f);  // 00F95830
ape::TuningFloat g_bismarqueRepelPlayerUpAngle("Entity.Bismarque.RepelPlayer.UpAngle", 25.0f);  // 00F9587C
ape::TuningFloat g_bismarqueRepelPlayerAirHeight("Entity.Bismarque.RepelPlayer.AirHeight", 1.5f);  // 00F958C8
ape::TuningFloat g_bismarqueRepelPlayerAirForce("Entity.Bismarque.RepelPlayer.AirForce", 15.0f);  // 00F95914
ape::TuningFloat g_bismarqueRepelPlayerMaxDist("Entity.Bismarque.RepelPlayer.MaxDist", 17.0f);  // 00F95960
ape::TuningFloat g_bismarqueWhaleSplashDamage("Entity.Bismarque.WhaleSplashDamage", 20.0f);  // 00F95FE8
ape::TuningFloat g_bismarqueWhaleSplashImpulse("Entity.Bismarque.WhaleSplashImpulse", 6.0f);  // 00F96034
ape::TuningFloat g_bismarqueWhaleSplashRadius("Entity.Bismarque.WhaleSplashRadius", 3.0f);  // 00F96080
ape::TuningFloat g_bismarqueWhaleSplashHeight("Entity.Bismarque.WhaleSplashHeight", 1.5f);  // 00F960CC
ape::TuningFloat g_bismarqueQTEGameGrizzTimeDelay("Entity.Bismarque.QTE.Game.GrizzTimeDelay", 3.0f);  // 00F959AC
ape::TuningFloat g_bismarqueQTEGamePlyChaseFollowDist("Entity.Bismarque.QTE.Game.PlyChaseFollowDist", 1.5f);  // 00F959F8
ape::TuningFloat g_bismarqueQTEGameDetectBarInputBuffer("Entity.Bismarque.QTE.Game.DetectBar.InputBuffer", 0.5f);  // 00F95A44
ape::TuningFloat g_bismarqueQTEGameDetectBarMoveSpeed("Entity.Bismarque.QTE.Game.DetectBar.MoveSpeed", 1.5f);  // 00F95A90
ape::TuningFloat g_bismarqueQTEGameDetectBarYOffset("Entity.Bismarque.QTE.Game.DetectBar.YOffset", -2.0f);  // 00F95ADC
ape::TuningFloat g_bismarqueQTEGameDetectBarStartXOffset("Entity.Bismarque.QTE.Game.DetectBar.StartXOffset", -3.0f);  // 00F95B28
ape::TuningFloat g_bismarqueQTEGameIntroDelayTimeMax("Entity.Bismarque.QTE.Game.IntroDelayTimeMax", 3.7f);  // 00F95B74
ape::TuningFloat g_bismarqueQTEButtonShiftZPos("Entity.Bismarque.QTE.Button.ShiftZPos", -0.074f);  // 00F95BC0
ape::TuningFloat g_bismarqueQTEButtonPressTimeMax("Entity.Bismarque.QTE.Button.PressTimeMax", 0.2f);  // 00F95C0C
ape::TuningFloat g_bismarqueQTEButtonToPlyInterpTimeMax("Entity.Bismarque.QTE.Button.ToPlyInterpTimeMax", 1.2f);  // 00F95C58
ape::TuningFloat g_bismarqueQTEButtonToPlyInterpPercent1("Entity.Bismarque.QTE.Button.ToPlyInterpPercent1", 0.2f);  // 00F95CA4
ape::TuningFloat g_bismarqueQTEButtonToPlyInterpPercent2("Entity.Bismarque.QTE.Button.ToPlyInterpPercent2", 0.5f);  // 00F95CF0
ape::TuningFloat g_bismarqueQTEButtonToPlyInterpZ1("Entity.Bismarque.QTE.Button.ToPlyInterpZ1", 2.7f);  // 00F95D3C
ape::TuningFloat g_bismarqueQTEButtonToPlyInterpZ2("Entity.Bismarque.QTE.Button.ToPlyInterpZ2", 2.7f);  // 00F95D88
ape::TuningFloat g_bismarqueQTEButtonToPlyQuestionMarkRevealTimeMax("Entity.Bismarque.QTE.Button.ToPlyQuestionMarkRevealTimeMax", 0.5f);  // 00F95DD4
ape::TuningFloat g_bismarqueQTEButtonToGrizzIntroInterpTimeMax("Entity.Bismarque.QTE.Button.ToGrizzIntroInterpTimeMax", 0.6f);  // 00F95E20
ape::TuningFloat g_bismarqueQTEButtonToGrizzOutroInterpTimeMax("Entity.Bismarque.QTE.Button.ToGrizzOutroInterpTimeMax", 0.6f);  // 00F95E6C
ape::TuningFloat g_bismarqueQTEButtonToGrizzIntroDelayTimeMax("Entity.Bismarque.QTE.Button.ToGrizzIntroDelayTimeMax", 0.5f);  // 00F95EB8
ape::TuningFloat g_bismarqueQTEButtonToGrizzOutroDelayTimeMax("Entity.Bismarque.QTE.Button.ToGrizzOutroDelayTimeMax", 0.5f);  // 00F95F04
ape::TuningFloat g_bismarqueQTEButtonToGrizzStartXOffset("Entity.Bismarque.QTE.Button.ToGrizzStartXOffset", -3.0f);  // 00F95F50
ape::TuningFloat g_bismarqueQTEButtonToGrizzEndXOffset("Entity.Bismarque.QTE.Button.ToGrizzEndXOffset", 9.0f);  // 00F95F9C
ape::TuningFloat g_bismarqueSuperIceSliceAttackDistMin("Entity.Bismarque.SuperIceSlice.AttackDistMin", 4.0f);  // 00F96118
ape::TuningFloat g_bismarqueCounterAttackDistMin("Entity.Bismarque.Counter.AttackDistMin", 3.0f);  // 00F96164
ape::TuningFloat g_bismarqueCounterAttackDelayTimeMax("Entity.Bismarque.Counter.AttackDelayTimeMax", 3.0f);  // 00F961B0
ape::TuningBool g_bismarqueCounterEnabled("Entity.Bismarque.Counter.Enabled", true);  // 00F961FC
ape::TuningFloat g_bismarqueNormalCamFOV("Entity.Bismarque.NormalCam.FOV", 55.0f);  // 00F96248
ape::TuningFloat g_bismarqueNormalCamInterp("Entity.Bismarque.NormalCam.Interp", 0.05f);  // 00F96294
ape::TuningFloat g_bismarqueNormalCamFovInterp("Entity.Bismarque.NormalCam.FovInterp", 0.05f);  // 00F962E0
ape::TuningFloat g_bismarqueNormalCamGimbalRadius("Entity.Bismarque.NormalCam.GimbalRadius", 4.66f);  // 00F9632C
ape::TuningFloat g_bismarqueNormalCamAngleHome("Entity.Bismarque.NormalCam.AngleHome", 21.0f);  // 00F96378
ape::TuningVec3 g_bismarqueNormalCamTargetOffset("Entity.Bismarque.NormalCam.TargetOffset", 0.0f, 1.01f, 0.8f);  // 00F963D0
ape::TuningFloat g_bismarqueBeatdownCamFOV("Entity.Bismarque.BeatdownCam.FOV", 55.0f);  // 00F96430
ape::TuningFloat g_bismarqueBeatdownCamInterp("Entity.Bismarque.BeatdownCam.Interp", 0.05f);  // 00F9647C
ape::TuningFloat g_bismarqueBeatdownCamFovInterp("Entity.Bismarque.BeatdownCam.FovInterp", 0.05f);  // 00F964C8
ape::TuningFloat g_bismarqueBeatdownCamGimbalRadius("Entity.Bismarque.BeatdownCam.GimbalRadius", 4.66f);  // 00F96514
ape::TuningFloat g_bismarqueBeatdownCamAngleHome("Entity.Bismarque.BeatdownCam.AngleHome", 21.0f);  // 00F96560
ape::TuningVec3 g_bismarqueBeatdownCamTargetOffset("Entity.Bismarque.BeatdownCam.TargetOffset", 0.0f, 1.01f, 0.8f);  // 00F965B0
ape::TuningFloat g_bismarqueIceBarrierCamCloseFOV("Entity.Bismarque.IceBarrierCam.Close.FOV", 55.0f);  // 00F96610
ape::TuningFloat g_bismarqueIceBarrierCamCloseInterp("Entity.Bismarque.IceBarrierCam.Close.Interp", 0.01f);  // 00F9665C
ape::TuningFloat g_bismarqueIceBarrierCamCloseFovInterp("Entity.Bismarque.IceBarrierCam.Close.FovInterp", 0.01f);  // 00F966A8
ape::TuningFloat g_bismarqueIceBarrierCamCloseGimbalRadius("Entity.Bismarque.IceBarrierCam.Close.GimbalRadius", 4.66f);  // 00F966F4
ape::TuningFloat g_bismarqueIceBarrierCamCloseAngleHome("Entity.Bismarque.IceBarrierCam.Close.AngleHome", 21.0f);  // 00F96740
ape::TuningVec3 g_bismarqueIceBarrierCamCloseTargetOffset("Entity.Bismarque.IceBarrierCam.Close.TargetOffset", 0.0f, 1.01f, 0.8f);  // 00F96790
ape::TuningFloat g_bismarqueIceBarrierCamSwitchDist("Entity.Bismarque.IceBarrierCam.SwitchDist", 10.0f);  // 00F967F0
ape::TuningFloat g_bismarqueIceBarrierCamFarFOV("Entity.Bismarque.IceBarrierCam.Far.FOV", 55.0f);  // 00F9683C
ape::TuningFloat g_bismarqueIceBarrierCamFarInterp("Entity.Bismarque.IceBarrierCam.Far.Interp", 0.01f);  // 00F96888
ape::TuningFloat g_bismarqueIceBarrierCamFarFovInterp("Entity.Bismarque.IceBarrierCam.Far.FovInterp", 0.01f);  // 00F968D4
ape::TuningFloat g_bismarqueIceBarrierCamFarGimbalRadius("Entity.Bismarque.IceBarrierCam.Far.GimbalRadius", 4.66f);  // 00F96920
ape::TuningFloat g_bismarqueIceBarrierCamFarAngleHome("Entity.Bismarque.IceBarrierCam.Far.AngleHome", 21.0f);  // 00F9696C
ape::TuningVec3 g_bismarqueIceBarrierCamFarTargetOffset("Entity.Bismarque.IceBarrierCam.Far.TargetOffset", 0.0f, 1.01f, 0.8f);  // 00F969C0
ape::TuningFloat g_bismarqueStalactiteGroundOffset("Entity.Bismarque.Stalactite.GroundOffset", 14.6f);  // 00F96A20
ape::TuningColor g_bismarqueStalactiteGroundMarkerColorStart("Entity.Bismarque.Stalactite.GroundMarkerColorStart", { 0, 255, 0, 255 });  // 00F96A6C
ape::TuningColor g_bismarqueStalactiteGroundMarkerColorMid("Entity.Bismarque.Stalactite.GroundMarkerColorMid", { 255, 255, 0, 255 });  // 00F96AB8
ape::TuningColor g_bismarqueStalactiteGroundMarkerColorEnd("Entity.Bismarque.Stalactite.GroundMarkerColorEnd", { 255, 0, 0, 255 });  // 00F96B04
ape::TuningFloat g_bismarqueStalactiteGroundMarkerColorDuration("Entity.Bismarque.Stalactite.GroundMarkerColorDuration", 2.0f);  // 00F96B50
ape::TuningVec3 g_bismarqueStalactiteGroundMarkerScale("Entity.Bismarque.Stalactite.GroundMarkerScale", 2.0f, 2.0f, 2.0f);  // 00F96BA0
ape::TuningFloat g_bismarqueStalactitePredictFutureTime("Entity.Bismarque.Stalactite.PredictFutureTime", 1.0f);  // 00F96C00
ape::TuningFloat g_bismarqueStalactiteDiameter("Entity.Bismarque.Stalactite.Diameter", 8.22f);  // 00F96C4C
ape::TuningFloat g_bismarqueStalactiteGroundFudgeOffset("Entity.Bismarque.Stalactite.GroundFudgeOffset", 0.0f);  // 00F96C98
ape::TuningFloat g_bismarquePlayfieldBoundRadius("Entity.Bismarque.Playfield.BoundRadius", 30.0f);  // 00F96CE4
ape::TuningFloat g_bismarqueAttackRingSpeed("Entity.Bismarque.AttackRing.Speed", 10.0f);  // 00F96D30
ape::TuningFloat g_bismarqueAttackRingRange("Entity.Bismarque.AttackRing.Range", 20.0f);  // 00F96D7C
ape::TuningFloat g_bismarqueAttackRingHeight("Entity.Bismarque.AttackRing.Height", 1.0f);  // 00F96DC8
ape::TuningFloat g_bismarqueAttackRingRadius("Entity.Bismarque.AttackRing.Radius", 1.0f);  // 00F96E14
ape::TuningFloat g_bismarqueAttackRingDamage("Entity.Bismarque.AttackRing.Damage", 20.0f);  // 00F96E60
ape::TuningFloat g_bismarqueAttackRingImpulse("Entity.Bismarque.AttackRing.Impulse", 0.0f);  // 00F96EAC
ape::TuningBool g_bismarqueDebugDrawAttackRing("Entity.Bismarque.Debug.DrawAttackRing", false);  // 00F96EF8
ape::TuningFloat g_bismarqueGroundSmashStartDist("Entity.Bismarque.GroundSmash.StartDist", 15.31f);  // 00F96F44
ape::TuningFloat g_bismarqueBeatdownPunchCooldownTimeMax("Entity.Bismarque.Beatdown.PunchCooldownTimeMax", 0.5f);  // 00F96F90
ape::TuningVec3 g_bismarqueHealthSpawnOffset("Entity.Bismarque.Health.SpawnOffset", 0.0f, 3.6f, 1.6f);  // 00F96FE0
ape::TuningBool g_bismarqueEnableSkateBlur("Entity.Bismarque.EnableSkateBlur", false);  // 00F97040
ape::TuningFloat g_bismarqueIceBarrierYOffset("Entity.Bismarque.IceBarrier.YOffset", 0.0f);  // 00F9708C
ape::TuningFloat g_bismarqueIceBarrierPieceThrowLifetimeMax("Entity.Bismarque.IceBarrier.PieceThrowLifetimeMax", 7.0f);  // 00F970D8
ape::TuningFloat g_bismarqueIceBarrierThrowOffsetTimemax("Entity.Bismarque.IceBarrier.ThrowOffsetTimemax", 0.75f);  // 00F97124
ape::TuningFloat g_bismarqueIceBarrierThrowRampupTimeMax("Entity.Bismarque.IceBarrier.ThrowRampupTimeMax", 5.0f);  // 00F97170
ape::TuningFloat g_bismarqueWindStrength("Entity.Bismarque.Wind.Strength", 15000.0f);  // 00F971BC
ape::TuningFloat g_bismarqueWindRampUp("Entity.Bismarque.Wind.RampUp", 1.0f);  // 00F97208
ape::TuningFloat g_bismarqueWindRampDown("Entity.Bismarque.Wind.RampDown", 1.0f);  // 00F97254
ape::TuningFloat g_bismarqueWindAirDampeningTime("Entity.Bismarque.Wind.AirDampeningTime", 0.05f);  // 00F972A0
ape::TuningFloat g_bismarqueWindAirDampeningPercent("Entity.Bismarque.Wind.AirDampeningPercent", 0.3f);  // 00F972EC
ape::TuningBool g_bismarqueWindDebugDraw("Entity.Bismarque.Wind.DebugDraw", false);  // 00F97338
ape::TuningBool g_bismarqueDebugDrawPathSwitchPoints("Entity.Bismarque.Debug.DrawPathSwitchPoints", false);  // 00F97384
ape::TuningBool g_bismarqueDebugToMinigameState("Entity.Bismarque.Debug.ToMinigameState", false);  // 00F973D0
ape::TuningBool g_bismarqueDebugToBeatdownState("Entity.Bismarque.Debug.ToBeatdownState", false);  // 00F9741C
ape::TuningBool g_bismarqueDebugToBeatdownFinalState("Entity.Bismarque.Debug.ToBeatdownFinalState", false);  // 00F97468
ape::TuningBool g_bismarqueDebugDrawStalactite("Entity.Bismarque.Debug.DrawStalactite", false);  // 00F974B4
ape::TuningBool g_bismarqueFootTrailEnable("Entity.Bismarque.FootTrail.Enable", true);  // 00F9BCD8
ape::TuningVec3 g_bismarqueFootTrailOffsetLeft("Entity.Bismarque.FootTrail.OffsetLeft", 0.7f, 0.05f, 0.005f);  // 00F9BD30
ape::TuningVec3 g_bismarqueFootTrailOffsetRight("Entity.Bismarque.FootTrail.OffsetRight", -0.7f, 0.05f, 0.005f);  // 00F9BD90
ape::TuningFloat g_bismarqueFootTrailLength("Entity.Bismarque.FootTrail.Length", 3.0f);  // 00F9BDF0
ape::TuningFloat g_bismarqueFootTrailFPS("Entity.Bismarque.FootTrail.FPS", 60.0f);  // 00F9BE40
ape::TuningColor g_bismarqueFootTrailColor("Entity.Bismarque.FootTrail.Color", { 255, 255, 255, 255 });  // 00F9BE90
ape::TuningFloat g_bismarqueFootTrailWidth("Entity.Bismarque.FootTrail.Width", 0.375f);  // 00F9BEE0
ape::TuningFloat g_bismarqueFootTrailAlphaSpeed("Entity.Bismarque.FootTrail.AlphaSpeed", 5.0f);  // 00F9BF30
ape::TuningFloat g_bismarqueFootTrailWorldIcePos("Entity.Bismarque.FootTrail.WorldIcePos", 121.65f);  // 00F9BF80
ape::TuningFloat g_bismarqueFootTrailWorldIcePosThreshold("Entity.Bismarque.FootTrail.WorldIcePosThreshold", 0.25f);  // 00F9BFD0

ape::TuningBool g_elJefeDrawAttackCollision("Entity.ElJefe.DrawAttackCollision", false);  // 00F97500
ape::TuningFloat g_elJefeFireRingSpeed("Entity.ElJefe.FireRingSpeed", 10.0f);  // 00F9754C
ape::TuningFloat g_elJefeFireRingHeight("Entity.ElJefe.FireRingHeight", 1.0f);  // 00F97598
ape::TuningFloat g_elJefeFireRingRadius("Entity.ElJefe.FireRingRadius", 1.0f);  // 00F975E4
ape::TuningFloat g_elJefeFireRingRange("Entity.ElJefe.FireRingRange", 20.0f);  // 00F97630
ape::TuningFloat g_elJefeFireRingDamage("Entity.ElJefe.FireRingDamage", 20.0f);  // 00F9767C
ape::TuningFloat g_elJefeFireRingImpulse("Entity.ElJefe.FireRingImpulse", 0.0f);  // 00F976C8
ape::TuningFloat g_elJefeLargeFireRingSpeed("Entity.ElJefe.LargeFireRingSpeed", 6.0f);  // 00F97714
ape::TuningFloat g_elJefeLargeFireRingHeight("Entity.ElJefe.LargeFireRingHeight", 6.0f);  // 00F97760
ape::TuningFloat g_elJefeLargeFireRingRadius("Entity.ElJefe.LargeFireRingRadius", 1.0f);  // 00F977AC
ape::TuningFloat g_elJefeLargeFireRingRange("Entity.ElJefe.LargeFireRingRange", 20.0f);  // 00F977F8
ape::TuningFloat g_elJefeLargeFireRingDamage("Entity.ElJefe.LargeFireRingDamage", 20.0f);  // 00F97844
ape::TuningFloat g_elJefeLargeFireRingImpulse("Entity.ElJefe.LargeFireRingImpulse", 0.0f);  // 00F97890
ape::TuningFloat g_elJefeElectricBeamRange("Entity.ElJefe.ElectricBeamRange", 20.0f);  // 00F978DC
ape::TuningFloat g_elJefeElectricBeamRadius("Entity.ElJefe.ElectricBeamRadius", 1.0f);  // 00F97928
ape::TuningFloat g_elJefeElectricBeamDamage("Entity.ElJefe.ElectricBeamDamage", 30.0f);  // 00F97974
ape::TuningFloat g_elJefeTowerCamGimbalRadius("Entity.ElJefe.TowerCamGimbalRadius", 7.4f);  // 00F979C0
ape::TuningFloat g_elJefeTowerCamTargetYOffset("Entity.ElJefe.TowerCamTargetYOffset", 2.0f);  // 00F97A0C
ape::TuningFloat g_elJefeTowerCamAngle("Entity.ElJefe.TowerCamAngle", 13.0f);  // 00F97A58
ape::TuningFloat g_elJefeTowerCamInterp("Entity.ElJefe.TowerCamInterp", 0.015f);  // 00F97AA4
ape::TuningFloat g_elJefeElectricCamGimbalRadius("Entity.ElJefe.ElectricCamGimbalRadius", 10.0f);  // 00F97B3C
ape::TuningFloat g_elJefeElectricCamTargetYOffset("Entity.ElJefe.ElectricCamTargetYOffset", 3.0f);  // 00F97B88
ape::TuningFloat g_elJefeElectricCamAngle("Entity.ElJefe.ElectricCamAngle", 5.0f);  // 00F97BD4
ape::TuningFloat g_elJefeElectricCamInterp("Entity.ElJefe.ElectricCamInterp", 0.05f);  // 00F97C20
ape::TuningFloat g_elJefeFireBallCamGimbalRadius("Entity.ElJefe.FireBallCamGimbalRadius", 7.4f);  // 00F97C6C
ape::TuningFloat g_elJefeFireBallCamTargetYOffset("Entity.ElJefe.FireBallCamTargetYOffset", 2.0f);  // 00F97CB8
ape::TuningFloat g_elJefeFireBallCamAngle("Entity.ElJefe.FireBallCamAngle", 17.0f);  // 00F97D04
ape::TuningFloat g_elJefeFireBallCamInterp("Entity.ElJefe.FireBallCamInterp", 0.1f);  // 00F97D50
ape::TuningFloat g_elJefeFireRingCamGimbalRadius("Entity.ElJefe.FireRingCamGimbalRadius", 13.0f);  // 00F97D9C
ape::TuningFloat g_elJefeFireRingCamTargetYOffset("Entity.ElJefe.FireRingCamTargetYOffset", 3.0f);  // 00F97DE8
ape::TuningFloat g_elJefeFireRingCamAngle("Entity.ElJefe.FireRingCamAngle", 5.0f);  // 00F97E34
ape::TuningFloat g_elJefeFireRingCamInterp("Entity.ElJefe.FireRingCamInterp", 0.1f);  // 00F97E80
ape::TuningFloat g_elJefeStunnedCamGimbalRadius("Entity.ElJefe.StunnedCamGimbalRadius", 2.0f);  // 00F97ECC
ape::TuningFloat g_elJefeStunnedCamTargetYOffset("Entity.ElJefe.StunnedCamTargetYOffset", 1.0f);  // 00F97F18
ape::TuningFloat g_elJefeStunnedCamAngle("Entity.ElJefe.StunnedCamAngle", 5.0f);  // 00F97F64
ape::TuningFloat g_elJefeStunnedCamInterp("Entity.ElJefe.StunnedCamInterp", 0.1f);  // 00F97FB0
ape::TuningFloat g_elJefeLightningMinTime("Entity.ElJefe.LightningMinTime", 2.0f);  // 00F97FFC
ape::TuningFloat g_elJefeLightningMaxTime("Entity.ElJefe.LightningMaxTime", 4.0f);  // 00F98048
ape::TuningBool g_elJefeDisableDualTargetCam("Entity.ElJefe.DisableDualTargetCam", false);  // 00F97AF0
ape::TuningHash g_elJefeStage1("Entity.ElJefe.Stage1", 0x75742F36u);  // 00F98094
ape::TuningHash g_elJefeStage2("Entity.ElJefe.Stage2", 0x75742F36u);  // 00F980E0
ape::TuningHash g_elJefeStage3("Entity.ElJefe.Stage3", 0x75742F35u);  // 00F9812C
ape::TuningHash g_elJefeStage4("Entity.ElJefe.Stage4", 0x75742F35u);  // 00F98178
ape::TuningHash g_elJefeStage5("Entity.ElJefe.Stage5", 0x0A211E20u);  // 00F981C4
ape::TuningHash g_elJefeStage6("Entity.ElJefe.Stage6", 0x93D8EE68u);  // 00F98210
ape::TuningHash g_elJefeStage7("Entity.ElJefe.Stage7", 0x0A211E20u);  // 00F9825C
ape::TuningHash g_elJefeStage8("Entity.ElJefe.Stage8", 0x75742F34u);  // 00F982A8
ape::TuningHash g_elJefeStage9("Entity.ElJefe.Stage9", 0x75742F34u);  // 00F982F4
ape::TuningHash g_elJefeStage10("Entity.ElJefe.Stage10", 0x93D8EE69u);  // 00F98340
ape::TuningHash g_elJefeStage11("Entity.ElJefe.Stage11", 0x75742F35u);  // 00F9838C
ape::TuningHash g_elJefeStage12("Entity.ElJefe.Stage12", 0x75742F35u);  // 00F983D8
ape::TuningHash g_elJefeStage13("Entity.ElJefe.Stage13", 0x93D8EE6Au);  // 00F98424
ape::TuningHash g_elJefeStage14("Entity.ElJefe.Stage14", 0x13234CFFu);  // 00F98470
ape::TuningHash g_elJefeStage15("Entity.ElJefe.Stage15", 0x13234CFFu);  // 00F984BC
ape::TuningHash g_elJefeStage16("Entity.ElJefe.Stage16", 0x13234CFFu);  // 00F98508
ape::TuningHash g_elJefeStage17("Entity.ElJefe.Stage17", 0x13234CFFu);  // 00F98554
ape::TuningHash g_elJefeStage18("Entity.ElJefe.Stage18", 0x13234CFFu);  // 00F985A0
ape::TuningHash g_elJefeStage19("Entity.ElJefe.Stage19", 0x13234CFFu);  // 00F985EC
ape::TuningHash g_elJefeStage20("Entity.ElJefe.Stage20", 0x13234CFFu);  // 00F98638
ape::TuningFloat g_elJefeHealth("Entity.ElJefe.Health", 1000.0f);  // 00F98768
ape::TuningFloat g_elJefeSpawnHealthRate("Entity.ElJefe.SpawnHealthRate", 0.25f);  // 00F987B4
ape::TuningInt g_elJefeHitsToDizzy("Entity.ElJefe.HitsToDizzy", 1);  // 00F98800
ape::TuningFloat g_elJefeStunnedTimeTower1("Entity.ElJefe.StunnedTimeTower1", 6.0f);  // 00F9884C
ape::TuningFloat g_elJefeStunnedTimeTower2("Entity.ElJefe.StunnedTimeTower2", 4.5f);  // 00F98898
ape::TuningFloat g_elJefeStunnedTimeTower3("Entity.ElJefe.StunnedTimeTower3", 3.5f);  // 00F988E4
ape::TuningFloat g_elJefeTower1Threshold("Entity.ElJefe.Tower1Threshold", 0.66f);  // 00F98684
ape::TuningFloat g_elJefeTower2Threshold("Entity.ElJefe.Tower2Threshold", 0.33f);  // 00F986D0
ape::TuningFloat g_elJefeTower3Threshold("Entity.ElJefe.Tower3Threshold", 0.0f);  // 00F9871C
ape::TuningFloat g_elJefeGridAttackRate("Entity.ElJefe.GridAttackRate", 0.5f);  // 00F98930
ape::TuningFloat g_elJefeGridAttackArcDuration("Entity.ElJefe.GridAttackArcDuration", 0.5f);  // 00F9897C
ape::TuningFloat g_elJefeGridAttackTellDuration("Entity.ElJefe.GridAttackTellDuration", 0.5f);  // 00F989C8
ape::TuningFloat g_elJefeGridAttackDamageDuration("Entity.ElJefe.GridAttackDamageDuration", 10.0f);  // 00F98A14
ape::TuningFloat g_elJefeGridAttackDamage("Entity.ElJefe.GridAttackDamage", 20.0f);  // 00F98A60
ape::TuningString g_elJefeGridPatternTower2("Entity.ElJefe.GridPatternTower2", "C5C4C2C1C0A3E3B3D3F5");  // 00F98AAC
ape::TuningString g_elJefeGridPatternTower3("Entity.ElJefe.GridPatternTower3", "C5C4C2C1A3E3B3D3");  // 00F98B74
ape::TuningString g_elJefeTower1Pattern("Entity.ElJefe.Tower1Pattern", "SEQFSSEWFSEQFW");  // 00F98C3C
ape::TuningString g_elJefeTower2Pattern("Entity.ElJefe.Tower2Pattern", "LGEQFLGEQWFLGEQF");  // 00F98D04
ape::TuningString g_elJefeTower3Pattern("Entity.ElJefe.Tower3Pattern", "LGEWQFLMWQFLMWQWF");  // 00F98DCC
ape::TuningFloat g_elJefeTower1IdleSpeedMul("Entity.ElJefe.Tower1IdleSpeedMul", 0.8f);  // 00F98E94
ape::TuningFloat g_elJefeTower2IdleSpeedMul("Entity.ElJefe.Tower2IdleSpeedMul", 0.8f);  // 00F98EE0
ape::TuningFloat g_elJefeTower3IdleSpeedMul("Entity.ElJefe.Tower3IdleSpeedMul", 0.8f);  // 00F98F2C
ape::TuningFloat g_elJefePairedActionDist("Entity.ElJefe.PairedActionDist", 4.0f);  // 00F98F78
ape::TuningFloat g_elJefePairedActionDamage("Entity.ElJefe.PairedActionDamage", 30.0f);  // 00F98FC4
ape::TuningFloat g_elJefeRoarKnockback("Entity.ElJefe.RoarKnockback", 10.0f);  // 00F99010

ape::TuningFloat g_msDecibelHealth("Entity.MsDecibel.Health", 600.0f);  // 00F9905C
ape::TuningFloat g_msDecibelPlatform1HealthThreshold("Entity.MsDecibel.Platform1HealthThreshold", 500.0f);  // 00F990A8
ape::TuningFloat g_msDecibelPlatform2HealthThreshold("Entity.MsDecibel.Platform2HealthThreshold", 400.0f);  // 00F990F4
ape::TuningFloat g_msDecibelPlatform3HealthThreshold("Entity.MsDecibel.Platform3HealthThreshold", 300.0f);  // 00F99140
ape::TuningBool g_msDecibelJumboTronPosTypeCamera("Entity.MsDecibel.JumboTron.PosTypeCamera", false);  // 00F9AA04
ape::TuningFloat g_msDecibelJumboTronCamOffset("Entity.MsDecibel.JumboTron.CamOffset", -50.0f);  // 00F9AA50
ape::TuningFloat g_msDecibelJumboTronMaxSpeed("Entity.MsDecibel.JumboTron.MaxSpeed", 5.0f);  // 00F9AA9C
ape::TuningBool g_msDecibelJumboTronDebugDraw("Entity.MsDecibel.JumboTron.DebugDraw", false);  // 00F9AAE8
ape::TuningColor g_msDecibelJumboTronBGColor0("Entity.MsDecibel.JumboTron.BGColor0", { 0, 115, 0, 255 });  // 00F9AB34
ape::TuningColor g_msDecibelJumboTronBGColor1("Entity.MsDecibel.JumboTron.BGColor1", { 0, 255, 0, 255 });  // 00F9AB80
ape::TuningBool g_msDecibelDrawAttackCollision("Entity.MsDecibel.DrawAttackCollision", false);  // 00F9ABCC
ape::TuningBool g_msDecibelUseTriggersForPlatformCombat("Entity.MsDecibel.UseTriggersForPlatformCombat", false);  // 00F9AC18
ape::TuningFloat g_msDecibelBeatdownTime1("Entity.MsDecibel.BeatdownTime1", 16.0f);  // 00F9AC64
ape::TuningFloat g_msDecibelBeatdownTime2("Entity.MsDecibel.BeatdownTime2", 8.0f);  // 00F9ACB0
ape::TuningFloat g_msDecibelBeatdownTime3("Entity.MsDecibel.BeatdownTime3", 8.0f);  // 00F9ACFC
ape::TuningFloat g_msDecibelProjectileTime("Entity.MsDecibel.ProjectileTime", 3.0f);  // 00F9AD48
ape::TuningFloat g_msDecibelGlassShatterSequenceTime("Entity.MsDecibel.GlassShatterSequenceTime", 1.5f);  // 00F991D8
ape::TuningFloat g_msDecibelGlassShatterTimeVariance("Entity.MsDecibel.Glass.ShatterTimeVariance", 3.0f);  // 00F9918C
ape::TuningFloat g_msDecibelGlassResonateTime("Entity.MsDecibel.Glass.ResonateTime", 0.5f);  // 00F99224
ape::TuningFloat g_msDecibelGlassResonateAmplitude("Entity.MsDecibel.Glass.ResonateAmplitude", 0.1f);  // 00F99270
ape::TuningFloat g_msDecibelGlassResonateFrequency("Entity.MsDecibel.Glass.ResonateFrequency", 32.0f);  // 00F992BC
ape::TuningFloat g_msDecibelGlassResetTime("Entity.MsDecibel.Glass.ResetTime", 1.5f);  // 00F99308
ape::TuningBool g_msDecibelGlassResetAllAtOnce("Entity.MsDecibel.Glass.ResetAllAtOnce", false);  // 00F99354
ape::TuningBool g_msDecibelGlassUseSequences("Entity.MsDecibel.Glass.UseSequences", true);  // 00F993A0
ape::TuningBool g_msDecibelGlassUseSafeGroup("Entity.MsDecibel.Glass.UseSafeGroup", true);  // 00F993EC
ape::TuningBool g_msDecibelGlassDebug("Entity.MsDecibel.Glass.Debug", false);  // 00F99438
ape::TuningString g_msDecibelGlassRow1("Entity.MsDecibel.Glass.Row1", "23121");  // 00F99484
ape::TuningString g_msDecibelGlassRow2("Entity.MsDecibel.Glass.Row2", "21323");  // 00F9954C
ape::TuningString g_msDecibelGlassRow3("Entity.MsDecibel.Glass.Row3", "12312");  // 00F99614
ape::TuningString g_msDecibelGlassRow4("Entity.MsDecibel.Glass.Row4", "13223");  // 00F996DC
ape::TuningString g_msDecibelGlassRow5("Entity.MsDecibel.Glass.Row5", "31112");  // 00F997A4
ape::TuningString g_msDecibelGlassRow6("Entity.MsDecibel.Glass.Row6", "23321");  // 00F9986C
ape::TuningString g_msDecibelGlassRow7("Entity.MsDecibel.Glass.Row7", "13213");  // 00F99934
ape::TuningFloat g_msDecibelMusicV("Entity.MsDecibel.Music.V", 0.0f);  // 00F9A414
ape::TuningFloat g_msDecibelMusicStep("Entity.MsDecibel.Music.Step", 0.0f);  // 00F9A460
ape::TuningFloat g_msDecibelMusicAnimOffset("Entity.MsDecibel.Music.AnimOffset", 0.0f);  // 00F9A4AC
ape::TuningBool g_msDecibelMusicCamEnabled("Entity.MsDecibel.Music.CamEnabled", true);  // 00F9A4F8
ape::TuningBool g_msDecibelMusicCamUsePlayerY("Entity.MsDecibel.Music.CamUsePlayerY", false);  // 00F9A544
ape::TuningFloat g_msDecibelMusicCamGimbalRadius("Entity.MsDecibel.Music.CamGimbalRadius", 14.0f);  // 00F9A590
ape::TuningFloat g_msDecibelMusicCamHeight("Entity.MsDecibel.Music.CamHeight", 1.5f);  // 00F9A5DC
ape::TuningFloat g_msDecibelMusicCamTargetYOffset("Entity.MsDecibel.Music.CamTargetYOffset", -10.8f);  // 00F9A628
ape::TuningFloat g_msDecibelMusicCamLookAhead("Entity.MsDecibel.Music.CamLookAhead", 25.0f);  // 00F9A674
ape::TuningFloat g_msDecibelMusicCamInterp("Entity.MsDecibel.Music.CamInterp", 1.0f);  // 00F9A6C0
ape::TuningFloat g_msDecibelMusicSpeed("Entity.MsDecibel.Music.Speed", 15.0f);  // 00F9A70C
ape::TuningFloat g_msDecibelMusicTempoBPM("Entity.MsDecibel.Music.TempoBPM", 180.0f);  // 00F9A758
ape::TuningFloat g_msDecibelMusicSpeed2("Entity.MsDecibel.Music.Speed2", 15.0f);  // 00F9A7A4
ape::TuningFloat g_msDecibelMusicTempoBPM2("Entity.MsDecibel.Music.TempoBPM2", 220.0f);  // 00F9A7F0
ape::TuningFloat g_msDecibelMusicFadeDist("Entity.MsDecibel.Music.FadeDist", 15.0f);  // 00F9A83C
ape::TuningFloat g_msDecibelMusicIntroSpeedScale("Entity.MsDecibel.Music.IntroSpeedScale", 2.0f);  // 00F9A888
ape::TuningFloat g_msDecibelPlatformShieldCamGimbalRadius("Entity.MsDecibel.Platform.ShieldCamGimbalRadius", 16.0f);  // 00F9A2E4
ape::TuningFloat g_msDecibelPlatformShieldCamTargetYOffset("Entity.MsDecibel.Platform.ShieldCamTargetYOffset", 5.0f);  // 00F9A330
ape::TuningFloat g_msDecibelPlatformShieldCamAngle("Entity.MsDecibel.Platform.ShieldCamAngle", 5.0f);  // 00F9A37C
ape::TuningFloat g_msDecibelPlatformShieldCamInterp("Entity.MsDecibel.Platform.ShieldCamInterp", 0.5f);  // 00F9A3C8
ape::TuningFloat g_msDecibelPlatformStunnedCamGimbalRadius("Entity.MsDecibel.Platform.StunnedCamGimbalRadius", 3.5f);  // 00F9A8D4
ape::TuningFloat g_msDecibelPlatformStunnedCamTargetYOffset("Entity.MsDecibel.Platform.StunnedCamTargetYOffset", 0.0f);  // 00F9A920
ape::TuningFloat g_msDecibelPlatformStunnedCamAngle("Entity.MsDecibel.Platform.StunnedCamAngle", 5.0f);  // 00F9A96C
ape::TuningFloat g_msDecibelPlatformStunnedCamInterp("Entity.MsDecibel.Platform.StunnedCamInterp", 0.5f);  // 00F9A9B8
ape::TuningFloat g_msDecibelPlatformProjectileRotationRate("Entity.MsDecibel.Platform.ProjectileRotationRate", 60.0f);  // 00F9AF5C
ape::TuningFloat g_msDecibelPlatformCounterAttackDist("Entity.MsDecibel.Platform.CounterAttackDist", 8.0f);  // 00F9AFA8
ape::TuningInt g_msDecibelPlatformPlatform1HitsToDizzy("Entity.MsDecibel.Platform.Platform1HitsToDizzy", 1);  // 00F9AFF4
ape::TuningInt g_msDecibelPlatformPlatform2HitsToDizzy("Entity.MsDecibel.Platform.Platform2HitsToDizzy", 2);  // 00F9B040
ape::TuningInt g_msDecibelPlatformPlatform3HitsToDizzy("Entity.MsDecibel.Platform.Platform3HitsToDizzy", 3);  // 00F9B08C
ape::TuningInt g_msDecibelFinalHitsToCharge("Entity.MsDecibel.Final.HitsToCharge", 3);  // 00F99D40
ape::TuningFloat g_msDecibelFinalCamGimbalRadius("Entity.MsDecibel.Final.CamGimbalRadius", 6.0f);  // 00F99D8C
ape::TuningFloat g_msDecibelFinalCamTargetYOffset("Entity.MsDecibel.Final.CamTargetYOffset", 1.5f);  // 00F99DD8
ape::TuningFloat g_msDecibelFinalCamAngle("Entity.MsDecibel.Final.CamAngle", 15.0f);  // 00F99E24
ape::TuningFloat g_msDecibelFinalCamInterp("Entity.MsDecibel.Final.CamInterp", 1.0f);  // 00F99E70
ape::TuningFloat g_msDecibelFinalBounceCamGimbalRadius("Entity.MsDecibel.Final.BounceCamGimbalRadius", 12.0f);  // 00F99EBC
ape::TuningFloat g_msDecibelFinalBounceCamTargetYOffset("Entity.MsDecibel.Final.BounceCamTargetYOffset", 4.0f);  // 00F99F08
ape::TuningFloat g_msDecibelFinalBounceCamAngle("Entity.MsDecibel.Final.BounceCamAngle", 5.0f);  // 00F99F54
ape::TuningFloat g_msDecibelFinalBounceCamInterp("Entity.MsDecibel.Final.BounceCamInterp", 0.5f);  // 00F99FA0
ape::TuningFloat g_msDecibelFinalSatCamInterp("Entity.MsDecibel.Final.SatCamInterp", 3.5f);  // 00F99FEC
ape::TuningFloat g_msDecibelFinalSatCamInterpOut("Entity.MsDecibel.Final.SatCamInterpOut", 1.0f);  // 00F9A038
ape::TuningFloat g_msDecibelFinalSatCamOffsetY("Entity.MsDecibel.Final.SatCamOffsetY", 10.0f);  // 00F9A084
ape::TuningFloat g_msDecibelFinalSatCamOffsetZ("Entity.MsDecibel.Final.SatCamOffsetZ", 0.0f);  // 00F9A0D0
ape::TuningFloat g_msDecibelFinalSatCamZMulSpread("Entity.MsDecibel.Final.SatCamZMulSpread", 0.75f);  // 00F9A11C
ape::TuningFloat g_msDecibelFinalSatCamZMulSly("Entity.MsDecibel.Final.SatCamZMulSly", 0.75f);  // 00F9A168
ape::TuningFloat g_msDecibelFinalSatCamDist("Entity.MsDecibel.Final.SatCamDist", 90.0f);  // 00F9A1B4
ape::TuningFloat g_msDecibelFinalDashStopAngle("Entity.MsDecibel.Final.DashStopAngle", 45.0f);  // 00F9A200
ape::TuningFloat g_msDecibelFinalDashCloseAngle("Entity.MsDecibel.Final.DashCloseAngle", 10.0f);  // 00F9A24C
ape::TuningFloat g_msDecibelFinalDashCloseDist("Entity.MsDecibel.Final.DashCloseDist", 9.0f);  // 00F9A298
ape::TuningFloat g_msDecibelFinalShockWaveSpeed("Entity.MsDecibel.Final.ShockWaveSpeed", 20.0f);  // 00F9AD94
ape::TuningFloat g_msDecibelFinalShockWaveHeight("Entity.MsDecibel.Final.ShockWaveHeight", 1.0f);  // 00F9ADE0
ape::TuningFloat g_msDecibelFinalShockWaveRadius("Entity.MsDecibel.Final.ShockWaveRadius", 1.0f);  // 00F9AE2C
ape::TuningFloat g_msDecibelFinalShockWaveRange("Entity.MsDecibel.Final.ShockWaveRange", 40.0f);  // 00F9AE78
ape::TuningFloat g_msDecibelFinalShockWaveDamage("Entity.MsDecibel.Final.ShockWaveDamage", 20.0f);  // 00F9AEC4
ape::TuningFloat g_msDecibelFinalMusicNoteDamage("Entity.MsDecibel.Final.MusicNoteDamage", 20.0f);  // 00F9AF10
ape::TuningFloat g_msDecibelFinalProjectileRotationRate("Entity.MsDecibel.Final.ProjectileRotationRate", 30.0f);  // 00F9B0D8
ape::TuningFloat g_msDecibelFinalTrunkSwipeDist("Entity.MsDecibel.Final.TrunkSwipeDist", 9.0f);  // 00F9B124
ape::TuningString g_msDecibelPatternPlatform1("Entity.MsDecibel.Pattern_Platform1", "B");  // 00F9B170
ape::TuningString g_msDecibelPatternPlatform2("Entity.MsDecibel.Pattern_Platform2", "MB");  // 00F9B238
ape::TuningString g_msDecibelPatternPlatform3("Entity.MsDecibel.Pattern_Platform3", "MBHB");  // 00F9B300
ape::TuningString g_msDecibelPatternFinal1("Entity.MsDecibel.Pattern_Final1", "WP");  // 00F9B3C8
ape::TuningString g_msDecibelPatternFinal2("Entity.MsDecibel.Pattern_Final2", "WMHB");  // 00F9B490
ape::TuningString g_msDecibelPatternFinal3("Entity.MsDecibel.Pattern_Final3", "WMHWB");  // 00F9B558
ape::TuningString g_msDecibelPatternFinal4("Entity.MsDecibel.Pattern_Final4", "WMHMH");  // 00F9B620
ape::TuningBool g_msDecibelDebugTrunkSwipe("Entity.MsDecibel.Debug.TrunkSwipe", false);  // 00F9B6E8
ape::TuningBool g_msDecibelDebugProjectiles("Entity.MsDecibel.Debug.Projectiles", false);  // 00F9B734
ape::TuningBool g_msDecibelDebugGroundSmash("Entity.MsDecibel.Debug.GroundSmash", false);  // 00F9B780
ape::TuningFloat g_msDecibelShieldSpawnRate("Entity.MsDecibel.ShieldSpawnRate", 1.0f);  // 00F999FC
ape::TuningFloat g_msDecibelShieldSpinRate("Entity.MsDecibel.ShieldSpinRate", 800.0f);  // 00F99A48
ape::TuningFloat g_msDecibelShieldCycleDuration("Entity.MsDecibel.ShieldCycleDuration", 30.0f);  // 00F99A94
ape::TuningFloat g_msDecibelShieldSpeedBase("Entity.MsDecibel.ShieldSpeedBase", 5.0f);  // 00F99AE0
ape::TuningFloat g_msDecibelShieldSpeedAddMin("Entity.MsDecibel.ShieldSpeedAddMin", 2.0f);  // 00F99B2C
ape::TuningFloat g_msDecibelShieldSpeedAddMax("Entity.MsDecibel.ShieldSpeedAddMax", 3.0f);  // 00F99B78
ape::TuningFloat g_msDecibelShieldOffset1("Entity.MsDecibel.ShieldOffset1", 0.0f);  // 00F99BC4
ape::TuningFloat g_msDecibelShieldOffset2("Entity.MsDecibel.ShieldOffset2", 120.0f);  // 00F99C10
ape::TuningFloat g_msDecibelShieldOffset3("Entity.MsDecibel.ShieldOffset3", 240.0f);  // 00F99C5C
ape::TuningFloat g_msDecibelShieldSpread("Entity.MsDecibel.ShieldSpread", 2.0f);  // 00F99CA8
ape::TuningBool g_msDecibelShieldLines("Entity.MsDecibel.ShieldLines", true);  // 00F99CF4

ape::TuningFloat g_paradoxDamageToSly("Entity.Paradox.DamageToSly", 20.0f);  // 00F9C028
ape::TuningFloat g_paradoxDamageToParadox("Entity.Paradox.DamageToParadox", 5.0f);  // 00F9C078
ape::TuningBool g_paradoxPunishIncorrectInput("Entity.Paradox.PunishIncorrectInput", false);  // 00F9C0C8
ape::TuningFloat g_paradoxAttackTellTimeScale("Entity.Paradox.Attack.TellTimeScale", 0.02f);  // 00F9C118
ape::TuningFloat g_paradoxTraversalDist("Entity.Paradox.TraversalDist", 6.0f);  // 00F9C168
ape::TuningFloat g_paradoxGrindSpeed("Entity.Paradox.GrindSpeed", 12.0f);  // 00F9C1B8
ape::TuningFloat g_paradoxGrindJumpLead("Entity.Paradox.GrindJumpLead", 0.65f);  // 00F9C208
ape::TuningFloat g_paradoxGrindInitialLead("Entity.Paradox.GrindInitialLead", 0.05f);  // 00F9C258
ape::TuningBool g_paradoxCameraUseRandom("Entity.Paradox.Camera.UseRandom", false);  // 00F9C2A8
ape::TuningFloat g_paradoxCameraDist("Entity.Paradox.Camera.Dist", 5.5f);  // 00F9C2F8
ape::TuningFloat g_paradoxCameraXOffset("Entity.Paradox.Camera.XOffset", -3.0f);  // 00F9C348
ape::TuningFloat g_paradoxCameraZoomDist("Entity.Paradox.Camera.ZoomDist", 3.0f);  // 00F9C398
ape::TuningFloat g_paradoxCameraZoomInterpTime("Entity.Paradox.Camera.ZoomInterpTime", 0.5f);  // 00F9C3E8
ape::TuningBool g_paradoxCameraZoomDuringParryWindow("Entity.Paradox.Camera.ZoomDuringParryWindow", false);  // 00F9C438
ape::TuningFloat g_paradoxCameraHeight("Entity.Paradox.Camera.Height", 1.2f);  // 00F9C488
ape::TuningFloat g_paradoxCameraTargetHeight("Entity.Paradox.Camera.TargetHeight", 1.2f);  // 00F9C4D8
ape::TuningFloat g_paradoxCameraFOV("Entity.Paradox.Camera.FOV", 55.0f);  // 00F9C528
ape::TuningFloat g_paradoxCameraInterp("Entity.Paradox.Camera.Interp", 1.0f);  // 00F9C578
ape::TuningBool g_paradoxDebugEnabled("Entity.Paradox.Debug.Enabled", false);  // 00F9C5C8
ape::TuningBool g_paradoxDebugTestSequence("Entity.Paradox.Debug.TestSequence", false);  // 00F9C618
ape::TuningBool g_paradoxDebugFastInputs("Entity.Paradox.Debug.FastInputs", false);  // 00F9C668
ape::TuningBool g_paradoxDebugSlowInputs("Entity.Paradox.Debug.SlowInputs", false);  // 00F9C6B8

ape::TuningBool g_aiThrownObjDamageEnable("AI.ThrownObjDamageEnable", true);  // 00F9C704
ape::TuningFloat g_aiThrownObjDamageAmount("AI.ThrownObjDamageAmount", 50.0f);  // 00F9C750
ape::TuningFloat g_aiImpactDamage("AI.ImpactDamage", 100.0f);  // 00F9C79C
ape::TuningColor g_aiStealthTargetColor("AI.StealthTargetColor", { 0, 0, 255, 255 });  // 00F9C7E8
ape::TuningFloat g_aiStealthTargetPulseRate("AI.StealthTargetPulseRate", 20.0f);  // 00F9C834
ape::TuningFloat g_aiFailCamTargetDist("AI.FailCamTargetDist", 5.0f);  // 00F9C880
ape::TuningFloat g_aiFailCamFOVScaler("AI.FailCamFOVScaler", 0.5f);  // 00F9C8CC
ape::TuningFloat g_aiShrinkTime("AI.ShrinkTime", 10.0f);  // 00F9C918
ape::TuningFloat g_aiShrinkInterpTime("AI.ShrinkInterpTime", 1.0f);  // 00F9C964
ape::TuningFloat g_aiShrinkScale("AI.ShrinkScale", 0.25f);  // 00F9C9B0
ape::TuningFloat g_aiConfuseTime("AI.ConfuseTime", 10.0f);  // 00F9C9FC
ape::TuningFloat g_aiScareTime("AI.ScareTime", 10.0f);  // 00F9CA48
ape::TuningFloat g_aiFatalFallDist("AI.FatalFallDist", 6.5f);  // 00F9CA94
ape::TuningFloat g_aiSneakAttackWindow("AI.SneakAttackWindow", 0.6f);  // 00F9CAE0

ape::TuningBool g_bismarqueIceCaveSearchTowardsPlayer("Entity.BismarqueIceCave.SearchTowardsPlayer", false);  // 00F9B7CC
ape::TuningFloat g_bismarqueIceCaveMinTimeBetweenActions("Entity.BismarqueIceCave.MinTimeBetweenActions", 2.0f);  // 00F9B818
ape::TuningFloat g_bismarqueIceCaveMaxTimeBetweenActions("Entity.BismarqueIceCave.MaxTimeBetweenActions", 4.0f);  // 00F9B864
ape::TuningHash g_bismarqueIceCaveDiggingMachineInst("Entity.BismarqueIceCave.DiggingMachineInst", 0x6263D7FFu);  // 00F9B8B0
ape::TuningFloat g_bismarqueIceCaveImpatientAnimationWeight("Entity.BismarqueIceCave.ImpatientAnimationWeight", 3.0f);  // 00F9B8FC
ape::TuningFloat g_bismarqueIceCaveLookForPlayerWeight("Entity.BismarqueIceCave.LookForPlayerWeight", 1.0f);  // 00F9B948
ape::TuningFloat g_bismarqueIceCaveApproachPlayerWeight("Entity.BismarqueIceCave.ApproachPlayerWeight", 1.0f);  // 00F9B994
ape::TuningHash g_bismarqueIceCaveButtBone("Entity.BismarqueIceCave.ButtBone", 0x001F4A13u);  // 00F9B9E0
ape::TuningHash g_bismarqueIceCaveDartDef("Entity.BismarqueIceCave.DartDef", 0x689D8D4Du);  // 00F9BA2C
ape::TuningFloat g_bismarqueIceCaveButtRadius("Entity.BismarqueIceCave.ButtRadius", 1.0f);  // 00F9BA78
ape::TuningFloat g_bismarqueIceCaveButtLength("Entity.BismarqueIceCave.ButtLength", 2.5f);  // 00F9BAC4
ape::TuningBool g_bismarqueIceCaveDebugButt("Entity.BismarqueIceCave.DebugButt", false);  // 00F9BB10

ape::TuningFloat g_aiAttackFrequency("AI.AttackFrequency", 1.0f);  // 00F9CB2C
ape::TuningFloat g_aiChaseVOFrequency("AI.ChaseVOFrequency", 5.0f);  // 00F9CB78
ape::TuningFloat g_aiAttackPlayerRecoveryTime("AI.AttackPlayerRecoveryTime", 3.0f);  // 00F9CBC4
ape::TuningFloat g_aiAudioAlertRangeNormal("AI.AudioAlertRangeNormal", 15.0f);  // 00F9CC10
ape::TuningFloat g_aiAudioAlertRangeFootsteps("AI.AudioAlertRangeFootsteps", 7.5f);  // 00F9CC5C
ape::TuningFloat g_aiAudioAlertRangeBottle("AI.AudioAlertRangeBottle", 15.0f);  // 00F9CCA8

const ape::Matrix34 g_aiFrameXRotNeg90 = {
    { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }
};  // 00F9BB60
const ape::Matrix34 g_aiFrameXRotPos90 = {
    { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }
};  // 00F9BBA0

} // namespace ai
} // namespace levels
} // namespace sly
