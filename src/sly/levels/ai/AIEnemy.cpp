// openTIT - Standard enemy behaviour (vtable 0x00B0E5B8).
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/core/ApeString.h"
#include "sly/game/GameEvent.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AIProxyPoints.h"

#include <algorithm>
#include <cmath>

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr uint32_t kClip_EventAlert = 0x1DE2AB30u;   // NOTE: clip names unknown
constexpr uint32_t kClip_DodgeA = 0xE22B711Au;
constexpr uint32_t kClip_DodgeB = 0xBCD6F543u;
constexpr uint32_t kScriptState_Alarm = 0x39F5F435u;
constexpr uint32_t kGroup_Enemies = 0xCB186A06u;
constexpr uint32_t kEvent_CallForHelp = 0x752CA457u;

constexpr int kState_Alarmed = 0x12;
constexpr int kState_Dodge = 0x1A;
constexpr int kState_TurnInPlace = 0x22;

bool Chance(float probability)
{
    return ape::RandomUnit(ape::g_gameRandom) < probability;
}

} // namespace

// @func 0x0002EC04 EnemyAI::Disengage
void EnemyAI::Disengage()
{
    if (!(flags0 & kF0_Bit0))
        return;
    flags0 &= static_cast<uint8_t>(~kF0_Bit0);
    DropTarget();
    if (claimSlot)
        claimSlot->owner = nullptr;
    claimSlot = nullptr;
}

// @func 0x0002ED1C EnemyAI::~EnemyAI
// @func 0x0002ED84 EnemyAI::~EnemyAI (deleting)
EnemyAI::~EnemyAI()
{
    Disengage();
    CharacterSetEventListener(characterComp, nullptr);
}

// @func 0x0002F814 EnemyAI::SetActive
void EnemyAI::SetActive(bool active)
{
    if (!active) {
        CharacterSetEventListener(characterComp, nullptr);
    } else {
        enemyFlagsC &= static_cast<uint8_t>(~0x20u);
        enemyFlagsA &= kEA_Bit3;
        field_180 = 0;
        flags1 &= static_cast<uint8_t>(~kF1_Bit3);
        enemyFlagsB &= 0xC3u;
        field_1BC = ape::Vec3{};
        field_19C = ape::Vec3{};
        patrolInteractionTime = 0.0f;
        field_1B0 = 0.0f;
        field_181 = 0;
        field_1C8 = 0;
        attackTimer = PropFloat(settings, AIProp::Enemy_InitialAttackTime);
        CharacterSetEventListener(characterComp, this);
    }
    AIController::SetActive(active);
}

// @func 0x0002F900 EnemyAI::RefreshTarget
void EnemyAI::RefreshTarget()
{
    if (target && EntityHoldsAttention(target))
        return;
    AIManager* manager = GetAIManager();
    Entity* best = manager ? manager->PickTarget(this) : nullptr;
    if (best && best != target)
        SetTarget(best);
}

// @func 0x00030888 EnemyAI::PreUpdate
void EnemyAI::PreUpdate(float dt)
{
    (void)dt;
}

// @func 0x00030898 EnemyAI::OnEvent
void EnemyAI::OnEvent(int kind)
{
    if (kind == 0)
        enemyFlagsA |= kEA_Bit7;
    else if (kind == 1)
        PlayClip(kClip_EventAlert);
}

// @func 0x00030A70 EnemyAI::LockEntityAnimation
void EnemyAI::LockEntityAnimation()
{
    EntitySetEngaged(entity, true);
    entity->flags2 |= entity::kEnt2_NoUpdate;
}

// @func 0x0003094C EnemyAI::Engage
void EnemyAI::Engage(Entity* newTarget)
{
    if (!(flags0 & kF0_Bit0)) {
        if (!PropBool(settings, AIProp::Enemy_CombatAlways)) {
            GameEvent alert;
            InitEvent03(alert, AsWorldObject(entity), 0, 0, 0);
            GameWorldPostEvent(0, alert);
        }
        flags0 |= kF0_Bit0;
        if (!(enemyFlagsB & kEB_Bit1))
            field_1C = 0x13;
    }
    SetTarget(newTarget);
    field_1B8 = 0;
}

// @func 0x00032740 EnemyAI::IsCharacterDead
bool EnemyAI::IsCharacterDead()
{
    return state == 0x2C || (state >= 0x37 && state <= 0x3A);
}

// @func 0x00032770 EnemyAI::IsHelpless
bool EnemyAI::IsHelpless()
{
    return state > 0x30 && state < 0x35;
}

// @func 0x00032794 EnemyAI::IsEngaged
bool EnemyAI::IsEngaged()
{
    const bool engaged = (flags0 & kF0_Bit0) != 0;
    if (!engaged || !(enemyFlagsB & kEB_Bit1) || !target || !target->IsAlive())
        return engaged;
    const float vision = PropFloat(settings, AIProp::Enemy_VisionDistanceCombat);
    const float range = std::fmax(PropFloat(settings, AIProp::Enemy_FarAttackRange), vision);
    return targetDist < range;
}

// @func 0x00032834 EnemyAI::CanBeTakenDown
bool EnemyAI::CanBeTakenDown()
{
    if (!AllowsInteraction())
        return false;
    // Heavies cannot be taken down while engaged unless helpless.
    if (PropBool(settings, AIProp::Enemy_Heavy) && (flags0 & kF0_Bit0) && !IsHelpless())
        return false;
    return true;
}

// @func 0x00032908 EnemyAI::AllowsInteraction
bool EnemyAI::AllowsInteraction()
{
    if (IsCharacterDead() || field_1B0 > 0.0f || (flags1 & kF1_HitLocked))
        return false;
    return state != 0x24 && state != 0x28 && state != 0x25;
}

// @func 0x000329A8 EnemyAI::OnAlarm
void EnemyAI::OnAlarm()
{
    if ((flags0 & kF0_Bit0) && state != 0x0B) {
        EntitySetScriptState(entity, kScriptState_Alarm);
        Disengage();
        SetState(kState_Alarmed);
    }
}

// @func 0x00032A30 EnemyAI::IsCalm
bool EnemyAI::IsCalm()
{
    return state == 8 || state == 7 || state == 9 || state == 10;
}

// @func 0x0003352C EnemyAI::StateName
const char* EnemyAI::StateName(int s)
{
    return ape::TempFormat("%d", s);
}

// @func 0x00033780 EnemyAI::OnAttacked
void EnemyAI::OnAttacked(Entity* attacker)
{
    if (!(state == 0x19 || state == 0x13 || state == 0x17 || state == 0x16) || !IsBodyKeyframed())
        return;
    if (!Chance(PropFloat(settings, AIProp::Enemy_DodgeRate)))
        return;
    bool hasComponent = false;
    const bool flag = AttackerComponentFlag(attacker, &hasComponent);
    if (!hasComponent)
        return;
    PlayClip(flag ? kClip_DodgeA : kClip_DodgeB);
    SetState(kState_Dodge);
}

// @func 0x000339A0 EnemyAI::OnTauntChance
void EnemyAI::OnTauntChance()
{
    if (Chance(PropFloat(settings, AIProp::Enemy_TauntRate)))
        enemyFlagsA |= kEA_Taunt;
}

// @func 0x00035D44 EnemyAI::CallForHelp
void EnemyAI::CallForHelp()
{
    if (!AITargetingSuppressed())
        BroadcastInRange(entity, PropFloat(settings, AIProp::Enemy_CallForHelpDist), kGroup_Enemies,
                         kEvent_CallForHelp, 0);
}

// @func 0x0003A92C EnemyAI::UpdateMovement
void EnemyAI::UpdateMovement(float dt)
{
    if (!(enemyFlagsA & kEA_Bit1) && state != 0x28)
        AIController::UpdateMovement(dt);
    else if (body)
        BodyRequestHold(body);
}

// @func 0x0003AA00 EnemyAI::GravitySpeed
float EnemyAI::GravitySpeed()
{
    if (state == 0x25 || state == 0x24)
        return -4.0f;
    if (!(character->classBits & 0x4000u) && !IsBodyKeyframed())
        return -10.0f;
    return 0.0f;
}

// @func 0x0003B78C EnemyAI::SetTarget
void EnemyAI::SetTarget(Entity* newTarget)
{
    if ((flags0 & kF0_Bit0) || (newTarget->classBits & 1u))
        AIController::SetTarget(newTarget);
}

// @func 0x0003BA6C EnemyAI::OnTurnInPlace
void EnemyAI::OnTurnInPlace()
{
    if (state != 0x27)
        SetState(kState_TurnInPlace);
}

// @func 0x0003B7F8 EnemyAI::OnNavGoalChanged
void EnemyAI::OnNavGoalChanged()
{
    SetFacing({navGoal.x - transform.translation.x, navGoal.y - transform.translation.y,
               navGoal.z - transform.translation.z, 0.0f});
    const float degrees = SignedAngle(transform.zAxis, facing) * 57.2957763671875f;
    SetState(std::fabs(degrees) > 60.0f ? 0x23 : 0x1F);
}

// @func 0x0003BAB8 EnemyAI::OnProxyPointReached
void EnemyAI::OnProxyPointReached()
{
    const PropertyBlock* props =
        proxyPoint ? static_cast<const PropertyBlock*>(proxyPoint->properties) : nullptr;
    if (const char* anim = PropString(props, AIProp::ProxyPt_Anim)) {
        proxyClip = ape::HashNameNoCase(anim);
        SetState(0x0D);
    }
    AIController::OnProxyPointReached();
}

// @func 0x0003BBB8 EnemyAI::NavFilter
int EnemyAI::NavFilter()
{
    if (PropBool(settings, AIProp::Enemy_Heavy) && state != 6 && state != 1 && state != 0x0C && state != 0x3B)
        return 1;
    return 2;
}

// @func 0x0003BE08 EnemyAI::FilterRootMotion
ape::Vec3 EnemyAI::FilterRootMotion(const ape::Vec3& step)
{
    return step;
}

// @func 0x0003BE30 EnemyAI::NavSearchRadius
float EnemyAI::NavSearchRadius()
{
    return (enemyFlagsB & kEB_FarSearch) ? 500.0f : 50.0f;
}

// @func 0x00034444 EnemyAI::TargetInAttackRange
bool EnemyAI::TargetInAttackRange()
{
    const float range = std::max(PropFloat(settings, AIProp::Enemy_RangedAttackRange),
                                 PropFloat(settings, AIProp::Enemy_FarAttackRange));
    if (!target || targetDist >= range)
        return false;
    if (!PropBool(settings, AIProp::Enemy_AttackRequiresLOS))
        return true;
    return TestLineOfSight(entity->GetCenter(), target->GetCenter(), target);
}

namespace {
constexpr uint32_t kClip_PatrolInteraction = 0x13BF9082u;   // NOTE: name unknown
constexpr int kState_PatrolInteraction = 13;
} // namespace

// @func 0x00039314 EnemyAI::CanStartPatrolInteraction
bool EnemyAI::CanStartPatrolInteraction(AIController* other, uint32_t clip)
{
    if (patrolInteractionTime <= PropFloat(settings, AIProp::Enemy_PatrolInteractionMinTime))
        return false;
    float distSq = partnerDistSq;
    if (other != partner) {
        const ape::Vec3& a = other->transform.translation;
        const ape::Vec3& b = transform.translation;
        const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        distSq = dx * dx + dy * dy + dz * dz;
    }
    const float minDist = PropFloat(settings, AIProp::Enemy_PatrolInteractionMinDist);
    const float maxDist = PropFloat(settings, AIProp::Enemy_PatrolInteractionMaxDist);
    if (distSq >= maxDist * maxDist || distSq <= minDist * minDist)
        return false;
    if (!CharacterHasClip(characterComp, clip) || clip != kClip_PatrolInteraction)
        return false;
    return state == 0 || state == 12 || state == 13;
}

// @func 0x00039178 EnemyAI::StartPatrolInteraction
void EnemyAI::StartPatrolInteraction(AIController* other, uint32_t clip)
{
    patrolInteractionTime = 0.0f;
    proxyClip = clip;
    const ape::Vec3& to = other->transform.translation;
    const ape::Vec3& from = transform.translation;
    facing = {to.x - from.x, to.y - from.y, to.z - from.z, 0.0f};
    if (!(flags1 & kF1_FreeFacing) && ((flags1 & kF1_FlatFacing) || !CanPitch()))
        facing.y = 0.0f;
    const float inv = 1.0f / std::sqrt(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
    facing = {facing.x * inv, facing.y * inv, facing.z * inv, 0.0f};
    if (std::fabs(SignedAngle(facing, transform.zAxis)) > 0.7853982f) {
        OnTurnInPlace();
        stateAfterTurn = kState_PatrolInteraction;
    } else {
        SetState(kState_PatrolInteraction);
    }
}

} // namespace ai
} // namespace levels
} // namespace sly
