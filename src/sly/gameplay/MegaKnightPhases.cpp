// openTIT - Mega knight phases (0x003044B0, 0x00304654-0x00304C14,
// 0x00312118-0x0031AEF0).
#include "sly/gameplay/MegaKnight.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeInstances.h"
#include "sly/entity/GameCamera.h"
#include "sly/game/GameScripts.h"
#include "sly/gameplay/MegaKnightHooks.h"
#include "sly/gameplay/Sequencer.h"
#include "sly/tuning/MegaKnightDragonTuning.h"

#include <cmath>
#include <cstring>

namespace sly {
namespace gameplay {

namespace {

// Object variable of the phase scripts that refers to the boss.
constexpr uint32_t kScriptVar_Boss = 0x0027469Eu;
constexpr uint32_t kScriptState_Spawn = 0x04B6FB5Bu;   // "Spawn"

// State names of the phases (string tables 0x00AD9970..0x00AD9B6C).
const char* const kIdleStates[] = {"None", "Idle", "Done"};
const char* const kLaserStates[] = {"None", "Intro", "Loop", "EndLaser", "Exit", "Done"};
const char* const kLaserQuickStates[] = {"None", "Intro", "Loop", "LaserEnd", "Exit", "Done"};
const char* const kBlockShotStates[] = {"None", "Swat", "Done"};
const char* const kMonitorStates[] = {"None", "Idle_Start", "Idle_Loop", "Idle_End", "Done"};
const char* const kShootStates[] = {"None", "LaserCharge", "LaserShoot", "LaserEnd", "Done"};
const char* const kRopeStates[] = {"None", "CutRope_Start", "CutRope_Sweep", "CutRope_Sweep_LaserEnd",
                                   "CutRope_End", "Done"};
const char* const kGroundSmashStates[] = {"None", "Start", "Repeat", "End", "Done"};
const char* const kShoulderHitStates[] = {"None", "Hit", "Fail", "Done"};
const char* const kStruggleStates[] = {"None", "Idle", "Done"};
const char* const kTargetHitStates[] = {"None", "Idle", "IdleDone", "Done"};
const char* const kTowerShakeStates[] = {"None", "Start", "IGC", "End", "Done"};

template <size_t N>
const char* TableName(const char* const (&table)[N], int state)
{
    return table[state];
}

// Anim script of each phase.
constexpr uint32_t kScript_Idle = 0x86A9B514u;
constexpr uint32_t kScript_Laser = 0x83EA2095u;
constexpr uint32_t kScript_LaserQuick = 0x60C4B6A6u;
constexpr uint32_t kScript_BlockShot = 0x7E1C66E1u;
constexpr uint32_t kScript_Platform = 0x92256EDBu;     // monitor and shoot platform phases
constexpr uint32_t kScript_RopeAttached = 0xC0941292u;
constexpr uint32_t kScript_GroundSmash = 0x2FAE6EF3u;
constexpr uint32_t kScript_ShoulderHit = 0x6644DF75u;
constexpr uint32_t kScript_Struggle = 0x8665BA4Bu;
constexpr uint32_t kScript_TargetHit = 0x0B2921E6u;
constexpr uint32_t kScript_TowerShake = 0x27AD9D6Bu;

// Spline the rope phase's laser follows.
constexpr uint32_t kSpline_RopeLaser = 0xE9F0AB3Fu;
// Default fight camera of most phases.
constexpr uint32_t kCamera_Default = 0xDB8DE820u;

ape::Vec3 TuningVector(const ape::TuningVec3& v)
{
    return {v.Get()[0], v.Get()[1], v.Get()[2], 0.0f};
}

} // namespace

// ===========================================================================
// Base

MegaKnightPhase::MegaKnightPhase(MegaKnight* owner, uint32_t scriptName, const char* const* names, int count)
    : StateMachine(1, 0, 0), boss(owner), stateNames(16)
{
    SetScript(scriptName);
    for (int i = 0; i < count; ++i)
        if (names[i])
            stateNames.PushBack(ape::HashNameNoCase(names[i]));
}

// @func 0x00312118 MegaKnightPhase::~MegaKnightPhase
// @func 0x003121C0 MegaKnightPhase::~MegaKnightPhase (deleting)
MegaKnightPhase::~MegaKnightPhase()
{
    delete script;
    script = nullptr;
}

// @func 0x003044B0 MegaKnightPhase::SetScript
void MegaKnightPhase::SetScript(uint32_t scriptName)
{
    delete script;
    script = nullptr;
    ape::ApeAnimScript* found = ape::ApeAnimScriptLibrary::FindScriptAnywhere(scriptName, 0, true);
    if (!found)
        return;
    // NOTE: the console also clears bit 9 of the new instance's flag word.
    script = new ape::ApeAnimScriptInstance(found);
    if (ape::ApeAnimScriptVar* var = script->FindVar(kScriptVar_Boss)) {
        if (var->GetType() == ape::AnimScriptVarType::Object)
            static_cast<ape::ApeAnimScriptObjectVar*>(var)->SetObject(MKEntityScriptObject(boss));
    }
    script->SetStateAndUpdate(kScriptState_Spawn);
}

// @func 0x00312274 MegaKnightPhase::EnterScriptState
void MegaKnightPhase::EnterScriptState(uint32_t name)
{
    if (!script || !script->GetScript())
        return;
    if (ape::ApeAnimScriptState* state = script->GetScript()->FindState(name))
        script->SetStateAndUpdate(state);
}

// @func 0x003123AC MegaKnightPhase::OnMessage
// Entering a state switches the script to the state of the same name; an
// update runs the phase, then its focus point and its script.
bool MegaKnightPhase::OnMessage(const Message& message)
{
    if (message.type == kMsg_Enter) {
        const uint32_t state = GetState(0);
        if (state < stateNames.Count())
            EnterScriptState(stateNames[state]);
    } else if (message.type == kMsg_Update) {
        const bool result = UpdatePhase(message.dt);
        UpdateFocus();
        if (script)
            script->Update(message.dt);
        return result;
    }
    return true;
}

// @func 0x003124D0 MegaKnightPhase::OnEnter
void MegaKnightPhase::OnEnter()
{
    field_24 = 0.0f;
    ++enterCount;
}

// @func 0x003124E8 MegaKnightPhase::OnExit
void MegaKnightPhase::OnExit(uint32_t next) { (void)next; }
// @func 0x003124EC MegaKnightPhase::OnEventA
void MegaKnightPhase::OnEventA() {}
// @func 0x003124F0 MegaKnightPhase::OnEventB
void MegaKnightPhase::OnEventB() {}
// @func 0x003124F4 MegaKnightPhase::OnEventC
void MegaKnightPhase::OnEventC() {}
// @func 0x003124F8 MegaKnightPhase::OnEventD
void MegaKnightPhase::OnEventD() {}

// @func 0x003124FC MegaKnightPhase::UpdateFocus
// The boss looks at its tracking anchor.
void MegaKnightPhase::UpdateFocus()
{
    boss->lookAt.target.x = boss->tracking.anchor.x;
    boss->lookAt.target.y = boss->tracking.anchor.y;
    boss->lookAt.target.z = boss->tracking.anchor.z;
}

// ===========================================================================
// Idle

// @func 0x00304C14 MKPhaseIdle::MKPhaseIdle
MKPhaseIdle::MKPhaseIdle(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_Idle, kIdleStates, 3)
{
}

// @func 0x0031252C MKPhaseIdle::~MKPhaseIdle
// @func 0x003125C8 MKPhaseIdle::~MKPhaseIdle (deleting)
MKPhaseIdle::~MKPhaseIdle() = default;

// @func 0x00768958 MKPhaseIdle::StateName
const char* MKPhaseIdle::StateName(int state) const { return TableName(kIdleStates, state); }

// @func 0x00312698 MKPhaseIdle::OnEnter
void MKPhaseIdle::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    b->PlayAnim(0x4849E763u, 1.0f, true);
    if (b->currentCamera == kCamera_Default)
        b->SetCamera(-1.0f, 0x001EFED1u, true);
    for (uint32_t arm = 0; arm < 3; ++arm)
        b->SetArmMarker(arm, false);
    b->BlendGlowTo(1.0f);
    b->ClearMarkedTargets();
    const ape::Vec3 trampoline = TuningVector(g_megaKnightMarkerOffsetTrampoline);
    if (b->armIntact[0] || b->armIntact[2]) {
        if (b->armIntact[0])
            b->AddMarkedTarget(b->armTargets[0], trampoline);
        if (b->armIntact[2])
            b->AddMarkedTarget(b->field_328, trampoline);
    } else {
        b->AddMarkedTarget(b->armTargets[1], trampoline);
    }
    SetState(0, 1);
}

// @func 0x00312980 MKPhaseIdle::OnExit
void MKPhaseIdle::OnExit(uint32_t next)
{
    (void)next;
    waitForStamps = false;
    duration = 0.0f;
    stampCount = 0;
}

// @func 0x00312994 MKPhaseIdle::IsDone
bool MKPhaseIdle::IsDone() { return GetState(0) == 2; }

// @func 0x003129E8 MKPhaseIdle::UpdatePhase
// Idles until the stamps the behaviour asked for have landed and the idle
// time is over, then moves on to the next phase; a player on the platform
// starts the monitoring phase.
bool MKPhaseIdle::UpdatePhase(float dt)
{
    (void)dt;
    if (GetState(0) != 1)
        return true;
    if (boss->tracking.fightActive.Get()) {
        boss->SwitchPhase(kMKPhase_MonitorPlatform);
        SetState(0, 2);
        return false;
    }
    if (waitForStamps) {
        if (!boss->stampManager->Settled(stampCount))
            return true;
        waitForStamps = false;
    }
    if (duration != 0.0f && TimeInState(0) < duration)
        return true;
    const uint32_t next = boss->ChooseNextPhase();
    if (int(next) == PhaseId()) {
        SetState(0, 1);
        return true;
    }
    boss->SwitchPhase(next);
    SetState(0, 2);
    return false;
}

// ===========================================================================
// Laser

// @func 0x00304A5C MKPhaseLaser::MKPhaseLaser
MKPhaseLaser::MKPhaseLaser(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_Laser, kLaserStates, 6)
{
    duration = g_megaKnightLaserDefaultDuration.Get();
}

// @func 0x00312BD4 MKPhaseLaser::~MKPhaseLaser
// @func 0x00312C8C MKPhaseLaser::~MKPhaseLaser (deleting)
MKPhaseLaser::~MKPhaseLaser() = default;

// @func 0x00768974 MKPhaseLaser::StateName
const char* MKPhaseLaser::StateName(int state) const { return TableName(kLaserStates, state); }

// TODO(0x00312D78 MKPhaseLaser::OnEnter): aims the laser (vector code the
// disassembler cannot follow).
void MKPhaseLaser::OnEnter()
{
    MegaKnightPhase::OnEnter();
    if (duration <= 0.0f)
        duration = g_megaKnightLaserDefaultDuration.Get();
    interrupted = false;
}

// @func 0x003131E0 MKPhaseLaser::OnExit
void MKPhaseLaser::OnExit(uint32_t next)
{
    (void)next;
    duration = g_megaKnightLaserDefaultDuration.Get();
    boss->platform->Reset(false);
}

// @func 0x00313268 MKPhaseLaser::OnEventA
void MKPhaseLaser::OnEventA() { interrupted = true; }

// @func 0x00313274 MKPhaseLaser::UpdateFocus
// The boss looks at the platform.
void MKPhaseLaser::UpdateFocus()
{
    const ape::Vec3& p = boss->platform->position;
    boss->lookAt.target.x = p.x;
    boss->lookAt.target.y = p.y;
    boss->lookAt.target.z = p.z;
}

// @func 0x0031321C MKPhaseLaser::IsDone
bool MKPhaseLaser::IsDone() { return GetState(0) == 5; }

// TODO(0x003132B0 MKPhaseLaser::UpdatePhase)
bool MKPhaseLaser::UpdatePhase(float dt)
{
    (void)dt;
    return true;
}

// ===========================================================================
// LaserQuick

MKPhaseLaserQuick::MKPhaseLaserQuick(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_LaserQuick, kLaserQuickStates, 6)
{
}

// @func 0x00313E38 MKPhaseLaserQuick::~MKPhaseLaserQuick
// @func 0x00313ED4 MKPhaseLaserQuick::~MKPhaseLaserQuick (deleting)
MKPhaseLaserQuick::~MKPhaseLaserQuick() = default;

// @func 0x00768990 MKPhaseLaserQuick::StateName
const char* MKPhaseLaserQuick::StateName(int state) const { return TableName(kLaserQuickStates, state); }

// @func 0x00313FA0 MKPhaseLaserQuick::OnEnter
// Charges up: the charge clip, the glow to full and the laser camera.
void MKPhaseLaserQuick::OnEnter()
{
    MegaKnightPhase::OnEnter();
    interrupted = false;
    MegaKnight* b = boss;
    b->PlayAnim(0xE8534CE4u, 1.0f, false);
    // The glow blends up at half a unit per second.
    b->glow.to = 1.0f;
    b->glow.from = b->glow.value;
    b->glow.elapsed = 0.0f;
    b->glow.SetDuration((1.0f - b->glow.value) * 0.5f);
    b->SetCamera(0.0f, 0xCB1675F3u, false);
    SetState(0, 1);
}

// @func 0x0031417C MKPhaseLaserQuick::OnExit
void MKPhaseLaserQuick::OnExit(uint32_t next)
{
    (void)next;
    boss->platform->Reset(false);
}

// @func 0x003141F4 MKPhaseLaserQuick::OnEventA
void MKPhaseLaserQuick::OnEventA() { interrupted = true; }

// @func 0x00314200 MKPhaseLaserQuick::UpdateFocus
void MKPhaseLaserQuick::UpdateFocus()
{
    const ape::Vec3& p = boss->platform->position;
    boss->lookAt.target.x = p.x;
    boss->lookAt.target.y = p.y;
    boss->lookAt.target.z = p.z;
}

// @func 0x003141A8 MKPhaseLaserQuick::IsDone
bool MKPhaseLaserQuick::IsDone() { return GetState(0) == 5; }

// TODO(0x00314248 MKPhaseLaserQuick::UpdatePhase)
bool MKPhaseLaserQuick::UpdatePhase(float dt)
{
    (void)dt;
    return true;
}

// ===========================================================================
// BlockShot

MKPhaseBlockShot::MKPhaseBlockShot(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_BlockShot, kBlockShotStates, 3)
{
}

// @func 0x00314BB4 MKPhaseBlockShot::~MKPhaseBlockShot
// @func 0x00314C50 MKPhaseBlockShot::~MKPhaseBlockShot (deleting)
MKPhaseBlockShot::~MKPhaseBlockShot() = default;

// @func 0x007689AC MKPhaseBlockShot::StateName
const char* MKPhaseBlockShot::StateName(int state) const { return TableName(kBlockShotStates, state); }

// @func 0x00314D20 MKPhaseBlockShot::OnEnter
// Swats with an arm that still stands (a random one when both do).
void MKPhaseBlockShot::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    const bool left = b->armIntact[0];
    const bool right = b->armIntact[2];
    if (left && right) {
        const uint32_t seed = ape::g_gameRandom.seed;
        ape::RandomUnit(ape::g_gameRandom);
        swatAnim = (seed & 1) ? 0x2AFA8677u : 0x34ACA98Cu;
    } else if (left) {
        swatAnim = 0x2AFA8677u;
    } else if (right) {
        swatAnim = 0x34ACA98Cu;
    }
    b->SetFlag80(false, -1);
    b->BlendGlowTo(0.0f);
    b->SetCamera(0.0f, 0, false);
    b->PlayAnim(swatAnim, 1.0f, false);
    SetState(0, 1);
}

// @func 0x00314FB8 MKPhaseBlockShot::OnExit
void MKPhaseBlockShot::OnExit(uint32_t next)
{
    (void)next;
    boss->SetFlag80(true, -1);
}

// @func 0x00315000 MKPhaseBlockShot::IsDone
bool MKPhaseBlockShot::IsDone() { return GetState(0) == 2; }

// @func 0x0031504C MKPhaseBlockShot::UpdatePhase
// Done once the swat clip is over.
bool MKPhaseBlockShot::UpdatePhase(float dt)
{
    (void)dt;
    if (GetState(0) != 1 || !MKClipDone(boss))
        return true;
    SetState(0, 2);
    return false;
}

// ===========================================================================
// MonitorPlatform

MKPhaseMonitorPlatform::MKPhaseMonitorPlatform(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_Platform, kMonitorStates, 5)
{
}

// @func 0x00315154 MKPhaseMonitorPlatform::~MKPhaseMonitorPlatform
// @func 0x003151F0 MKPhaseMonitorPlatform::~MKPhaseMonitorPlatform (deleting)
MKPhaseMonitorPlatform::~MKPhaseMonitorPlatform() = default;

// @func 0x007689C8 MKPhaseMonitorPlatform::StateName
const char* MKPhaseMonitorPlatform::StateName(int state) const { return TableName(kMonitorStates, state); }

// @func 0x003152C0 MKPhaseMonitorPlatform::OnEnter
// Watches the arm the player stands on: plays that arm's clips, marks the
// heart (no arm left) or the arm's targets, and points the camera at it.
void MKPhaseMonitorPlatform::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    int onArm = -1;
    if (b->tracking.onArm[0].Get())
        onArm = 0;
    else if (b->tracking.onArm[1].Get())
        onArm = 2;
    else if (b->tracking.onArm[2].Get())
        onArm = 1;
    arm = onArm;

    uint32_t clips[3];
    if (onArm == 0) {
        clips[0] = 0x0FCDA78Cu; clips[1] = 0xA0A65258u; clips[2] = 0x9E40F733u;
    } else if (onArm == 2) {
        clips[0] = 0x418FD0D5u; clips[1] = 0x41752CBFu; clips[2] = 0xBFDEDEBCu;
    } else {
        if (onArm == -1)
            arm = 1;
        clips[0] = 0x5F5891BEu; clips[1] = 0x37DC59E6u; clips[2] = 0x2F84C5E5u;
    }
    // The first half of the fight uses the clip after each one.
    if (!b->secondHalf)
        for (uint32_t& clip : clips)
            ++clip;
    anims[0] = clips[0];
    anims[1] = clips[1];
    anims[2] = clips[2];
    b->PlayAnim(anims[0], 1.0f, false);
    SetState(0, 1);
    b->BlendGlowTo(0.0f);
    b->ClearMarkedTargets();
    const ape::Vec3 shoulder = TuningVector(g_megaKnightMarkerOffsetShoulder);
    if (!b->armIntact[0] && !b->armIntact[2]) {
        b->AddMarkedTarget(b->heartTarget, TuningVector(g_megaKnightMarkerOffsetHeart));
    } else {
        if (b->armIntact[0] && arm == 0)
            b->AddMarkedTarget(b->secondHalf ? b->pair32C.objects[0] : b->pair32C.objects[1], shoulder);
        if (b->armIntact[2] && arm == 2)
            b->AddMarkedTarget(b->secondHalf ? b->pair338.objects[0] : b->pair338.objects[1], shoulder);
    }
    eventC = false;
    b->stampManager->ClearPending();
    b->restartBehavior = true;
    b->SetArmMarker(uint32_t(arm), true);
    b->SetCamera(-1.0f, kCamera_Default, true);
}

// @func 0x00315738 MKPhaseMonitorPlatform::OnExit
void MKPhaseMonitorPlatform::OnExit(uint32_t next) { (void)next; }
// @func 0x003157B0 MKPhaseMonitorPlatform::OnEventA
void MKPhaseMonitorPlatform::OnEventA() {}
// @func 0x003157B4 MKPhaseMonitorPlatform::OnEventC
void MKPhaseMonitorPlatform::OnEventC() { eventC = true; }

// @func 0x00315788 MKPhaseMonitorPlatform::UpdateFocus
void MKPhaseMonitorPlatform::UpdateFocus()
{
    boss->lookAt.target.x = boss->tracking.anchor.x;
    boss->lookAt.target.y = boss->tracking.anchor.y;
    boss->lookAt.target.z = boss->tracking.anchor.z;
}

// @func 0x003157C0 MKPhaseMonitorPlatform::Query17
bool MKPhaseMonitorPlatform::Query17() const { return true; }

// @func 0x0031573C MKPhaseMonitorPlatform::IsDone
bool MKPhaseMonitorPlatform::IsDone() { return GetState(0) == 4; }

// TODO(0x003157D8 MKPhaseMonitorPlatform::UpdatePhase)
bool MKPhaseMonitorPlatform::UpdatePhase(float dt)
{
    (void)dt;
    return true;
}

// ===========================================================================
// ShootPlatform

MKPhaseShootPlatform::MKPhaseShootPlatform(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_Platform, kShootStates, 5)
{
}

// @func 0x003161A4 MKPhaseShootPlatform::~MKPhaseShootPlatform
// @func 0x00316240 MKPhaseShootPlatform::~MKPhaseShootPlatform (deleting)
MKPhaseShootPlatform::~MKPhaseShootPlatform() = default;

// @func 0x007689E4 MKPhaseShootPlatform::StateName
const char* MKPhaseShootPlatform::StateName(int state) const { return TableName(kShootStates, state); }

// @func 0x00316308 MKPhaseShootPlatform::OnEnter
void MKPhaseShootPlatform::OnEnter()
{
    MegaKnightPhase::OnEnter();
    anims[0] = 0x785F53FEu;
    anims[1] = 0x67DC7D75u;
    anims[2] = 0x7883E3B1u;
    boss->PlayAnim(anims[0], g_megaKnightStateValuesShootPlatformStartupSpeed.Get(), false);
    boss->platform->Reset(false);
    SetState(0, 1);
}

// @func 0x003164AC MKPhaseShootPlatform::OnExit
void MKPhaseShootPlatform::OnExit(uint32_t next)
{
    (void)next;
    boss->platform->Reset(false);
}

// @func 0x00316548 MKPhaseShootPlatform::OnEventA
void MKPhaseShootPlatform::OnEventA()
{
    aborted = true;
    boss->ResetRopes();
}

// @func 0x00316614 MKPhaseShootPlatform::OnEventC
void MKPhaseShootPlatform::OnEventC() {}

// @func 0x00316524 MKPhaseShootPlatform::UpdateFocus
void MKPhaseShootPlatform::UpdateFocus()
{
    const ape::Vec3& p = boss->platform->position;
    boss->lookAt.target.x = p.x;
    boss->lookAt.target.y = p.y;
    boss->lookAt.target.z = p.z;
}

// @func 0x00316618 MKPhaseShootPlatform::Query17
bool MKPhaseShootPlatform::Query17() const { return GetState(0) < 2; }

// @func 0x003164D8 MKPhaseShootPlatform::IsDone
bool MKPhaseShootPlatform::IsDone() { return GetState(0) == 4; }

// TODO(0x00316698 MKPhaseShootPlatform::UpdatePhase)
bool MKPhaseShootPlatform::UpdatePhase(float dt)
{
    (void)dt;
    return true;
}

// ===========================================================================
// RopeAttached

// @func 0x00304834 MKPhaseRopeAttached::MKPhaseRopeAttached
MKPhaseRopeAttached::MKPhaseRopeAttached(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_RopeAttached, kRopeStates, 6)
{
    spline = MKFindSplineInstance(kSpline_RopeLaser);
}

// @func 0x003170C0 MKPhaseRopeAttached::~MKPhaseRopeAttached
// @func 0x0031715C MKPhaseRopeAttached::~MKPhaseRopeAttached (deleting)
MKPhaseRopeAttached::~MKPhaseRopeAttached() = default;

// @func 0x00768A00 MKPhaseRopeAttached::StateName
const char* MKPhaseRopeAttached::StateName(int state) const { return TableName(kRopeStates, state); }

// @func 0x00317228 MKPhaseRopeAttached::OnEnter
// Picks the held rope, measures the player's distance to the matching
// bone, starts the laser at the rope's end of its path and charges up.
void MKPhaseRopeAttached::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    if (b->ropes[0] && MKRopeHeld(b->ropes[0]))
        rope = 0;
    else if (b->ropes[2] && MKRopeHeld(b->ropes[2]))
        rope = 2;
    else
        rope = 0;
    for (uint32_t arm = 0; arm < 3; ++arm)
        b->SetArmMarker(arm, false);
    field_74 = rope == 0 ? b->field_3BC : (rope == 2 ? b->field_3C0 : -1);
    if (field_74 != -1) {
        ape::Matrix34 m = ape::world::kWorldIdentity;
        b->BoneWorldTransform(field_74, m);
        const ape::Vec3& a = b->tracking.anchor;
        const float dx = a.x - m.translation.x, dy = a.y - m.translation.y, dz = a.z - m.translation.z;
        distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    // The current camera framing values.
    field_9C = b->cameraController->blends[0].value;
    field_A0 = b->cameraController->blends[1].value;
    field_A4 = b->cameraController->blends[2].value;
    eventB = false;
    field_61 = false;
    field_68 = 0.0f;
    reversed = rope == 2;
    if (spline)
        spline->EvaluatePosition(rope == 2 ? 1.0f : 0.0f, laserPoint, false);
    // Cutting speed by fight half and number of outer arms standing.
    const float speeds[3] = {
        b->secondHalf ? g_megaKnightStateValuesLaserCutterPhase1Arm0.Get() : g_megaKnightStateValuesLaserCutterPhase2Arm0.Get(),
        b->secondHalf ? g_megaKnightStateValuesLaserCutterPhase1Arm1.Get() : g_megaKnightStateValuesLaserCutterPhase2Arm1.Get(),
        b->secondHalf ? g_megaKnightStateValuesLaserCutterPhase1Arm2.Get() : g_megaKnightStateValuesLaserCutterPhase2Arm2.Get(),
    };
    const uint32_t standing = (b->armIntact[0] ? 1u : 0u) + (b->armIntact[2] ? 1u : 0u);
    field_6C = speeds[standing];
    b->glow.to = 1.0f;
    b->glow.from = b->glow.value;
    b->glow.elapsed = 0.0f;
    b->glow.SetDuration((1.0f - b->glow.value) * 0.5f);
    b->lookAt.target = {laserPoint.x, laserPoint.y, laserPoint.z, b->lookAt.target.pad};
    b->lookAt.current = {laserPoint.x, laserPoint.y, laserPoint.z, b->lookAt.current.pad};
    field_B0 = false;
    field_B4 = 0.0f;
    b->PlayAnim(rope == 0 ? 0xAB00CE2Eu : 0x87E1289Bu, 1.0f, false);
    for (entity::Entity* r : b->ropes)
        if (r)
            MKSetInteractionState(r, 4);
    SetState(0, 1);
}

// @func 0x0031771C MKPhaseRopeAttached::OnExit
void MKPhaseRopeAttached::OnExit(uint32_t next)
{
    (void)next;
    boss->platform->Reset(false);
    boss->ResetRopes();
    for (entity::Entity* rope : boss->ropes)
        if (rope)
            MKSetInteractionState(rope, 0);
    rope = -1;
}

// @func 0x003178B8 MKPhaseRopeAttached::OnEventB
void MKPhaseRopeAttached::OnEventB()
{
    eventB = true;
    eventTime = TimeInState(0);
}

// @func 0x003178D0 MKPhaseRopeAttached::UpdateFocus
// While the laser runs the boss looks at the laser point.
void MKPhaseRopeAttached::UpdateFocus()
{
    const uint32_t state = GetState(0);
    if (state > 0 && state < 5) {
        boss->lookAt.target.x = laserPoint.x;
        boss->lookAt.target.y = laserPoint.y;
        boss->lookAt.target.z = laserPoint.z;
    } else {
        boss->lookAt.target.x = boss->tracking.anchor.x;
        boss->lookAt.target.y = boss->tracking.anchor.y;
        boss->lookAt.target.z = boss->tracking.anchor.z;
    }
}

// @func 0x0031786C MKPhaseRopeAttached::IsDone
bool MKPhaseRopeAttached::IsDone() { return GetState(0) == 5; }

// TODO(0x003179B8 MKPhaseRopeAttached::UpdatePhase)
bool MKPhaseRopeAttached::UpdatePhase(float dt)
{
    (void)dt;
    return true;
}

// ===========================================================================
// GroundSmash

// @func 0x00304654 MKPhaseGroundSmash::MKPhaseGroundSmash
MKPhaseGroundSmash::MKPhaseGroundSmash(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_GroundSmash, kGroundSmashStates, 5), pattern(16)
{
}

// @func 0x00318674 MKPhaseGroundSmash::~MKPhaseGroundSmash
// @func 0x0031873C MKPhaseGroundSmash::~MKPhaseGroundSmash (deleting)
MKPhaseGroundSmash::~MKPhaseGroundSmash() = default;

// @func 0x00768A1C MKPhaseGroundSmash::StateName
const char* MKPhaseGroundSmash::StateName(int state) const { return TableName(kGroundSmashStates, state); }

// @func 0x00318830 MKPhaseGroundSmash::OnEnter
// Smashes with an arm that still stands (random when both do) following the
// smash pattern ("SFSF" when none was given).
void MKPhaseGroundSmash::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    const bool left = b->armIntact[0];
    const bool right = b->armIntact[2];
    mirrored = false;
    if (left && right) {
        const uint32_t seed = ape::g_gameRandom.seed;
        ape::RandomUnit(ape::g_gameRandom);
        mirrored = (seed & 1) != 0;
    } else if (left || right) {
        mirrored = left;
    }
    patternIndex = 0;
    if (pattern.Count() == 0) {
        static const char kDefaultPattern[] = "SFSF";
        for (size_t i = 0; i < std::strlen(kDefaultPattern); ++i)
            pattern.PushBack(kDefaultPattern[i]);
        std::strncpy(patternText, kDefaultPattern, sizeof(patternText) - 1);
        patternIndex = 0;
        field_59 = false;
    }
    uint32_t clip;
    if (pattern[0] == 'F')
        clip = mirrored ? 0x4D3A1C52u : 0x2D3112ABu;
    else
        clip = mirrored ? 0xEEEF018Du : 0xC218D4D0u;
    field_58 = true;
    b->SetCamera(0.0f, 0xCB7E4D04u, false);
    b->BlendGlowTo(0.0f);
    b->PlayAnim(clip, 1.0f, false);
    SetState(0, 1);
}

// @func 0x00318BD4 MKPhaseGroundSmash::OnExit
void MKPhaseGroundSmash::OnExit(uint32_t next) { (void)next; }

// @func 0x00318BD8 MKPhaseGroundSmash::IsDone
bool MKPhaseGroundSmash::IsDone() { return GetState(0) == 4; }

// @func 0x00318C30 MKPhaseGroundSmash::UpdatePhase
// State 1: the first smash; 2: the next smashes of the pattern ('F' plays
// the far variant); 3: the recovery clip. A player who reaches the fight
// area sends the boss to watch the platform.
bool MKPhaseGroundSmash::UpdatePhase(float dt)
{
    (void)dt;
    const bool playerInFight = DelayedStateGet(boss->tracking.fightActive) != 0;
    MegaKnight* b = boss;
    auto checkSmash = [&]() {
        if (playerInFight) {
            field_58 = false;
            b->SwitchPhase(kMKPhase_MonitorPlatform);
            SetState(0, 4);
            return false;
        }
        if (!MKClipDone(b))
            return true;
        SetState(0, 2);
        return false;
    };
    switch (GetState(0)) {
    case 1:
        if (TimeInState(0) == 0.0f)
            sly::StartObjectScript(0xE594F012u, nullptr, 0, true);
        return checkSmash();
    case 2:
        if (TimeInState(0) != 0.0f)
            return checkSmash();
        ++patternIndex;
        if (uint32_t(patternIndex) < pattern.Count()) {
            if (!field_59) {
                const bool far = pattern[uint32_t(patternIndex)] == 'F';
                field_5B = far;
                const uint32_t clip = far ? (mirrored ? 0x57EA75C5u : 0x76D24A8Cu)
                                          : (mirrored ? 0xECD237EAu : 0x7EE2CD07u);
                b->currentAnim = 0;
                b->PlayAnim(clip, 1.0f, false);
                return checkSmash();
            }
        } else {
            field_59 = true;
        }
        SetState(0, 3);
        return false;
    case 3: {
        if (TimeInState(0) == 0.0f) {
            const uint32_t clip = field_5B ? (mirrored ? 0xB3A038D1u : 0xF962F16Au)
                                           : (mirrored ? 0xAE3330CCu : 0x512EF8CFu);
            b->PlayAnim(clip, 1.0f, false);
        }
        if (!MKClipDone(b))
            return true;
        if (enterCount == 0 && field_58)
            sly::StartObjectScript(0xB99A192Du, nullptr, 0, true);
        SetState(0, 4);
        return false;
    }
    default:
        return true;
    }
}

// ===========================================================================
// ShoulderHit

MKPhaseShoulderHit::MKPhaseShoulderHit(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_ShoulderHit, kShoulderHitStates, 4)
{
}

// @func 0x00319300 MKPhaseShoulderHit::~MKPhaseShoulderHit
// @func 0x0031939C MKPhaseShoulderHit::~MKPhaseShoulderHit (deleting)
MKPhaseShoulderHit::~MKPhaseShoulderHit() = default;

// @func 0x00768A40 MKPhaseShoulderHit::StateName
const char* MKPhaseShoulderHit::StateName(int state) const { return TableName(kShoulderHitStates, state); }

// @func 0x00319468 MKPhaseShoulderHit::OnEnter
// The shoulder hit plays as a cut scene (per side and fight half); the arm
// markers, ropes and stamps are cleared first.
void MKPhaseShoulderHit::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    b->stampManager->Reset();
    b->currentAnim = 0;
    if (side == 0)
        sequence = b->secondHalf ? 0x7BF6902Fu : 0xFC4CE686u;
    else
        sequence = b->secondHalf ? 0x0331D6D4u : 0x8DA64B5Du;
    b->ClearArmMarkers();
    b->ResetRopes();
    for (entity::Entity* r : b->ropes)
        if (r)
            MKRopeRelease(r);
    MKQueueSequence(sequence);
    b->BlendGlowTo(0.0f);
    b->SetCamera(0.0f, kCamera_Default, false);
    SetState(0, 1);
}

// @func 0x003196D4 MKPhaseShoulderHit::OnExit
// Hides the five arm debris entities and resets the tracking states.
void MKPhaseShoulderHit::OnExit(uint32_t next)
{
    (void)next;
    for (uint32_t i = 0; i < 5; ++i) {
        if (entity::Entity* debris = entity::FindEntity(0x7001C018u + i)) {
            debris->SetVisible(false, true, true, true);
            debris->Release(true);
        }
    }
    MegaKnightTracking& t = boss->tracking;
    t.state220.Force(1);
    t.fightActive.Force(0);
    t.state250.Force(0);
    t.onArm[0].Force(0);
    t.onArm[1].Force(0);
    t.onArm[2].Force(0);
    boss->ResetRopes();
}

// @func 0x003198FC MKPhaseShoulderHit::OnEventD
// The hit failed: the cut scene is replaced by the failure one.
void MKPhaseShoulderHit::OnEventD()
{
    StopSequenceByName(sequence);
    MegaKnight* b = boss;
    sequence = side == 0 ? 0x9D32BD2Eu : 0xEFA2D5A9u;
    b->ClearArmMarkers();
    b->ResetRopes();
    for (entity::Entity* r : b->ropes)
        if (r)
            MKRopeRelease(r);
    MKQueueSequence(sequence);
    SetState(0, 2);
}

// @func 0x003198B0 MKPhaseShoulderHit::IsDone
bool MKPhaseShoulderHit::IsDone() { return GetState(0) == 3; }

// @func 0x00319AB8 MKPhaseShoulderHit::UpdatePhase
// Once the cut scene (hit or failure) is over: control goes back, the arm
// falls (hit) or its shoulder targets come back (failure), and the boss
// idles.
bool MKPhaseShoulderHit::UpdatePhase(float dt)
{
    (void)dt;
    const int state = GetState(0);
    if (state != 1 && state != 2)
        return true;
    if (TimeInState(0) == 0.0f || MKFrameFrozen() || MKSequenceQueued(sequence))
        return true;
    if (entity::GameCamera* camera = MKGameCamera())
        camera->SetActive(false);
    MKPlayerCharacter()->SetActive(false);
    MegaKnight* b = boss;
    if (state == 1) {
        b->OnArmDestroyed(uint32_t(side));
    } else {
        MegaKnightObjectPair& pair = side == 0 ? b->pair32C : b->pair338;
        pair.Show(true, b->secondHalf);
    }
    b->SetCamera(0.0f, 0x001EFED1u, true);
    MegaKnightTracking& t = b->tracking;
    t.state220.Force(1);
    t.fightActive.Force(0);
    t.state250.Force(0);
    t.onArm[0].Force(0);
    t.onArm[1].Force(0);
    t.onArm[2].Force(0);
    if (MegaKnightPhase* idle = b->phases[kMKPhase_Idle]) {
        b->SwitchPhase(kMKPhase_Idle);
        static_cast<MKPhaseIdle*>(idle)->duration = g_megaKnightStateValuesShoulderHitIdleDuration.Get();
        b->SetCamera(1.0f, 0, false);
    }
    SetState(0, 3);
    return false;
}

// ===========================================================================
// Struggle

MKPhaseStruggle::MKPhaseStruggle(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_Struggle, kStruggleStates, 3)
{
}

// @func 0x00319F7C MKPhaseStruggle::~MKPhaseStruggle
// @func 0x0031A018 MKPhaseStruggle::~MKPhaseStruggle (deleting)
MKPhaseStruggle::~MKPhaseStruggle() = default;

// @func 0x00768A64 MKPhaseStruggle::StateName
const char* MKPhaseStruggle::StateName(int state) const { return TableName(kStruggleStates, state); }

// @func 0x0031A0E8 MKPhaseStruggle::OnEnter
// Struggles against the rope the player holds (2 for the third rope, else 0).
void MKPhaseStruggle::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    if (b->ropes[0] && MKRopeHeld(b->ropes[0]))
        rope = 0;
    else if (b->ropes[2] && MKRopeHeld(b->ropes[2]))
        rope = 2;
    else
        rope = 0;
    SetState(0, 1);
    b->BlendGlowTo(0.0f);
    b->SetCamera(0.0f, kCamera_Default, false);
    b->PlayAnim(rope == 0 ? 0x0D592198u : 0x9E21728Bu, 1.0f, true);
}

// @func 0x0031A328 MKPhaseStruggle::OnExit
void MKPhaseStruggle::OnExit(uint32_t next) { (void)next; }

// @func 0x0031A32C MKPhaseStruggle::IsDone
bool MKPhaseStruggle::IsDone() { return GetState(0) == 2; }

// @func 0x0031A378 MKPhaseStruggle::UpdatePhase
// Done once the boss's 0x220 delayed flag comes up.
bool MKPhaseStruggle::UpdatePhase(float dt)
{
    (void)dt;
    if (GetState(0) != 1 || !DelayedStateGet(boss->tracking.state220))
        return true;
    SetState(0, 2);
    return false;
}

// ===========================================================================
// TargetHit

MKPhaseTargetHit::MKPhaseTargetHit(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_TargetHit, kTargetHitStates, 4)
{
}

// @func 0x0031A43C MKPhaseTargetHit::~MKPhaseTargetHit
// @func 0x0031A4D8 MKPhaseTargetHit::~MKPhaseTargetHit (deleting)
MKPhaseTargetHit::~MKPhaseTargetHit() = default;

// @func 0x00768A88 MKPhaseTargetHit::StateName
const char* MKPhaseTargetHit::StateName(int state) const { return TableName(kTargetHitStates, state); }

// @func 0x0031A5A8 MKPhaseTargetHit::OnEnter
void MKPhaseTargetHit::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    if (b->ropes[0] && MKRopeHeld(b->ropes[0]))
        rope = 0;
    else if (b->ropes[2] && MKRopeHeld(b->ropes[2]))
        rope = 2;
    else
        rope = 0;
    SetState(0, 1);
    b->BlendGlowTo(0.0f);
    b->SetCamera(0.0f, kCamera_Default, false);
    b->PlayAnim(rope == 0 ? 0x6BB381E9u : 0x0B131C5Au, 1.0f, false);
}

// @func 0x0031A7E4 MKPhaseTargetHit::OnExit
void MKPhaseTargetHit::OnExit(uint32_t next) { (void)next; }

// @func 0x0031A7E8 MKPhaseTargetHit::IsDone
bool MKPhaseTargetHit::IsDone() { return GetState(0) == 3; }

// @func 0x0031A834 MKPhaseTargetHit::UpdatePhase
// Once the hit clip is over the boss struggles (second half) or goes back
// to the rope.
bool MKPhaseTargetHit::UpdatePhase(float dt)
{
    (void)dt;
    if (GetState(0) != 1 || !MKClipDone(boss))
        return true;
    boss->SwitchPhase(boss->secondHalf ? kMKPhase_Struggle : kMKPhase_RopeAttached);
    SetState(0, 3);
    return false;
}

// ===========================================================================
// TowerShake

MKPhaseTowerShake::MKPhaseTowerShake(MegaKnight* owner)
    : MegaKnightPhase(owner, kScript_TowerShake, kTowerShakeStates, 5)
{
}

// @func 0x0031A984 MKPhaseTowerShake::~MKPhaseTowerShake
// @func 0x0031AA20 MKPhaseTowerShake::~MKPhaseTowerShake (deleting)
MKPhaseTowerShake::~MKPhaseTowerShake() = default;

// @func 0x00768AAC MKPhaseTowerShake::StateName
const char* MKPhaseTowerShake::StateName(int state) const { return TableName(kTowerShakeStates, state); }

// @func 0x0031AAF0 MKPhaseTowerShake::OnEnter
// Clips per arm the player stands on; with skipStart the phase goes straight
// to its second state.
void MKPhaseTowerShake::OnEnter()
{
    MegaKnightPhase::OnEnter();
    MegaKnight* b = boss;
    b->stampManager->Reset();
    b->currentAnim = 0;
    if (b->tracking.onArm[0].Get()) {
        startAnim = 0x96F8E23Bu; endAnim = 0x99104CF4u; igcSequence = 0x60E4DF46u;
    } else if (b->tracking.onArm[1].Get()) {
        startAnim = 0x1AEB962Eu; endAnim = 0xC1F561A7u; igcSequence = 0xBC0D6A9Du;
    } else {
        startAnim = 0x6B54CFC9u; endAnim = 0x13064602u; igcSequence = 0xABD233D4u;
    }
    b->BlendGlowTo(0.0f);
    b->SetCamera(0.0f, kCamera_Default, false);
    if (!skipStart) {
        b->PlayAnim(startAnim, 1.0f, false);
        SetState(0, 1);
    } else {
        SetState(0, 2);
    }
    skipStart = false;
}

// @func 0x0031ADD4 MKPhaseTowerShake::OnExit
// Leaving for the idle phase puts the ropes back.
void MKPhaseTowerShake::OnExit(uint32_t next)
{
    if (next == kMKPhase_Idle)
        boss->ResetRopes();
}

// @func 0x0031AEF0 MKPhaseTowerShake::Query17
bool MKPhaseTowerShake::Query17() const { return GetState(0) == 1; }

// @func 0x0031AEA4 MKPhaseTowerShake::IsDone
bool MKPhaseTowerShake::IsDone() { return GetState(0) == 4; }

// @func 0x0031AF50 MKPhaseTowerShake::UpdatePhase
// State 1: the start clip (a player leaving the fight area cancels to the
// idle phase); 2: the shake cut scene, then idle; 3: the end clip.
bool MKPhaseTowerShake::UpdatePhase(float dt)
{
    (void)dt;
    MegaKnight* b = boss;
    const bool playerInFight = DelayedStateGet(b->tracking.fightActive) != 0;
    auto toIdle = [&]() {
        if (MegaKnightPhase* idle = b->phases[kMKPhase_Idle]) {
            b->SwitchPhase(kMKPhase_Idle);
            static_cast<MKPhaseIdle*>(idle)->duration = 0.5f;
            b->SetCamera(1.0f, 0, false);
        }
        SetState(0, 4);
        return false;
    };
    switch (GetState(0)) {
    case 1:
        if (!MKClipDone(b)) {
            if (playerInFight)
                return true;
            return toIdle();
        }
        SetState(0, 2);
        return false;
    case 2:
        if (TimeInState(0) == 0.0f) {
            // The cut scene casts the player's character variant.
            const entity::Entity* player = MKPlayerCharacter();
            const bool variant = (player->classBits & entity::kClass_20) != 0;
            const MKSequenceBinding bindings[2] = {
                {0x91FAB2C5u, variant ? 0xFEE2E46Au : 0x00014120u},
                {0xAA90B7DDu, variant ? 0x7411A023u : 0u},
            };
            const uint32_t sequence = igcSequence;
            b->ClearArmMarkers();
            b->ResetRopes();
            for (entity::Entity* r : b->ropes)
                if (r)
                    MKRopeRelease(r);
            MKQueueSequenceBound(sequence, bindings, 2);
            return true;
        }
        if (MKFrameFrozen() || MKSequenceQueued(igcSequence))
            return true;
        b->ResetRopes();
        b->SetCamera(0.0f, 0x001EFED1u, true);
        {
            MegaKnightTracking& t = b->tracking;
            t.state220.Force(1);
            t.fightActive.Force(0);
            t.state250.Force(0);
            t.onArm[0].Force(0);
            t.onArm[1].Force(0);
            t.onArm[2].Force(0);
        }
        return toIdle();
    case 3:
        if (TimeInState(0) == 0.0f)
            b->PlayAnim(endAnim, 1.0f, false);
        if (!MKClipDone(b))
            return true;
        SetState(0, 4);
        return false;
    default:
        return true;
    }
}

} // namespace gameplay
} // namespace sly
