// openTIT - Tuning variables of the entity component module.
//
// Defaults; Scripts/Tuning.xml may override them. Values were extracted from the
// disassembly of the module's static initialiser (Ghidra loses most float arguments).
#include "sly/components/ComponentTuning.h"

#include "sly/components/InventoryDescs.h"

namespace sly {
namespace components {

// @func 0x002D6A40 StaticInit_ComponentTuning
// Compiler-generated initialiser for the globals of the component module, in
// console order. Besides the tuning variables below it constructs the module's
// registries, which live with the code that uses them:
//   0x01056F04 component template table (cap 0x140, sly/entity RegisterComponentTemplate),
//   0x01056F1C live components (cap 0x100, Components.cpp), 0x0105AE30 (cap 0x20),
//   0x0105752C / 0x01057544 (cap 0x20), 0x010578A4 / 0x010578BC / 0x01057874 /
//   0x0105788C (cap 4; the IK solver registries, IKSolverDefs.cpp), 0x0105ABB8 (cap 0x20),
//   0x0105BE04 / 0x0105BE18 / 0x0105BE30 (inventory slots/groups/items, InventoryDescs.cpp),
//   0x0105C8E0 (0x00765E7C) and 0x0105CB80 (0x0074E97C); it also clears the zero
//   vector 0x01067AD0 and sets the NaN "unset" float 0x01056F00.
// NOTE: "UI.Binocucom.Radar.EnemyColor" is registered twice (0x0105A1CC and
// 0x0105B730) with different alphas; both are kept.

ape::TuningInt g_debugDisplayCharAnim("Debug.Display.CharAnim", 0);  // 010578D4

ape::TuningBool g_animationNoBlend("Animation.NoBlend", false);  // 0105AE48

ape::TuningInt g_characterMoveSpecialBallMode("Entity.Character.Move.Special.Ball.Mode", 1);  // 0105AE98
ape::TuningFloat g_characterMoveSpecialBallAccel("Entity.Character.Move.Special.Ball.Accel", 12.0f);  // 0105AEE8
ape::TuningFloat g_characterMoveSpecialBallDampingA("Entity.Character.Move.Special.Ball.DampingA", 21.0f);  // 0105AF38
ape::TuningFloat g_characterMoveSpecialBallFriction("Entity.Character.Move.Special.Ball.Friction", 1.0f);  // 0105AF88
ape::TuningFloat g_characterMoveSpecialBallGravity("Entity.Character.Move.Special.Ball.Gravity", 1.0f);  // 0105AFD8
ape::TuningFloat g_characterMoveSpecialBallJoyMin("Entity.Character.Move.Special.Ball.JoyMin", 0.3f);  // 0105B028
ape::TuningFloat g_characterMoveSpecialBallMass("Entity.Character.Move.Special.Ball.Mass", 50.0f);  // 0105B078
ape::TuningFloat g_characterMoveSpecialBallRestitution("Entity.Character.Move.Special.Ball.Restitution", 0.15f);  // 0105B0C8
ape::TuningFloat g_characterMoveSpecialBallSpeedMax("Entity.Character.Move.Special.Ball.SpeedMax", 9.0f);  // 0105B118
ape::TuningVec3 g_characterMoveSpecialBallTurnParams("Entity.Character.Move.Special.Ball.TurnParams", 360.0f, 0.0f, 0.5f);  // 0105B170
ape::TuningFloat g_characterMoveSpecialBallWalkSpeedMin("Entity.Character.Move.Special.Ball.WalkSpeedMin", 0.35f);  // 0105B1D0
ape::TuningFloat g_characterMoveSpecialBallWalkAnimSpeed("Entity.Character.Move.Special.Ball.WalkAnimSpeed", 2.0f);  // 0105B220
ape::TuningFloat g_characterMoveSpecialBallRunSpeedMin("Entity.Character.Move.Special.Ball.RunSpeedMin", 4.5f);  // 0105B270
ape::TuningFloat g_characterMoveSpecialBallRunAnimSpeed("Entity.Character.Move.Special.Ball.RunAnimSpeed", 7.0f);  // 0105B2C0
ape::TuningFloat g_characterMoveSpecialBallAnimTimeMin("Entity.Character.Move.Special.Ball.AnimTimeMin", 0.35f);  // 0105B310
ape::TuningVec3 g_characterMoveSpecialBallCharOffset("Entity.Character.Move.Special.Ball.CharOffset", 0.0f, -0.05f, 0.25f);  // 0105B360
ape::TuningFloat g_characterMoveSpecialBallImpactDamage("Entity.Character.Move.Special.Ball.ImpactDamage", 50.0f);  // 0105B3C0
ape::TuningFloat g_characterMoveSpecialBallImpactImpulse("Entity.Character.Move.Special.Ball.ImpactImpulse", 500.0f);  // 0105B410
ape::TuningFloat g_characterMoveSpecialBallSFXImpactSpeedMin("Entity.Character.Move.Special.Ball.SFXImpactSpeedMin", 4.5f);  // 0105B460
ape::TuningFloat g_characterMoveSpecialBallSFXRollSpeedMin("Entity.Character.Move.Special.Ball.SFXRollSpeedMin", 0.3f);  // 0105B4B0
ape::TuningFloat g_characterMoveSpecialBallSFXRollPOffset("Entity.Character.Move.Special.Ball.SFXRollPOffset", -4000.0f);  // 0105B500
ape::TuningFloat g_characterMoveSpecialBallSFXRollPFactor("Entity.Character.Move.Special.Ball.SFXRollPFactor", 100.0f);  // 0105B550
ape::TuningFloat g_characterMoveSpecialBallColOffsetMax("Entity.Character.Move.Special.Ball.ColOffsetMax", 0.5f);  // 0105B5A0

ape::TuningHash g_binocucomDefaultShowSFX("EntityComp.Binocucom.DefaultShowSFX", 0x858D68EEu);  // 01057D94
ape::TuningHash g_binocucomDefaultHideSFX("EntityComp.Binocucom.DefaultHideSFX", 0x2C1FB420u);  // 01057DE0
ape::TuningHash g_binocucomDefaultAmbientSFX("EntityComp.Binocucom.DefaultAmbientSFX", 0x162D30C9u);  // 01057E2C
ape::TuningHash g_binocucomDefaultZoomInSFX("EntityComp.Binocucom.DefaultZoomInSFX", 0x57DBF322u);  // 01057E78
ape::TuningHash g_binocucomDefaultZoomOutSFX("EntityComp.Binocucom.DefaultZoomOutSFX", 0x57DBF322u);  // 01057EC4
ape::TuningHash g_binocucomDefaultPhotoSFX("EntityComp.Binocucom.DefaultPhotoSFX", 0xA31258A3u);  // 01057F10
ape::TuningHash g_binocucomDefault3DPhotoAnimGeo("EntityComp.Binocucom.Default3DPhotoAnimGeo", 0xF2C2E504u);  // 01057F5C
ape::TuningHash g_binocucomDefault3DPhotoAnim("EntityComp.Binocucom.Default3DPhotoAnim", 0x1EAC11B6u);  // 01057FA8
ape::TuningString g_binocucomDefaultUIParentGroup("EntityComp.Binocucom.DefaultUIParentGroup", "InGame.Binocucom01");  // 01057FF4
ape::TuningString g_binocucomDefaultNGReticul("EntityComp.Binocucom.DefaultNGReticul", "InGame.Binocucom01.Reticle_NG");  // 010580BC
ape::TuningString g_binocucomDefaultPartialReticle("EntityComp.Binocucom.DefaultPartialReticle", "InGame.Binocucom01.Reticle_Partial");  // 01058184
ape::TuningString g_binocucomDefaultFullReticle("EntityComp.Binocucom.DefaultFullReticle", "InGame.Binocucom01.Reticle_Full");  // 0105824C
ape::TuningString g_binocucomDefaultReticleTitle("EntityComp.Binocucom.DefaultReticleTitle", "InGame.Binocucom01.Reticle_Full.Target_Name");  // 01058314
ape::TuningString g_binocucomDefaultPhotoParentGroup("EntityComp.Binocucom.DefaultPhotoParentGroup", "InGame.BinocucomPhoto");  // 010583DC
ape::TuningString g_binocucomDefaultPhotoImage("EntityComp.Binocucom.DefaultPhotoImage", "InGame.BinocucomPhoto.Polaroid.Image");  // 010584A4
ape::TuningString g_binocucomDefaultPhotoTitle("EntityComp.Binocucom.DefaultPhotoTitle", "InGame.BinocucomPhoto.Polaroid.Label");  // 0105856C
ape::TuningString g_binocucomDefaultPhotoDate("EntityComp.Binocucom.DefaultPhotoDate", "InGame.BinocucomPhoto.Polaroid.Date");  // 01058634
ape::TuningString g_binocucomDefaultTarget("EntityComp.Binocucom.DefaultTarget", "InGame.Binocucom01.TargetList.Text");  // 01058748
ape::TuningString g_binocucomDefaultCheckMark("EntityComp.Binocucom.DefaultCheckMark", "InGame.Binocucom01.TargetList.CheckMark");  // 01058810
ape::TuningString g_binocucomDefaultCheckBox("EntityComp.Binocucom.DefaultCheckBox", "InGame.Binocucom01.TargetList.CheckBox");  // 010588D8
ape::TuningHash g_binocucomDefault2DPhotoStartAnim("EntityComp.Binocucom.Default2DPhotoStartAnim", 0xF7D9606Bu);  // 010589A0
ape::TuningHash g_binocucomDefault2DPhotoEndAnim("EntityComp.Binocucom.Default2DPhotoEndAnim", 0x635A9724u);  // 010589EC
ape::TuningFloat g_binocucomDefaultPhotoDisplayTime("EntityComp.Binocucom.DefaultPhotoDisplayTime", 2.0f);  // 010586FC
ape::TuningString g_binocucomDefaultHelpText("EntityComp.Binocucom.DefaultHelpText", "InGame.3DElements.Binocucom.3DText");  // 01058A38
ape::TuningString g_binocucomDefaultHelpTextSecondary("EntityComp.Binocucom.DefaultHelpTextSecondary", "InGame.3DElements.Binocucom.3DTextSecondary");  // 01058B00
ape::TuningHash g_binocucomDefaultInGameShowScript("EntityComp.Binocucom.DefaultInGameShowScript", 0xAEEEE582u);  // 01058BC8
ape::TuningHash g_binocucomDefaultCinematicShowScript("EntityComp.Binocucom.DefaultCinematicShowScript", 0xAEEEE582u);  // 01058C14
ape::TuningHash g_binocucomDefaultTutorialShowScript("EntityComp.Binocucom.DefaultTutorialShowScript", 0x3BDE797Bu);  // 01058C60
ape::TuningHash g_binocucomDefaultInGameHideScript("EntityComp.Binocucom.DefaultInGameHideScript", 0xAEE9E7C7u);  // 01058CAC
ape::TuningHash g_binocucomDefaultCinematicHideScript("EntityComp.Binocucom.DefaultCinematicHideScript", 0xAEE9E7C7u);  // 01058CF8
ape::TuningHash g_binocucomDefaultTutorialHideScript("EntityComp.Binocucom.DefaultTutorialHideScript", 0x70F134D6u);  // 01058D44
ape::TuningHash g_binocucomDefaultShowLCharScript("EntityComp.Binocucom.DefaultShowLCharScript", 0x1ECC8CA4u);  // 01058D90
ape::TuningHash g_binocucomDefaultShowRCharScript("EntityComp.Binocucom.DefaultShowRCharScript", 0xBB1B68FFu);  // 01058DDC
ape::TuningHash g_binocucomDefaultShowStaticScript("EntityComp.Binocucom.DefaultShowStaticScript", 0xAA9F03CBu);  // 01058E28
ape::TuningHash g_binocucomDefaultStaticScript("EntityComp.Binocucom.DefaultStaticScript", 0xAF6E47B3u);  // 01058E74
ape::TuningHash g_binocucomDefaultAlertScript("EntityComp.Binocucom.DefaultAlertScript", 0x2DF1CF57u);  // 01058EC0
ape::TuningHash g_binocucomDefaultFullReticleScript("EntityComp.Binocucom.DefaultFullReticleScript", 0x3BCAF3E3u);  // 01058F0C
ape::TuningHash g_binocucomDefaultPartialReticleScript("EntityComp.Binocucom.DefaultPartialReticleScript", 0xE5D56D95u);  // 01058F58
ape::TuningHash g_binocucomDefaultCenteredNGReticleScript("EntityComp.Binocucom.DefaultCenteredNGReticleScript", 0x307D7DC5u);  // 01058FA4
ape::TuningColor g_binocucomVisionNormalColorTint("EntityComp.Binocucom.Vision.Normal.colorTint", { 255, 255, 255, 255 });  // 01058FF0 NOTE: console global 0x01164CA0, assumed white
ape::TuningFloat g_binocucomVisionNormalVisionOffset("EntityComp.Binocucom.Vision.Normal.visionOffset", 0.0f);  // 0105903C
ape::TuningFloat g_binocucomVisionNormalScanLineFreq("EntityComp.Binocucom.Vision.Normal.scanLineFreq", 1100.0f);  // 01059088
ape::TuningBool g_binocucomVisionNormalMono("EntityComp.Binocucom.Vision.Normal.mono", false);  // 010590D4
ape::TuningBool g_binocucomVisionNormalBloom("EntityComp.Binocucom.Vision.Normal.bloom", false);  // 01059120
ape::TuningColor g_binocucomVisionThermalColorTint("EntityComp.Binocucom.Vision.Thermal.colorTint", { 0, 128, 255, 255 });  // 0105916C
ape::TuningFloat g_binocucomVisionThermalVisionOffset("EntityComp.Binocucom.Vision.Thermal.visionOffset", 0.0f);  // 010591B8
ape::TuningFloat g_binocucomVisionThermalScanLineFreq("EntityComp.Binocucom.Vision.Thermal.scanLineFreq", 840.0f);  // 01059204
ape::TuningBool g_binocucomVisionThermalMono("EntityComp.Binocucom.Vision.Thermal.mono", true);  // 01059250
ape::TuningBool g_binocucomVisionThermalBloom("EntityComp.Binocucom.Vision.Thermal.bloom", true);  // 0105929C

ape::TuningFloat g_uiBinocucomZoomArrowMaxU("UI.Binocucom.ZoomArrowMaxU", 0.0f);  // 010592FC
ape::TuningFloat g_uiBinocucomZoomArrowMinU("UI.Binocucom.ZoomArrowMinU", -0.87f);  // 01059348
ape::TuningFloat g_uiBinocucomZoomBarMaxU("UI.Binocucom.ZoomBarMaxU", 0.0f);  // 01059394
ape::TuningFloat g_uiBinocucomZoomBarMinU("UI.Binocucom.ZoomBarMinU", 0.5f);  // 010593E0
ape::TuningInt g_uiBinocucomMaxLines("UI.Binocucom.MaxLines", 3);  // 0105942C
ape::TuningFloat g_uiBinocucomMax2DReticleDistance("UI.Binocucom.Max2DReticleDistance", 0.075f);  // 01059478
ape::TuningColor g_uiBinocucomNearReticleColor("UI.Binocucom.NearReticleColor", { 255, 200, 0, 225 });  // 010594C4
ape::TuningColor g_uiBinocucomFarReticleColor("UI.Binocucom.FarReticleColor", { 255, 200, 0, 225 });  // 01059510
ape::TuningFloat g_uiBinocucomTargetListSpacing("UI.Binocucom.TargetListSpacing", 12.0f);  // 0105955C
ape::TuningFloat g_uiBinocucomTargetListYAdjust("UI.Binocucom.TargetListYAdjust", 0.5f);  // 010595A8
ape::TuningVec3 g_uiBinocucomScreenCenterOffset("UI.Binocucom.ScreenCenterOffset", 0.0f, -27.0f, 0.0f);  // 01059600
ape::TuningFloat g_uiBinocucomTargetAngleWeight("UI.Binocucom.TargetAngleWeight", 1.0f);  // 01059660
ape::TuningFloat g_uiBinocucomTargetDistanceWeight("UI.Binocucom.TargetDistanceWeight", -0.01f);  // 010596AC
ape::TuningFloat g_uiBinocucomDisplayRateFudgeFactor("UI.Binocucom.DisplayRateFudgeFactor", 0.5f);  // 010596F8
ape::TuningString g_uiBinocucomCtrlHintTakePhotoStr("UI.Binocucom.CtrlHintTakePhotoStr", "$PhotoOpPicture_ControlHintTakePhoto");  // 01059744
ape::TuningString g_uiBinocucomCtrlHintZoomInStr("UI.Binocucom.CtrlHintZoomInStr", "$PhotoOpPicture_ControlHintZoomIn");  // 0105980C
ape::TuningString g_uiBinocucomCtrlHintZoomOutStr("UI.Binocucom.CtrlHintZoomOutStr", "$PhotoOpPicture_ControlHintZoomOut");  // 010598D4
ape::TuningString g_uiBinocucomCtrlHintMoveStr("UI.Binocucom.CtrlHintMoveStr", "$PhotoOpPicture_ControlHintMove");  // 0105999C
ape::TuningString g_uiBinocucomPhotoErrorZoomInStr("UI.Binocucom.PhotoErrorZoomInStr", "$PhotoOpPicture_PhotoErrorZoomIn");  // 01059A64
ape::TuningString g_uiBinocucomPhotoErrorZoomOutStr("UI.Binocucom.PhotoErrorZoomOutStr", "$PhotoOpPicture_PhotoErrorZoomOut");  // 01059B2C
ape::TuningString g_uiBinocucomPhotoErrorBadFramingStr("UI.Binocucom.PhotoErrorBadFramingStr", "$PhotoOpPicture_PhotoErrorBadFraming");  // 01059BF4
ape::TuningString g_uiBinocucomPhotoErrorNoItemsInViewStr("UI.Binocucom.PhotoErrorNoItemsInViewStr", "$PhotoOpPicture_PhotoErrorNoItemsInView");  // 01059CBC
ape::TuningString g_uiBinocucomCtrlHintShootStr("UI.Binocucom.CtrlHintShootStr", "$BUTTON_R1_ANIM $STR_SHOOT");  // 01059D84
ape::TuningFloat g_uiBinocucomCtrlHintAlphaSpeed("UI.Binocucom.CtrlHintAlphaSpeed", 2.0f);  // 01059E4C

ape::TuningFloat g_binocucomPhotoPowerLossFromDamage("EntityComp.Binocucom.PhotoPowerLossFromDamage", 5.0f);  // 01059E98
ape::TuningHash g_binocucomPhotoPowerDrainedScript("EntityComp.Binocucom.PhotoPowerDrainedScript", 0xCF18C5D9u);  // 01059EE4
ape::TuningHash g_binocucomActivationNoPowerScript("EntityComp.Binocucom.ActivationNoPowerScript", 0x86112067u);  // 01059F30

ape::TuningBool g_uiBinocucomUsePhotoError("UI.Binocucom.UsePhotoError", false);  // 01059F7C

ape::TuningFloat g_binocucomDefaultHelpTextSecondaryYOffset("EntityComp.Binocucom.DefaultHelpTextSecondaryYOffset", -0.5f);  // 01059FC8

ape::TuningBool g_uiBinocucomRadarEnabled("UI.Binocucom.Radar.Enabled", false);  // 0105A014
ape::TuningVec3 g_uiBinocucomRadarOrigin("UI.Binocucom.Radar.Origin", 4.1f, -1.9f, 2.0f);  // 0105A060
ape::TuningVec3 g_uiBinocucomRadarBasisX("UI.Binocucom.Radar.BasisX", 0.9f, 0.0f, 0.0f);  // 0105A0C0
ape::TuningVec3 g_uiBinocucomRadarBasisY("UI.Binocucom.Radar.BasisY", 0.0f, 0.7f, -0.25f);  // 0105A120
ape::TuningFloat g_uiBinocucomRadarWorldRadius("UI.Binocucom.Radar.WorldRadius", 50.0f);  // 0105A180
ape::TuningColor g_uiBinocucomRadarEnemyColor("UI.Binocucom.Radar.EnemyColor", { 255, 0, 0, 128 });  // 0105A1CC
ape::TuningColor g_uiBinocucomRadarGoalColor("UI.Binocucom.Radar.GoalColor", { 0, 0, 255, 92 });  // 0105A218
ape::TuningBool g_uiBinocucomRadarDebugOutlineRegion("UI.Binocucom.Radar.DebugOutlineRegion", false);  // 0105A264
ape::TuningBool g_uiBinocucomRadarDebugDrawAxes("UI.Binocucom.Radar.DebugDrawAxes", false);  // 0105A2B0
ape::TuningBool g_uiBinocucomRadarPulseGoalBlipsByDistance("UI.Binocucom.Radar.PulseGoalBlipsByDistance", true);  // 0105A2FC
ape::TuningFloat g_uiBinocucomRadarMinGoalPulseSeconds("UI.Binocucom.Radar.MinGoalPulseSeconds", 0.2f);  // 0105A348
ape::TuningFloat g_uiBinocucomRadarMaxGoalPulseSeconds("UI.Binocucom.Radar.MaxGoalPulseSeconds", 1.5f);  // 0105A394
ape::TuningFloat g_uiBinocucomRadarGoalPulseSecondPerMeter("UI.Binocucom.Radar.GoalPulseSecondPerMeter", 0.02f);  // 0105A3E0
ape::TuningBool g_uiBinocucomRadarFadeRadarEnemies("UI.Binocucom.Radar.FadeRadarEnemies", true);  // 0105A42C
ape::TuningFloat g_uiBinocucomRadarStartEnemyFadeDistance("UI.Binocucom.Radar.StartEnemyFadeDistance", 45.0f);  // 0105A478
ape::TuningFloat g_uiBinocucomRadarBlipSize("UI.Binocucom.Radar.BlipSize", 0.125f);  // 0105A4C4
ape::TuningFloat g_uiBinocucomRadarSweepFXAlphaDecay("UI.Binocucom.Radar.SweepFXAlphaDecay", 1.0f);  // 0105A510
ape::TuningBool g_uiBinocucomRadarDebugDrawSweep("UI.Binocucom.Radar.DebugDrawSweep", false);  // 0105A55C
ape::TuningFloat g_uiBinocucomRadarSweepAnimAngleOffset("UI.Binocucom.Radar.SweepAnimAngleOffset", -0.25f);  // 0105A5A8
ape::TuningHash g_uiBinocucomRadarSweepAnimGeo("UI.Binocucom.Radar.SweepAnimGeo", 0xACD51374u);  // 0105A5F4
ape::TuningBool g_uiBinocucomRadarEnableEnemySweepFX("UI.Binocucom.Radar.EnableEnemySweepFX", true);  // 0105A640
ape::TuningBool g_uiBinocucomRadarEnableOnScreenGoalSweepFX("UI.Binocucom.Radar.EnableOnScreenGoalSweepFX", true);  // 0105A68C
ape::TuningBool g_uiBinocucomRadarEnableOffScreenGoalSweepFX("UI.Binocucom.Radar.EnableOffScreenGoalSweepFX", false);  // 0105A6D8

ape::TuningVec3 g_binocucomReticleOffset("EntityComp.Binocucom.ReticleOffset", 0.0f, 0.07f, 5.0f);  // 0105A730
ape::TuningHash g_binocucomPowerMeterHash("EntityComp.Binocucom.PowerMeter.Hash", 0xDF234965u);  // 0105A790
ape::TuningHash g_binocucomPowerMeterIdleAnimHash("EntityComp.Binocucom.PowerMeter.IdleAnimHash", 0xC68EFD5Eu);  // 0105A7DC
ape::TuningHash g_binocucomPowerMeterPulseAnimHash("EntityComp.Binocucom.PowerMeter.PulseAnimHash", 0x0BBB0F2Fu);  // 0105A828

ape::TuningHash g_uiBinocucomRadarBlipPlayer("UI.Binocucom.Radar.BlipPlayer", 0x9D5850F6u);  // 0105A874
ape::TuningHash g_uiBinocucomRadarBlipEnemy("UI.Binocucom.Radar.BlipEnemy", 0x6A296A46u);  // 0105A8C0
ape::TuningHash g_uiBinocucomRadarBlipGoal("UI.Binocucom.Radar.BlipGoal", 0x6A3CBC47u);  // 0105A90C
ape::TuningHash g_uiBinocucomRadarBlipGoalEdge("UI.Binocucom.Radar.BlipGoalEdge", 0x9D5850F6u);  // 0105A958
ape::TuningHash g_uiBinocucomRadarBlipHideout("UI.Binocucom.Radar.BlipHideout", 0x8B0ED9A6u);  // 0105A9A4
ape::TuningHash g_uiBinocucomRadarBlipVision("UI.Binocucom.Radar.BlipVision", 0xC9D26DAEu);  // 0105A9F0
ape::TuningHash g_uiBinocucomRadarBlipBottle("UI.Binocucom.Radar.BlipBottle", 0xA806AB8Cu);  // 0105AA3C
ape::TuningHash g_uiBinocucomRadarBlipSafe("UI.Binocucom.Radar.BlipSafe", 0x07658ED3u);  // 0105AA88
ape::TuningFloat g_uiBinocucomRadarAlphaWorldYMin("UI.Binocucom.Radar.Alpha.WorldYMin", 2.85f);  // 0105B5F0
ape::TuningFloat g_uiBinocucomRadarAlphaWorldYMax("UI.Binocucom.Radar.Alpha.WorldYMax", 14.0f);  // 0105B640
ape::TuningFloat g_uiBinocucomRadarAlphaMin("UI.Binocucom.Radar.Alpha.Min", 0.05f);  // 0105B690
ape::TuningFloat g_uiBinocucomRadarAlphaMax("UI.Binocucom.Radar.Alpha.Max", 1.0f);  // 0105B6E0
ape::TuningColor g_uiBinocucomRadarEnemyColor2("UI.Binocucom.Radar.EnemyColor", { 255, 0, 0, 255 });  // 0105B730
ape::TuningColor g_uiBinocucomRadarBottleColor("UI.Binocucom.Radar.BottleColor", { 87, 169, 95, 255 });  // 0105B780
ape::TuningColor g_uiBinocucomRadarSafeColor("UI.Binocucom.Radar.SafeColor", { 255, 191, 0, 255 });  // 0105B7D0

ape::TuningInt g_debugDisplayBuoyancy("Debug.Display.Buoyancy", 0);  // 0105AAD4
ape::TuningInt g_debugDisplayCombat("Debug.Display.Combat", 0);  // 0105B820

ape::TuningBool g_combatAttackOnHold("EntityComp.Combat.AttackOnHold", false);  // 0105B870
ape::TuningInt g_combatMode("EntityComp.Combat.Mode", 11);  // 0105B8C0
ape::TuningFloat g_combatFreezeScalar("EntityComp.Combat.FreezeScalar", 0.15f);  // 0105B910
ape::TuningFloat g_combatFreezeDuration("EntityComp.Combat.FreezeDuration", 1.5f);  // 0105B960
ape::TuningFloat g_combatFreezeTimeMin("EntityComp.Combat.FreezeTimeMin", 8.0f);  // 0105B9B0
ape::TuningFloat g_combatWallAngleMin("EntityComp.Combat.WallAngleMin", 60.0f);  // 0105BA00
ape::TuningInt g_combatRangeBBoxCache("EntityComp.Combat.RangeBBoxCache", 1);  // 0105BA50
ape::TuningColor g_combatDebugColor0("EntityComp.Combat.DebugColor0", { 136, 192, 120, 80 });  // 0105BAA0
ape::TuningColor g_combatDebugColor1("EntityComp.Combat.DebugColor1", { 160, 56, 52, 176 });  // 0105BAEC
ape::TuningColor g_combatDebugColor2("EntityComp.Combat.DebugColor2", { 208, 144, 48, 208 });  // 0105BB38

ape::TuningFloat g_contextualMarkerScaleMin("EntityComp.ContextualMarker.ScaleMin", 1.0f);  // 0105755C
ape::TuningFloat g_contextualMarkerScaleMax("EntityComp.ContextualMarker.ScaleMax", 2.2f);  // 010575A8
ape::TuningFloat g_contextualMarkerScaleDistMin("EntityComp.ContextualMarker.ScaleDistMin", 15.0f);  // 010575F4
ape::TuningFloat g_contextualMarkerScaleDistMax("EntityComp.ContextualMarker.ScaleDistMax", 25.0f);  // 01057640
ape::TuningFloat g_contextualMarkerScaleFactorFOV("EntityComp.ContextualMarker.ScaleFactorFOV", 0.0f);  // 0105768C

ape::TuningFloat g_controlExtJumpVel("EntityComp.ControlExt.JumpVel", 9.25f);  // 0105BB88

ape::TuningBool g_fxDeferSync("EntityComp.FX.DeferSync", true);  // 0105BBD8
ape::TuningFloat g_fxSlowTimeFadeTime("EntityComp.FX.SlowTime.FadeTime", 0.5f);  // 0105BC28
ape::TuningFloat g_fxSlowTimeTrailLen("EntityComp.FX.SlowTime.TrailLen", 1.5f);  // 0105BC78
ape::TuningColor g_fxSlowTimeTrailColor("EntityComp.FX.SlowTime.TrailColor", { 141, 105, 51, 255 });  // 0105BCC8
ape::TuningFloat g_fxSlowTimeTrailAlphaSpeed("EntityComp.FX.SlowTime.TrailAlphaSpeed", 85.0f);  // 0105BD18

ape::TuningBool g_animationIKLookAtDisable("Animation.IK.LookAt.Disable", false);  // 0105BD68
ape::TuningFloat g_animationIKLookAtAngleMax("Animation.IK.LookAt.AngleMax", 120.0f);  // 0105BDB8

ape::TuningBool g_debugDisplayInteractComp("Debug.Display.InteractComp", false);  // 0105AB6C

ape::TuningFloat g_inventoryStartScale("EntityComp.Inventory.StartScale", 0.0f);  // 0105BE48
ape::TuningFloat g_inventoryEndScale("EntityComp.Inventory.EndScale", 1.0f);  // 0105BE98
ape::TuningFloat g_inventoryScaleInterpRate("EntityComp.Inventory.ScaleInterpRate", 0.075f);  // 0105BEE8
ape::TuningFloat g_inventoryCoinIncreasePickpocketPercent("EntityComp.Inventory.CoinIncrease.PickpocketPercent", 1.5f);  // 0105BF38
ape::TuningFloat g_inventoryCoinIncreaseDeathPercent("EntityComp.Inventory.CoinIncrease.DeathPercent", 1.5f);  // 0105BF88
ape::TuningInt g_inventoryPickpocketAward1Num("EntityComp.Inventory.Pickpocket.Award1Num", 22);  // 0105BFD8
ape::TuningInt g_inventoryPickpocketAward1NumDefs("EntityComp.Inventory.Pickpocket.Award1NumDefs", 10);  // 0105C028
ape::TuningInt g_inventoryPickpocketAward2Num("EntityComp.Inventory.Pickpocket.Award2Num", 45);  // 0105C078
ape::TuningInt g_inventoryPickpocketAward2NumDefs("EntityComp.Inventory.Pickpocket.Award2NumDefs", 18);  // 0105C0C8
ape::TuningFloat g_inventoryPickpocketNoCollectTime("EntityComp.Inventory.Pickpocket.NoCollectTime", 0.4f);  // 0105C118

ape::TuningInt g_debugDisplayJumpRope("Debug.Display.JumpRope", 0);  // 0105AB20

ape::TuningBool g_monsterClosetKillMonsters("Entity.MonsterCloset.KillMonsters", false);  // 0105C168

ape::TuningInt g_debugDisplayCharPhysics("Debug.Display.CharPhysics", 0);  // 010572C4

ape::TuningFloat g_physicsGravityScalar("EntityComp.Physics.GravityScalar", 2.25f);  // 01057310
ape::TuningBool g_physicsRaycastEnable("EntityComp.Physics.RaycastEnable", true);  // 0105735C
ape::TuningFloat g_physicsInputGroundGravityEffect("EntityComp.Physics.Input.GroundGravityEffect", 0.0f);  // 01056F34
ape::TuningFloat g_physicsInputGroundFrictionStatic("EntityComp.Physics.Input.GroundFrictionStatic", 50.0f);  // 01056F80
ape::TuningFloat g_physicsInputGroundFrictionDyn("EntityComp.Physics.Input.GroundFrictionDyn", 1.5f);  // 01056FCC
ape::TuningFloat g_physicsInputAirFrictionXZ("EntityComp.Physics.Input.AirFrictionXZ", 0.125f);  // 01057018
ape::TuningFloat g_physicsInputAirFrictionY("EntityComp.Physics.Input.AirFrictionY", 0.0f);  // 01057064
ape::TuningFloat g_physicsInputAirRayRange("EntityComp.Physics.Input.AirRayRange", 10.0f);  // 010570B0
ape::TuningFloat g_physicsInputGroundSlopeMax("EntityComp.Physics.Input.GroundSlopeMax", 40.0f);  // 010570FC
ape::TuningFloat g_physicsInputWallSlopeMin("EntityComp.Physics.Input.WallSlopeMin", 65.0f);  // 01057148
ape::TuningFloat g_physicsInputImpactDamageScalarObj("EntityComp.Physics.Input.ImpactDamageScalarObj", 0.1f);  // 01057194
ape::TuningFloat g_physicsInputImpactDamageScalarChar("EntityComp.Physics.Input.ImpactDamageScalarChar", 1.0f);  // 010571E0
ape::TuningFloat g_physicsInputImpactDamageSpeedMin("EntityComp.Physics.Input.ImpactDamageSpeedMin", 2.0f);  // 0105722C
ape::TuningFloat g_physicsInputImpactDamageVelRatioMin("EntityComp.Physics.Input.ImpactDamageVelRatioMin", 0.8f);  // 01057278
ape::TuningFloat g_physicsKeepDistance("EntityComp.Physics.KeepDistance", 0.02f);  // 0105C1B8
ape::TuningFloat g_physicsBottomRadius("EntityComp.Physics.BottomRadius", 0.45f);  // 0105C208
ape::TuningInt g_physicsGroundPhantomMode("EntityComp.Physics.GroundPhantomMode", 1);  // 0105C258
ape::TuningVec3 g_physicsGroundPhantomSize("EntityComp.Physics.GroundPhantomSize", 1.5f, 1.5f, -0.05f);  // 0105C2B0

ape::TuningVec3 g_characterMoveGroundCrawlColSize("Entity.Character.Move.Ground.Crawl.ColSize", 0.25f, 0.1f, 0.75f);  // 0105C310
ape::TuningVec3 g_characterMoveGroundCrawlColOffset("Entity.Character.Move.Ground.Crawl.ColOffset", 0.0f, -0.05f, 1.0f);  // 0105C370
ape::TuningVec3 g_characterMoveGroundPlayDeadColSize("Entity.Character.Move.Ground.PlayDead.ColSize", 0.75f, 0.85f, 0.75f);  // 0105C3D0

ape::TuningFloat g_physicsPenetrationDepth("EntityComp.Physics.PenetrationDepth", 0.0f);  // 0105C430
ape::TuningFloat g_physicsFriction("EntityComp.Physics.Friction", 0.0f);  // 0105C480
ape::TuningFloat g_physicsRestitution("EntityComp.Physics.Restitution", 0.0f);  // 0105C4D0
ape::TuningFloat g_physicsWalkRayRange("EntityComp.Physics.WalkRayRange", 0.5f);  // 0105C520
ape::TuningFloat g_physicsMaxLinearVelocity("EntityComp.Physics.MaxLinearVelocity", 30.0f);  // 0105C570
ape::TuningFloat g_physicsMaxLinearAccel("EntityComp.Physics.MaxLinearAccel", 0.5f);  // 0105C5C0
ape::TuningFloat g_physicsSupportSlopeMax("EntityComp.Physics.SupportSlopeMax", 75.0f);  // 0105C610

ape::TuningFloat g_characterEdgeGrabNormalYCosMax("Entity.Character.EdgeGrab.NormalYCosMax", 0.995f);  // 0105C660
ape::TuningInt g_characterEdgeGrabTagMode("Entity.Character.EdgeGrab.TagMode", 1);  // 0105C6B0
ape::TuningBool g_characterEdgeGrabAutoCatch("Entity.Character.EdgeGrab.AutoCatch", true);  // 0105C700
ape::TuningFloat g_characterEdgeGrabHeightMin("Entity.Character.EdgeGrab.HeightMin", 1.75f);  // 0105C750
ape::TuningBool g_characterMoveGroundNormalRayEnable("Entity.Character.Move.Ground.NormalRayEnable", true);  // 0105C7A0
ape::TuningFloat g_characterMoveGroundNormalRayRange("Entity.Character.Move.Ground.NormalRayRange", 0.5f);  // 0105C7F0

ape::TuningFloat g_physicsJumpTimeMin("EntityComp.Physics.JumpTimeMin", 0.1f);  // 0105C840
ape::TuningFloat g_physicsInputCornerPushVel("EntityComp.Physics.Input.CornerPushVel", 0.0f);  // 0105C890

ape::TuningInt g_procAnimProcMode("EntityComp.ProcAnim.ProcMode", 3);  // 0105C8E8
ape::TuningFloat g_procAnimCutSpeed("EntityComp.ProcAnim.CutSpeed", 40.0f);  // 0105C938
ape::TuningInt g_procAnimDebugLevel("EntityComp.ProcAnim.Debug.Level", 0);  // 0105C988
ape::TuningColor g_procAnimDebugColor0("EntityComp.ProcAnim.Debug.Color0", { 255, 48, 48, 255 });  // 0105C9D8
ape::TuningColor g_procAnimDebugColor1("EntityComp.ProcAnim.Debug.Color1", { 160, 112, 160, 255 });  // 0105CA24
ape::TuningColor g_procAnimDebugColor2("EntityComp.ProcAnim.Debug.Color2", { 224, 224, 128, 192 });  // 0105CA70
ape::TuningColor g_procAnimDebugColor3("EntityComp.ProcAnim.Debug.Color3", { 160, 255, 96, 64 });  // 0105CABC
ape::TuningColor g_procAnimDebugColor4("EntityComp.ProcAnim.Debug.Color4", { 240, 208, 48, 176 });  // 0105CB08

ape::TuningBool g_eyesDebugDraw("EntityComp.Eyes.DebugDraw", false);  // 01066B00
ape::TuningBool g_eyesDebugPrintFloatTrack("EntityComp.Eyes.DebugPrintFloatTrack", false);  // 01066B50

ape::TuningFloat g_radarSplineMoveSpeed("EntityComp.Radar.SplineMoveSpeed", 7.0f);  // 010579B8
ape::TuningFloat g_radarSplineMoveTimeout("EntityComp.Radar.SplineMoveTimeout", 15.0f);  // 01057A04
ape::TuningFloat g_radarSplineMoveInputWaitTime("EntityComp.Radar.SplineMoveInputWaitTime", 3.0f);  // 01057A50
ape::TuningFloat g_radarGuidanceSplnPosOffset("EntityComp.Radar.GuidanceSplnPosOffset", 13.0f);  // 01057A9C
ape::TuningFloat g_radarMaxCompassArrowRotSpeed("EntityComp.Radar.MaxCompassArrowRotSpeed", 360.0f);  // 01057AE8
ape::TuningFloat g_radarDefaultActiveDuration("EntityComp.Radar.DefaultActiveDuration", 8.0f);  // 01057920
ape::TuningFloat g_radarDefaultRefreshDuration("EntityComp.Radar.DefaultRefreshDuration", 8.0f);  // 0105796C
ape::TuningBool g_radarUsePathAlways("EntityComp.Radar.UsePathAlways", false);  // 01057B34
ape::TuningBool g_radarUsePathNever("EntityComp.Radar.UsePathNever", false);  // 01057B80
ape::TuningBool g_radarDrawForwardPaths("EntityComp.Radar.DrawForwardPaths", false);  // 01057BCC
ape::TuningBool g_radarDrawBackwardPaths("EntityComp.Radar.DrawBackwardPaths", false);  // 01057C18
ape::TuningInt g_radarAwardNum("EntityComp.Radar.AwardNum", 18);  // 01057C64
ape::TuningInt g_radarAwardNumActivate("EntityComp.Radar.AwardNumActivate", 500);  // 01057CB0
ape::TuningFloat g_radarRadarDeactivateTime("EntityComp.Radar.RadarDeactivateTime", 2.0f);  // 01057CFC
ape::TuningFloat g_radarSafeMarkerYOffset("EntityComp.Radar.SafeMarkerYOffset", 2.0f);  // 01057D48

ape::TuningBool g_receptacleDebugEnable("Entity.Receptacle.DebugEnable", false);  // 01066BA0

ape::TuningColor g_projectorAlertColor("EntityComp.Projector.AlertColor", { 255, 0, 0, 255 });  // 010576D8
ape::TuningColor g_projectorNormColor("EntityComp.Projector.NormColor", { 255, 255, 255, 255 });  // 01057724

ape::TuningInt g_debugDisplayShooting("Debug.Display.Shooting", 0);  // 01066BEC

ape::TuningFloat g_shootingCrackShotMarkerScaleMin("EntityComp.Shooting.CrackShotMarker.ScaleMin", 0.35f);  // 0105ABD0
ape::TuningFloat g_shootingCrackShotMarkerScaleMax("EntityComp.Shooting.CrackShotMarker.ScaleMax", 2.01f);  // 0105AC1C
ape::TuningFloat g_shootingCrackShotMarkerScaleDistMin("EntityComp.Shooting.CrackShotMarker.ScaleDistMin", 10.4f);  // 0105AC68
ape::TuningFloat g_shootingCrackShotMarkerScaleDistMax("EntityComp.Shooting.CrackShotMarker.ScaleDistMax", 26.4f);  // 0105ACB4
ape::TuningInt g_shootingCrackShotAwardAwardNum("EntityComp.Shooting.CrackShotAward.AwardNum", 37);  // 01066C38
ape::TuningInt g_shootingCrackShotAwardNumKills("EntityComp.Shooting.CrackShotAward.NumKills", 10);  // 01066C88
ape::TuningFloat g_shootingCrackShotAwardTime("EntityComp.Shooting.CrackShotAward.Time", 20.0f);  // 01066CD8

ape::TuningInt g_debugDisplayTargeting("Debug.Display.Targeting", 0);  // 01066D28

ape::TuningInt g_targetingMode("EntityComp.Targeting.Mode", 0);  // 01066D78

ape::TuningFloat g_characterWallClimbStickThresholdLow("Entity.Character.WallClimb.StickThresholdLow", 0.5f);  // 0105AD00
ape::TuningFloat g_characterWallClimbStickThresholdHigh("Entity.Character.WallClimb.StickThresholdHigh", 0.85f);  // 0105AD4C
ape::TuningFloat g_characterWallClimbSlideRumbleStrength("Entity.Character.WallClimb.SlideRumbleStrength", 0.7f);  // 0105AD98
ape::TuningFloat g_characterWallClimbSlideRumbleDuration("Entity.Character.WallClimb.SlideRumbleDuration", 0.5f);  // 0105ADE4

ape::TuningInt g_debugDisplayWheelchair("Debug.Display.Wheelchair", 0);  // 01066DC8

ape::TuningBool g_wheelchairDisable("EntityComp.Wheelchair.Disable", false);  // 01066E18
ape::TuningVec2 g_wheelchairWheelTrackWidth("EntityComp.Wheelchair.Wheel.TrackWidth", 0.45f, 0.51f);  // 01066E68
ape::TuningVec2 g_wheelchairWheelRadius("EntityComp.Wheelchair.Wheel.Radius", 0.35f, 0.35f);  // 01066EB8
ape::TuningVec2 g_wheelchairWheelCamber("EntityComp.Wheelchair.Wheel.Camber", 20.0f, 29.5f);  // 01066F08
ape::TuningFloat g_wheelchairWheelDeformSpeedMin("EntityComp.Wheelchair.Wheel.DeformSpeedMin", 7.5f);  // 01066F58
ape::TuningFloat g_wheelchairWheelDeformSpeedMax("EntityComp.Wheelchair.Wheel.DeformSpeedMax", 9.0f);  // 01066FA8
ape::TuningFloat g_wheelchairWheelDeformFactor("EntityComp.Wheelchair.Wheel.DeformFactor", 1.25f);  // 01066FF8
ape::TuningFloat g_wheelchairWheelDeformAngle("EntityComp.Wheelchair.Wheel.DeformAngle", 60.0f);  // 01067048
ape::TuningFloat g_wheelchairWheelDeformRange("EntityComp.Wheelchair.Wheel.DeformRange", 85.0f);  // 01067098
ape::TuningFloat g_wheelchairWheelSpinnerFriction("EntityComp.Wheelchair.Wheel.SpinnerFriction", 0.005f);  // 010670E8
ape::TuningBool g_wheelchairTrailEnable("EntityComp.Wheelchair.Trail.Enable", true);  // 01067138
ape::TuningFloat g_wheelchairTrailLength("EntityComp.Wheelchair.Trail.Length", 5.0f);  // 01067188
ape::TuningFloat g_wheelchairTrailFPS("EntityComp.Wheelchair.Trail.FPS", 30.0f);  // 010671D8
ape::TuningColor g_wheelchairTrailColor("EntityComp.Wheelchair.Trail.Color", { 64, 64, 64, 40 });  // 01067228
ape::TuningFloat g_wheelchairTrailWidth("EntityComp.Wheelchair.Trail.Width", 0.025f);  // 01067278
ape::TuningFloat g_wheelchairTrailYOffset("EntityComp.Wheelchair.Trail.YOffset", 0.005f);  // 010672C8
ape::TuningFloat g_wheelchairTrailAlphaSpeed("EntityComp.Wheelchair.Trail.AlphaSpeed", 5.0f);  // 01067318
ape::TuningFloat g_wheelchairArmGainInterp("EntityComp.Wheelchair.Arm.GainInterp", 0.15f);  // 01067368
ape::TuningFloat g_wheelchairArmBoneLengthMax("EntityComp.Wheelchair.Arm.BoneLengthMax", 1.5f);  // 010673B8
ape::TuningFloat g_wheelchairArmSolverFPS("EntityComp.Wheelchair.Arm.SolverFPS", 240.0f);  // 01067408
ape::TuningFloat g_wheelchairArmSpringL("EntityComp.Wheelchair.Arm.SpringL", 500.0f);  // 01067458
ape::TuningFloat g_wheelchairArmSpringA("EntityComp.Wheelchair.Arm.SpringA", 510.0f);  // 010674A8
ape::TuningFloat g_wheelchairArmDampingA("EntityComp.Wheelchair.Arm.DampingA", 0.05f);  // 010674F8
ape::TuningFloat g_wheelchairArmDampingW("EntityComp.Wheelchair.Arm.DampingW", 0.0f);  // 01067548
ape::TuningFloat g_wheelchairArmBoundRadius("EntityComp.Wheelchair.Arm.BoundRadius", 7.5f);  // 01067598
ape::TuningInt g_wheelchairRemoteMode("EntityComp.Wheelchair.Remote.Mode", 1);  // 010675E8
ape::TuningFloat g_wheelchairRemoteTrigTimeMin("EntityComp.Wheelchair.Remote.TrigTimeMin", 0.35f);  // 01067638
ape::TuningVec3 g_wheelchairRemoteOffset("EntityComp.Wheelchair.Remote.Offset", 0.0f, 0.0f, 0.0f);  // 01067690
ape::TuningVec2 g_wheelchairRemoteVel("EntityComp.Wheelchair.Remote.Vel", 4.5f, 4.5f);  // 010676F0
ape::TuningVec2 g_wheelchairRemoteAVel("EntityComp.Wheelchair.Remote.AVel", -360.0f, 360.0f);  // 01067740
ape::TuningVec2 g_wheelchairRemoteAngleX("EntityComp.Wheelchair.Remote.AngleX", 35.0f, 35.0f);  // 01067790
ape::TuningVec2 g_wheelchairRemoteAngleY("EntityComp.Wheelchair.Remote.AngleY", 70.0f, 70.0f);  // 010677E0
ape::TuningFloat g_wheelchairRemoteStickDistMax("EntityComp.Wheelchair.Remote.StickDistMax", 3.0f);  // 01067830
ape::TuningFloat g_wheelchairRemoteUIDisplayTime("EntityComp.Wheelchair.Remote.UIDisplayTime", 0.0f);  // 01067880
ape::TuningFloat g_wheelchairShieldRadius("EntityComp.Wheelchair.Shield.Radius", 2.5f);  // 010678D0
ape::TuningFloat g_wheelchairShieldObjSize("EntityComp.Wheelchair.Shield.ObjSize", 1.65f);  // 01067920
ape::TuningVec3 g_wheelchairShieldOffset("EntityComp.Wheelchair.Shield.Offset", 0.0f, 1.0f, 0.25f);  // 01067970
ape::TuningVec3 g_wheelchairShieldPowerCost("EntityComp.Wheelchair.Shield.PowerCost", 100.0f, 10.0f, 50.0f);  // 010679D0
ape::TuningInt g_wheelchairRemoteBombPocketAwardNum("EntityComp.Wheelchair.Remote.BombPocket.AwardNum", 34);  // 01067A30
ape::TuningInt g_wheelchairRemoteBombPocketAwardNumBombs("EntityComp.Wheelchair.Remote.BombPocket.AwardNumBombs", 60);  // 01067A80

} // namespace components
} // namespace sly
