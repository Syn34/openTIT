// openTIT - Bentley vs Penelope boss fight fighters (see BvpFighter.h).
#include "sly/gameplay/BvpFighter.h"

#include "sly/gameplay/BvpTuning.h"
#include "sly/gameplay/GameplayWorld.h"

#include "ape/core/ApeString.h"

#include <algorithm>
#include <cmath>

namespace sly {
namespace gameplay {

namespace {

// Clip name formats: the fighter's own actor and the partner actor.
constexpr const char* kClipFormatA = "Clip_%s_%s";
constexpr const char* kClipFormatB = "%s_%s";
constexpr const char* kHockeyFightFormat = "IGC_OperationMousetrap_HockeyFight_%s_%s";

constexpr const char* kIdleMove = "Cbt_Idle_A";
constexpr const char* kWalkMoves[4] = {"Mov_Walk_Left", "Mov_Walk_Right", "Mov_Walk_Forward",
                                       "Mov_Walk_Back"};
constexpr const char* kDodgeMoves[4] = {"Cbt_Dodge_Left", "Cbt_Dodge_Right", "Cbt_Dodge_Back",
                                        "Cbt_Dodge_Forward"};
constexpr const char* kAttackMoves[3] = {"Cbt_Attack_Light_A", "Cbt_Attack_Light_B",
                                         "Cbt_Attack_Heavy"};
constexpr const char* kHitReactMoves[3] = {"Dmg_HitReact_Light", "Dmg_HitReact_Light_B",
                                           "Dmg_HitReact_Heavy"};

// Hints shown when a move cannot be paid for.
constexpr uint32_t kHintNoSuperPower = 0x2BD9DDB4;
constexpr uint32_t kHintNoDodgePower = 0x5E77B5FF;

// Pad buttons read by Bentley.
constexpr uint32_t kButtonDodge = 0x80;
constexpr uint32_t kButton20 = 0x20;
constexpr uint32_t kButtonSuper = 0x10;

// Direction used when the fighters stand on the same spot (console global
// 0x01164B70). NOTE: assumed to be the unit Z axis.
const ape::Vec3 kDefaultDirection = {0.0f, 0.0f, 1.0f, 0.0f};

constexpr float kRadToDeg = 57.2957764f;

float DistanceSq(const ape::Vec3& a, const ape::Vec3& b)
{
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

uint32_t ClipHash(const char* format, const char* prefix, const char* move)
{
    return ape::HashNameNoCase(ape::TempFormat(format, prefix, move));
}

} // namespace

// ---------------------------------------------------------------------------
// BvpBattle
// ---------------------------------------------------------------------------

// @func 0x003B04F8 BvpBattle::BvpBattle
BvpBattle::BvpBattle(WorldObject* ownerIn)
    : owner(ownerIn), controlPenelope(g_bvpControlPenelope)
{
    enemies.reserve(0x20);
    SetState(0, kPhase_Intro);
}

// @func 0x003B0630 BvpBattle::~BvpBattle
// @func 0x003B06C8 BvpBattle::~BvpBattle (deleting)
BvpBattle::~BvpBattle() = default;

// @func 0x003B3014 BvpBattle::OnMessage
bool BvpBattle::OnMessage(const Message& message)
{
    constexpr uint8_t kOwnerFinished = 6;
    if ((owner != nullptr && GoalOwnerState(owner) == kOwnerFinished) || stopped)
        return true;
    if (static_cast<uint32_t>(message.type) >= 3)
        return true;

    switch (GetState(0)) {
    case kPhase_Intro:
        if (message.type == kMsg_Update)
            UpdateIntro();
        return true;
    case kPhase_Fight:
        return HandleFight(message);
    case kPhase_HockeyFight:
        return HandleHockeyFight(message);
    default:
        return true;
    }
}

// @func 0x003B0784 BvpBattle::Reset
void BvpBattle::Reset()
{
    field_1D0 = false;
    field_1D1 = false;
    stopped = false;
    SetState(0, kPhase_Intro);
}

// TODO(0x003B0BA8 BvpBattle::UpdateIntro): sets up the fighters and actors.
void BvpBattle::UpdateIntro() {}
// TODO(0x003B2718 BvpBattle::HandleFight)
bool BvpBattle::HandleFight(const Message&) { return true; }
// TODO(0x003B1E88 BvpBattle::HandleHockeyFight)
bool BvpBattle::HandleHockeyFight(const Message&) { return true; }

void BvpBattle::BlendCamera(const float targets[kCameraParamCount], float time, bool snap)
{
    for (int i = 0; i < kCameraParamCount; ++i) {
        FloatTween& param = cameraParams[i];
        param.easing = kEase_Cosine;
        param.from = param.value;
        param.to = targets[i];
        param.elapsed = 0.0f;
        param.SetDuration(time);
        if (snap)
            param.Update(time);
    }
}

// @func 0x003ACE58 BvpBattle::SetDefaultCamera
void BvpBattle::SetDefaultCamera(bool snap)
{
    if (cameraMode == kCamera_Default && !snap)
        return;
    const float targets[kCameraParamCount] = {
        g_bvpCamDefaultCamDistAbovePlayerMin, g_bvpCamDefaultCamDistAbovePlayerMax,
        g_bvpCamDefaultCamDistFromPlayerMin,  g_bvpCamDefaultCamDistFromPlayerMax,
        g_bvpCamDefaultCamFocalDistAboveMin,  g_bvpCamDefaultCamFocalDistAboveMax,
        g_bvpCamDefaultCamFocalDistMin,       g_bvpCamDefaultCamFocalDistMax,
        g_bvpCamFOV,
    };
    BlendCamera(targets, g_bvpCamDefaultTransitionTime, snap);
    cameraMode = kCamera_Default;
}

// @func 0x003ACFDC BvpBattle::SetSuperCamera
void BvpBattle::SetSuperCamera(bool snap)
{
    if (cameraMode == kCamera_Super && !snap)
        return;
    const float targets[kCameraParamCount] = {
        g_bvpCamSuperCamDistAbovePlayerMin, g_bvpCamSuperCamDistAbovePlayerMax,
        g_bvpCamSuperCamDistFromPlayerMin,  g_bvpCamSuperCamDistFromPlayerMax,
        g_bvpCamSuperCamFocalDistAboveMin,  g_bvpCamSuperCamFocalDistAboveMax,
        g_bvpCamSuperCamFocalDistMin,       g_bvpCamSuperCamFocalDistMax,
        g_bvpCamFOVSuper,
    };
    BlendCamera(targets, g_bvpCamSuperTransitionTime, snap);
    cameraMode = kCamera_Super;
}

// ---------------------------------------------------------------------------
// BvpFighter
// ---------------------------------------------------------------------------

// @func 0x003AABBC BvpFighter::BvpFighter
BvpFighter::BvpFighter(BvpBattle* battleIn, WorldObject* actorIn, WorldObject* partnerIn,
                       WorldObject* victimIn)
    : StateMachine(1), battle(battleIn), actor(actorIn), partnerActor(partnerIn), victim(victimIn)
{
    if (actor != nullptr)
        EntityAddRef(actor);
    if (partnerActor != nullptr)
        EntityAddRef(partnerActor);
    if (victim != nullptr)
        EntityAddRef(victim);
}

// @func 0x003AAD78 BvpFighter::~BvpFighter
// @func 0x003AAEF0 BvpFighter::~BvpFighter (deleting)
BvpFighter::~BvpFighter()
{
    // Give the actors back: release, and remove those still in the world.
    WorldObject* const actors[3] = {actor, partnerActor, victim};
    for (WorldObject* entity : actors) {
        if (entity == nullptr)
            continue;
        EntityRelease(entity, false);
        if (EntityIsInWorld(entity))
            EntityRelease(entity, true);
    }
}

// @func 0x003AAF38 BvpFighter::OnMessage
bool BvpFighter::OnMessage(const Message& message)
{
    if (message.type == kMsg_Enter) {
        field_99 = false;
        field_98 = false;
        field_9D = false;
        field_78 = 0;
        field_7C = 0;
        field_9E = false;
        field_A1 = true;
    }
    return true;
}

// @func 0x003AAF7C BvpFighter::UpdateExtra
void BvpFighter::UpdateExtra()
{
}

// @func 0x003AAF88 BvpFighter::DamageVictim
void BvpFighter::DamageVictim(float amount)
{
    if (victim == nullptr || !EntityIsInWorld(victim) || !EntityIsActor(victim))
        return;

    const float maxHealth = ActorMaxHealth(victim);
    const float floor = maxHealth * 0.05f;
    if (!nonLethal || floor <= ActorHealth(victim) - amount) {
        ActorDamage(victim, amount, 0);
        return;
    }
    healthCapped = true;
    ActorSetHealth(victim, std::max(0.0f, std::min(floor, maxHealth)));
}

// @func 0x003AB0B8 BvpFighter::PlayClips
bool BvpFighter::PlayClips(float speed, uint32_t clipA, uint32_t clipB, bool loop, bool force)
{
    if (currentClip == clipA && !force)
        return false;
    currentClip = clipA;
    clipDone = false;
    if (actor != nullptr)
        ActorPlayScriptClip(actor, clipA, speed, loop);
    if (partnerActor != nullptr)
        ActorPlayScriptClip(partnerActor, clipB, speed, loop);
    return true;
}

void BvpFighter::PlayMove(const char* move, float speed, bool loop, bool force)
{
    const uint32_t clipA = ClipHash(kClipFormatA, actorPrefixA, move);
    const uint32_t clipB = ClipHash(kClipFormatB, actorPrefixB, move);
    PlayClips(speed, clipA, clipB, loop, force);
}

// @func 0x003AB280 BvpFighter::PlayIdle
void BvpFighter::PlayIdle()
{
    PlayMove(kIdleMove, 1.0f, true, false);
}

// @func 0x003AB350 BvpFighter::PlayWalk
void BvpFighter::PlayWalk()
{
    // The dominant stick axis picks the direction and sets the speed.
    const float x = stick[0];
    const float y = stick[1];
    int direction;
    float amount;
    if (std::fabs(y) < std::fabs(x)) {
        direction = x >= 0.0f ? 1 : 0;
        amount = x;
    } else {
        direction = y < 0.0f ? 2 : 3;
        amount = y;
    }
    const float speed = std::fabs(amount);
    PlayMove(kWalkMoves[direction], speed, true, false);

    if (actor != nullptr)
        ActorSetClipSpeed(actor, speed);
    if (partnerActor != nullptr)
        ActorSetClipSpeed(partnerActor, speed);
}

// @func 0x003AB560 BvpFighter::PlayDodge
void BvpFighter::PlayDodge()
{
    const float x = stick[0];
    const float y = stick[1];
    int direction;
    if (std::fabs(x) <= std::fabs(y) || std::fabs(x) < g_bvpBentleyMoveThresholdDodge)
        direction = y < 0.0f ? 3 : 2;
    else
        direction = x < 0.0f ? 0 : 1;
    PlayMove(kDodgeMoves[direction], 1.0f, false, true);
}

// @func 0x003AB6A0 BvpFighter::PlayAttack
void BvpFighter::PlayAttack(uint32_t index)
{
    if (index > 2)
        index = 0;
    PlayMove(kAttackMoves[index], 1.0f, false, true);
}

// @func 0x003AB764 BvpFighter::PlaySuperAttack
void BvpFighter::PlaySuperAttack()
{
    PlayMove("SuperAttack_A", g_bvpBenSuperAttackSpeed, false, true);
}

// @func 0x003AB810 BvpFighter::PlayHitReact
void BvpFighter::PlayHitReact()
{
    if (hitReact == 3)
        hitReact = 0;
    PlayMove(kHitReactMoves[hitReact], 1.0f, false, true);
}

// @func 0x003AB8F0 BvpFighter::PlayHockeyFightShot
void BvpFighter::PlayHockeyFightShot()
{
    // Both actors use the cutscene clip format here.
    const uint32_t clipA = ClipHash(kHockeyFightFormat, actorPrefixA, "Shot_01");
    const uint32_t clipB = ClipHash(kHockeyFightFormat, actorPrefixB, "Shot_01");
    PlayClips(1.0f, clipA, clipB, false, true);
}

// @func 0x003AB9B8 BvpFighter::PlayHockeyFightSuccess
void BvpFighter::PlayHockeyFightSuccess()
{
    PlayMove("HockeyFight_Success", 1.0f, false, true);
}

// @func 0x003ABA70 BvpFighter::PlayHockeyFightFailure
void BvpFighter::PlayHockeyFightFailure()
{
    PlayMove("HockeyFight_Failure", 1.0f, false, true);
}

// @func 0x003ABB20 BvpFighter::UpdateClipDone
void BvpFighter::UpdateClipDone()
{
    if (ActorClipFinished(actor))
        clipDone = true;
}

// @func 0x003ABD78 BvpFighter::DistanceToOpponent
float BvpFighter::DistanceToOpponent() const
{
    const ape::Vec3 from = EntityPosition(actor);
    const ape::Vec3 to = EntityPosition(opponent->actor);
    const float distanceSq = DistanceSq(to, from);
    return distanceSq != 0.0f ? std::sqrt(distanceSq) : 0.0f;
}

// @func 0x003ABE24 BvpFighter::ClipProgress
float BvpFighter::ClipProgress() const
{
    return EntityAnimTime(actor) / EntityAnimLength(actor);
}

// @func 0x003ABE88 BvpFighter::VictimHealthFraction
float BvpFighter::VictimHealthFraction() const
{
    if (victim != nullptr && EntityIsActor(victim))
        return ActorHealth(victim) / ActorMaxHealth(victim);
    return 1.0f;
}

// @func 0x003ABECC BvpFighter::DamageNearbyEnemies
void BvpFighter::DamageNearbyEnemies(float radiusSq)
{
    const ape::Vec3 centre = EntityPosition(actor);
    for (const BvpBattle::EnemySlot& slot : battle->enemies) {
        WorldObject* enemy = slot.actor;
        if (ActorIsDying(enemy) || !EntityIsInWorld(enemy) || !ActorControllerReady(enemy))
            continue;
        if (DistanceSq(EntityPosition(enemy), centre) <= radiusSq)
            ActorDamage(enemy, ActorMaxHealth(enemy), 0);
    }
}

// @func 0x003AC070 BvpFighter::CanHit
bool BvpFighter::CanHit(float range, float maxAngleDeg, const ape::Vec3& point) const
{
    const ape::Vec3 position = EntityPosition(actor);
    const float reachTotal = range + opponent->reach;
    if (DistanceSq(point, position) > reachTotal * reachTotal)
        return false;

    const ape::Vec3 target = EntityPosition(opponent->actor);
    ape::Vec3 direction = kDefaultDirection;
    const float dx = target.x - position.x;
    const float dy = target.y - position.y;
    const float dz = target.z - position.z;
    if (dx != 0.0f || dy != 0.0f || dz != 0.0f) {
        const float inv = 1.0f / std::sqrt(dx * dx + dy * dy + dz * dz);
        direction = {dx * inv, dy * inv, dz * inv, 0.0f};
    }
    const ape::Vec3 forward = EntityForward(actor);
    const float cosAngle =
        ape::Clamp(direction.x * forward.x + direction.y * forward.y + direction.z * forward.z,
                   -1.0f, 1.0f);
    return std::acos(cosAngle) * kRadToDeg <= maxAngleDeg;
}

// @func 0x003AC244 BvpFighter::CanHitOpponent
bool BvpFighter::CanHitOpponent(float range, float maxAngleDeg) const
{
    return CanHit(range, maxAngleDeg, EntityPosition(opponent->actor));
}

// ---------------------------------------------------------------------------
// BvpBentley
// ---------------------------------------------------------------------------

// @func 0x003ACDCC BvpBentley::~BvpBentley
// @func 0x003ACDF8 BvpBentley::~BvpBentley (deleting)
BvpBentley::~BvpBentley() = default;

// @func 0x003AD168 BvpBentley::HasSuperMovePower
bool BvpBentley::HasSuperMovePower()
{
    if (!superPressed)
        return false;
    if (power >= g_bvpBentleySuperMovePowerCost)
        return true;
    ShowButtonHint(kHintNoSuperPower);
    return false;
}

// @func 0x003AD1EC BvpBentley::HasDodgePower
bool BvpBentley::HasDodgePower()
{
    if (!dodgePressed)
        return false;
    if (power >= g_bvpBentleyDodgePowerCost)
        return true;
    ShowButtonHint(kHintNoDodgePower);
    return false;
}

// @func 0x003AE86C BvpBentley::Start
void BvpBentley::Start()
{
    SetState(0, 1);
}

// @func 0x003AE8B0 BvpBentley::TakeHit
bool BvpBentley::TakeHit(float time, bool heavy, int32_t react)
{
    const int state = GetState(0);
    if (state == 7)
        return false;
    if (state == 3) {
        // Late in a dodge Bentley cannot be hit.
        const float progress = EntityAnimTime(actor) / EntityAnimLength(actor);
        if (g_bvpBentleyDodgeInvulMin <= progress && g_bvpBentleyDodgeInvulMax <= progress)
            return false;
    }
    reactTime = time;
    heavyHit = heavy;
    SetState(0, 8);
    hitReact = react;
    return true;
}

// @func 0x003AEA18 BvpBentley::ChooseMove
void BvpBentley::ChooseMove()
{
    if (stick[0] * stick[0] + stick[1] * stick[1] == 0.0f)
        SetState(0, 1);
    else
        SetState(0, 2);
}

// @func 0x003AEAAC BvpBentley::ReadInput
void BvpBentley::ReadInput()
{
    const int pad = PlayerPadIndex(0);
    dodgePressed = PadPressed(pad, kButtonDodge);
    field_9B = PadPressed(pad, kButton20);
    superPressed = PadPressed(pad, kButtonSuper);
    for (int axis = 0; axis < 4; ++axis)
        stick[axis] = PadAxis(pad, axis);
}

// ---------------------------------------------------------------------------
// BvpPenelope
// ---------------------------------------------------------------------------

// @func 0x003AEBC0 BvpPenelope::~BvpPenelope
// @func 0x003AEBEC BvpPenelope::~BvpPenelope (deleting)
BvpPenelope::~BvpPenelope() = default;

// @func 0x003AEC4C BvpPenelope::StartChase
void BvpPenelope::StartChase()
{
    SetState(0, 2);
}

// @func 0x003B00C0 BvpPenelope::Start
void BvpPenelope::Start()
{
    SetState(0, 1);
}

// @func 0x003B0104 BvpPenelope::TakeHit
bool BvpPenelope::TakeHit(float time, bool heavy, int32_t react)
{
    const int state = GetState(0);
    if (state == 8 || state == 6) {
        const float progress = EntityAnimTime(actor) / EntityAnimLength(actor);
        if (progress <= g_bvpPenelopeDodgeInvul)
            return false;
    }
    reactTime = time;
    heavyHit = heavy;
    SetState(0, 7);
    hitReact = react;
    return true;
}

// @func 0x003B0220 BvpPenelope::ChooseMove
void BvpPenelope::ChooseMove()
{
    if (DistanceToOpponent() > g_bvpChaseDistMax)
        SetState(0, 2);
    else
        SetState(0, 1);
}

// @func 0x003B0360 BvpPenelope::UpdateExtra
void BvpPenelope::UpdateExtra()
{
    const int state = GetState(0);
    if (state == 7 && !heavyHit) {
        SetState(0, 8);
        dodged = true;
        return;
    }

    // The weaker Bentley is, the sooner Penelope dodges.
    const float health = VictimHealthFraction();
    float delay = g_bvpPenDodgeDelay3;
    if (health >= 0.33f) {
        delay = g_bvpPenDodgeDelay2;
        if (health >= 0.66f)
            delay = g_bvpPenDodgeDelay1;
    }
    if (delay <= stateTimer && (state == 1 || state == 2)) {
        SetState(0, 8);
        dodged = true;
    }
}

// @func 0x003B04F4 BvpPenelope::ReadInput
void BvpPenelope::ReadInput()
{
}

} // namespace gameplay
} // namespace sly
