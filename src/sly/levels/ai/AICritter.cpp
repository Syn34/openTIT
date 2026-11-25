// openTIT - Critter behaviour (vtable 0x00B0E028).
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/core/ApeString.h"
#include "ape/sys/ApeGeometry.h"
#include "sly/levels/ai/AIComponent.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AIProxyPoints.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/PhysicsHooks.h"
#include "sly/entity/ScriptedEntity.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"
#include "ape/math/ApeMatrixOps.h"
#include "sly/entity/EntityHits.h"

#include <cmath>

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr uint32_t kClip_Idle = 0x002237D4u;   // "Idle"
constexpr uint32_t kClip_Move = 0x002433D1u;   // "MOVE"
constexpr uint32_t kListenerGroup_Critters = 0xA306714Cu;   // NOTE: name unknown

// Critter messages (EntityMessage::id). NOTE: names unknown.
constexpr uint32_t kMsg_Kill = 0x03DE9A54u;
constexpr uint32_t kMsg_Hit1 = 0xF00B0E9Bu;
constexpr uint32_t kMsg_Hit2 = 0x61F07A50u;
constexpr uint32_t kMsg_Hit3 = 0x0665919Fu;
constexpr uint32_t kMsg_Scare1 = 0x9CB5529Eu;
constexpr uint32_t kMsg_Scare2 = 0x04861257u;
constexpr uint32_t kMsg_PointOfInterest = 0xC248AB03u;
constexpr uint32_t kMsg_Distract = 0x5E74BA3Fu;

// Script commands (OnScriptCommand). NOTE: names from what they do.
constexpr uint32_t kCmd_GoToPointIdle = 0xDD152036u;
constexpr uint32_t kCmd_TeleportToPoint = 0xAFAA98B7u;
constexpr uint32_t kCmd_PlayClip = 0x9EA78345u;
constexpr uint32_t kCmd_Resume = 0x8FE13130u;
constexpr uint32_t kCmd_Attack = 0x73B1E248u;
constexpr uint32_t kCmd_GoToPointMove = 0x376E1DF1u;
constexpr uint32_t kCmd_Idle = 0x002237D4u;          // "Idle"
constexpr uint32_t kCmd_SetSpawner = 0xF309EAC6u;
constexpr uint32_t kReaction_CritterBite = 0x2DA0A51Du;   // NOTE: name unknown

const ape::Vec3 kUp = {0.0f, 1.0f, 0.0f, 0.0f};          // 0x01164B60
const ape::Vec3 kForward = {0.0f, 0.0f, 1.0f, 0.0f};     // 0x01164B70 (assumed)

float DistanceSq(const ape::Vec3& a, const ape::Vec3& b)
{
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

float RandomBetween(const PropertyBlock* settings, AIProp minProp, AIProp maxProp)
{
    const float lo = PropFloat(settings, minProp);
    return (PropFloat(settings, maxProp) - lo) * ape::RandomUnit(ape::g_gameRandom) + lo;
}

} // namespace

// @func 0x0001D670 CritterAI::Attach
void CritterAI::Attach(Entity* newEntity)
{
    auto* component = static_cast<AIComponent*>(newEntity->GetComponent0D());
    settings = component ? component->GetSettings() : nullptr;
    AIController::Attach(newEntity);
    AddToListenerGroup(entity, kListenerGroup_Critters, true);

    const float scale = RandomBetween(settings, AIProp::Critter_ScaleMin, AIProp::Critter_ScaleMax);
    entity->SetScale({scale, scale, scale, 0.0f}, false);

    const float radius = PropFloat(settings, AIProp::Critter_BoundingRadius);
    boundsMax = {radius, radius, radius, 0.0f};
    boundsMin = {-radius, -radius, -radius, 0.0f};
    const bool spawned = HasSpawner(entity);
    if (spawned && !PropBool(settings, AIProp::Critter_AlwaysUseBoundingRadius))
        SpawnerAltObjectBounds(entity, boundsMin, boundsMax);

    moveClip = kClip_Move;
    idleClip = kClip_Idle;
    const bool flyer = PropBool(settings, AIProp::Critter_IsFlyer);
    flags1 = static_cast<uint8_t>((flags1 & ~(kF1_FlatFacing | kF1_FreeFacing)) |
                                  (PropBool(settings, AIProp::Critter_OrientYUp) ? kF1_FlatFacing : 0) |
                                  (flyer ? kF1_FreeFacing : 0));
    flags0 |= kF0_NoApply;
    field_13C = 0;
    const bool spawnAnim = PropHash(settings, AIProp::Critter_SpawnAnimClip) != 0;
    if ((!spawnAnim || PropBool(settings, AIProp::Critter_SpawnPosIsDst)) && !flyer)
        SnapToGround(true);

    // Spawn fields scatter their critters in one of four directions.
    if (spawned && SpawnerIsSpawnField(entity)) {
        const int quarter = ape::RandomInt(ape::g_gameRandom, 0, 3);
        ape::Matrix34 rotation;
        ape::MatrixAxisAngle(static_cast<float>(quarter) * 3.1415927f * 0.5f, rotation, kUp);
        Teleport(rotation, true);
    }

    idleDelay = RandomBetween(settings, AIProp::Critter_IdleDelayMin, AIProp::Critter_IdleDelayMax);
    poiDelay = RandomBetween(settings, AIProp::Critter_POIDelayMin, AIProp::Critter_POIDelayMax);
    quickTurnDelay = RandomBetween(settings, AIProp::Critter_QuickTurnDelayMin, AIProp::Critter_QuickTurnDelayMax);
    poiPosition = ape::Vec3{};
    poiDirection = kForward;
    field_1C = kState_Move;
    field_144 = 0;
    if (spawnAnim)
        SetState(kState_Spawn);
    else if (idleDelay > 0.0f || !CharacterHasClip(characterComp, idleClip))
        SetState(kState_Move);
    else
        SetState(kState_Idle);
}

// @func 0x0001DCF0 CritterAI::Update
void CritterAI::Update(float dt)
{
    if (target && !target->IsAlive())
        DropTarget();
    if (entity->flags & (0x40u | 0x20u))
        return;
    if (PropBool(settings, AIProp::Critter_SnapToGround) && state != kState_Dead && state != kState_Spawn &&
        state != 0x0E)
        SnapToGround(false);
    AIController::Update(dt);
}

// @func 0x0001D548 CritterAI::SnapToGround
// Probes the ground a little above and below the critter (from the spawn
// height the first time) and stands on what it hits; without a hit it
// stays at the spawn height, or goes back to its last position.
void CritterAI::SnapToGround(bool fromSpawnHeight)
{
    ape::Vec3 top = transform.translation;
    ape::Vec3 bottom = transform.translation;
    if (fromSpawnHeight) {
        // NOTE: the console uses the same height for both ends here.
        const float height = boundsMax.y + entity->objectRestoreTransform.translation.y;
        top.y = height;
        bottom.y = height;
    } else {
        top.y = transform.translation.y + 0.2f;
        bottom.y = transform.translation.y - 0.2f;
    }
    ape::Vec3 hit;
    if (GroundProbe(top, bottom, hit))
        transform.translation = hit;
    else
        transform.translation = fromSpawnHeight ? bottom : lastPosition;
}

// TODO(0x0001D1E0 CritterAI::GroundProbe): ray cast against the level
// collision (bad instruction data in the reference).
bool CritterAI::GroundProbe(const ape::Vec3& top, const ape::Vec3& bottom, ape::Vec3& hit)
{
    (void)top; (void)bottom; (void)hit;
    return false;
}

// TODO(0x0001E420 CritterAI::FleePoint)
ape::Vec3 CritterAI::FleePoint(const ape::Vec3& away)
{
    return {transform.translation.x - away.x, transform.translation.y, transform.translation.z - away.z, 0.0f};
}

// @func 0x0001DE74 CritterAI::TickEntity
// Physical critters follow their body.
void CritterAI::TickEntity(float dt)
{
    (void)dt;
    if (!PropBool(settings, AIProp::Critter_UsePhysics))
        return;
    void* body = EntityRigidBody(entity);
    if (body && entity::RigidBodyInWorld(body))
        transform = entity::RigidBodyTransform(body);
}

// @func 0x0001ED28 CritterAI::OnScriptCommand
void CritterAI::OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value)
{
    (void)value;
    entity->SetActive(false);
    // Walk to a named point (snapped to the nav mesh when the critter uses
    // navigation).
    auto goToPoint = [this](uint32_t pointName) {
        ape::world::ApeObj* point = ape::world::FindObject(pointName, ape::world::kObjType_PointInstance);
        if (!point)
            return;
        ape::Vec3 goal = point->GetLocal().translation;
        if (PropBool(settings, AIProp::Critter_UseNavigation))
            goal = NavSnapPoint(goal);
        SetHomePosition(goal);
    };
    switch (command) {
    case kCmd_GoToPointIdle:
        if (!arg2)
            return;
        goToPoint(arg2);
        if (arg1) {
            idleClip = arg1;
            field_1C = kState_ScriptAnim;
        }
        SetState(kState_GoTo);
        return;
    case kCmd_GoToPointMove:
        if (!arg2)
            return;
        goToPoint(arg2);
        if (arg1)
            moveClip = arg1;
        SetState(kState_GoTo);
        return;
    case kCmd_TeleportToPoint:
        if (ape::world::ApeObj* point = ape::world::FindObject(arg2, ape::world::kObjType_PointInstance))
            Teleport(point->GetLocal(), false);
        return;
    case kCmd_PlayClip:
        if (arg1)
            idleClip = arg1;
        entity::CharacterCompResetPose(characterComp);
        SetState(kState_ScriptAnim);
        return;
    case kCmd_Resume:
        entity::CharacterCompResetPose(characterComp);
        SetState(field_1C);
        return;
    case kCmd_Attack:
        if (Entity* victim = entity::FindEntity(arg2)) {
            SetTarget(victim);
            SetState(kState_Charge);
            flags1 |= kF1_Bit1;
        }
        return;
    case kMsg_Kill:
        if (state != kState_Dead)
            SetState(kState_Dead);
        return;
    case kCmd_Idle:
        if (arg1)
            idleClip = arg1;
        entity::CharacterCompResetPose(characterComp);
        SetState(kState_Idle);
        return;
    case kCmd_SetSpawner:
        if (void* spawner = FindSpawnerByName(arg2))
            entity->field_DC = spawner;
        return;
    default:
        return;
    }
}

// @func 0x0001F408 CritterAI::ChooseState
void CritterAI::ChooseState()
{
    const ape::Vec3& position = transform.translation;
    // Target that can still be fought: a living player character.
    auto fightableTarget = [this]() -> Entity* {
        if (!target || !(target->typeBits[0] & 2))
            return nullptr;
        if (static_cast<entity::ScriptedEntity*>(target)->IsDead())
            return nullptr;
        return target;
    };
    auto idleOrMove = [this]() { SetState(CharacterHasClip(characterComp, idleClip) ? kState_Idle : kState_Move); };
    const float attackDist = PropFloat(settings, AIProp::Critter_AttackPlayerDist);

    switch (state) {
    case kState_Idle:
        idleDelay = RandomBetween(settings, AIProp::Critter_IdleDelayMin, AIProp::Critter_IdleDelayMax);
        if (idleDelay > 0.0f)
            SetState(kState_Move);
        return;
    case kState_Move:
        if (IsWithinLeash() || stateTime > 6.0f)
            SetState(kState_Move);
        return;
    case kState_GoTo:
        if (IsWithinLeash())
            SetState(field_1C);
        return;
    case kState_Approach:
        if (IsWithinLeash())
            SetState(kState_Wait);
        return;
    case kState_Wait:
    case kState_Perched:
        return;
    case kState_Return:
        if (IsWithinLeash())
            SetState(kState_Move);
        return;
    case kState_Flee: {
        if (!IsWithinLeash() && stateTime <= 4.0f)
            return;
        Entity* player = PlayerCharacter();
        if (!player)
            return;
        const float stopDist = PropFloat(settings, AIProp::Critter_StopFleeingPlayerDist);
        if (DistanceSq(player->Position(), position) > stopDist * stopDist) {
            idleOrMove();
            return;
        }
        const ape::Vec3 away = {player->Position().x - position.x, player->Position().y - position.y,
                                player->Position().z - position.z, 0.0f};
        SetHomePosition(FleePoint(away));
        return;
    }
    case kState_FleeTurn:
        if (IsWithinLeash() || stateTime > 4.0f)
            SetState(kState_Flee);
        return;
    case kState_Charge: {
        Entity* victim = fightableTarget();
        const AIManager* manager = GetAIManager();
        if (!victim || (manager && manager->targetingPause > 0.0f)) {
            idleOrMove();
            return;
        }
        const float d2 = DistanceSq(victim->Position(), position);
        if (d2 < attackDist * attackDist) {
            SetState(kState_Attack);
            return;
        }
        if (!(victim->classBits & 1))
            return;
        const float chargeDist = PropFloat(settings, AIProp::Critter_ChargePlayerDist);
        if (d2 <= chargeDist * chargeDist && TargetReachable())
            return;
        SetState(kState_Move);
        return;
    }
    case kState_Attack: {
        Entity* victim = fightableTarget();
        if (!victim) {
            idleOrMove();
            return;
        }
        const float d2 = DistanceSq(victim->Position(), position);
        SetState(d2 >= attackDist * attackDist ? kState_Charge : kState_Attack);
        return;
    }
    case kState_Notice: {
        Entity* player = PlayerCharacter();
        if (!player)
            return;
        const float noticeDist = PropFloat(settings, AIProp::Critter_NoticePlayerDist);
        if (DistanceSq(player->Position(), position) > noticeDist * noticeDist)
            SetState(kState_Idle);
        return;
    }
    case kState_Dead:
    case kState_Gone:
        // The body stays where it fell and stops updating.
        entity->SetBusyFlag(true);
        entity->flags2 |= entity::kEnt2_NoUpdate;
        return;
    case kState_Perch:
        if (IsWithinLeash())
            SetState(kState_Land);
        else if (stateTime > 8.0f)
            SetState(kState_Move);
        return;
    case kState_Land:
        SetState(kState_Perched);
        return;
    default:
        SetState(kState_Move);
        return;
    }
}

// @func 0x00020310 CritterAI::Think
// Reacts to the nearest enemy (the target while it is held): flee or hide
// when it comes too close (perchers take off), notice it further out,
// charge it from the move state and hit it when in reach of the attack
// bone. Then the state timers: perches end, fleeing critters make quick
// turns, waits end, idles start and points of interest are visited.
bool CritterAI::Think(float dt)
{
    if (state == kState_Dead || state == kState_ScriptAnim || state == kState_GoTo || state == kState_Gone ||
        state == kState_Hit)
        return false;

    Entity* victim = (target && (target->typeBits[0] & 2)) ? target : nullptr;
    if (!victim || !(flags1 & kF1_Bit1) || !victim->IsAlive()) {
        const AIManager* manager = GetAIManager();
        victim = manager ? manager->PickTarget(this) : nullptr;
    }
    const ape::Vec3& position = transform.translation;
    const bool percher = PropBool(settings, AIProp::Critter_PerchOnRails);
    if (victim) {
        const ape::Vec3 away = {victim->Position().x - position.x, victim->Position().y - position.y,
                                victim->Position().z - position.z, 0.0f};
        float d2 = away.x * away.x + away.y * away.y + away.z * away.z;
        const float fleeDist = PropFloat(settings, AIProp::Critter_FleePlayerDist);
        if (d2 < fleeDist * fleeDist) {
            if (!percher) {
                if (PropHash(settings, AIProp::Critter_FleeAnimClip) != 0) {
                    SetState(kState_Gone);
                } else if (state != kState_FleeTurn && state != kState_Flee) {
                    SetState(kState_Flee);
                    SetHomePosition(FleePoint(away));
                    return true;
                }
            } else if (state != kState_TakeOff && state != kState_Move) {
                SetState(state == kState_Land ? kState_TakeOff : kState_Move);
            }
        } else {
            const float noticeDist = PropFloat(settings, AIProp::Critter_NoticePlayerDist);
            if (d2 < noticeDist * noticeDist && !percher && state != kState_Notice && state != kState_FleeTurn &&
                state != kState_Flee) {
                SetState(kState_Notice);
                SetHomePosition(position);
                return true;
            }
        }
        const float chargeDist = PropFloat(settings, AIProp::Critter_ChargePlayerDist);
        if (state == kState_Move && !CrittersMayNotCharge() && d2 < chargeDist * chargeDist && TargetReachable()) {
            SetTarget(victim);
            SetState(kState_Charge);
            return true;
        }
        // In reach of the attack bone: a knock back hit.
        if (const uint32_t attackBone = PropHash(settings, AIProp::Critter_AttackBone)) {
            if (const ape::world::ApeSkeletonBone* bone = entity->GetBoneByName(attackBone)) {
                const ape::Vec3& offset = bone->local->translation;
                const float dx = victim->Position().x - (position.x + offset.x);
                const float dy = victim->Position().y - (position.y + offset.y);
                const float dz = victim->Position().z - (position.z + offset.z);
                d2 = dx * dx + dy * dy + dz * dz;
            }
            const float attackDist = PropFloat(settings, AIProp::Critter_AttackPlayerDist);
            if (d2 < attackDist * attackDist) {
                entity::EntityHitRecord hit;
                hit.source = entity;
                hit.reaction = kReaction_CritterBite;
                hit.damageBits = 0;
                hit.flags = static_cast<uint16_t>((3u << 13) | ((entity->classBits & 1) ? entity::EntityHitRecord::kHit_FromPlayer : 0));
                hit.vec10 = kUp;
                hit.data.knock.amount = 10.0f;
                hit.data.knock.knockback = 0.0f;
                static_cast<entity::ScriptedEntity*>(victim)->OnHitCategory3(hit);
                entity->SetTimeScale(kCmd_Attack, true);
            }
        }
    }

    switch (state) {
    case kState_Perched:
        if (poiDelay > 0.0f) {
            poiDelay -= dt;
            if (poiDelay <= 0.0f)
                SetState(kState_TakeOff);
        }
        break;
    case kState_Flee:
        if (quickTurnDelay > 0.0f) {
            quickTurnDelay -= dt;
            Entity* player = PlayerCharacter();
            if (quickTurnDelay < 0.0f && player) {
                QuickTurn(*player);
                quickTurnDelay = RandomBetween(settings, AIProp::Critter_QuickTurnDelayMin,
                                               AIProp::Critter_QuickTurnDelayMax);
            }
        }
        break;
    case kState_Wait:
        if (poiDelay > 0.0f) {
            poiDelay -= dt;
            if (poiDelay <= 0.0f)
                SetState(kState_Return);
        }
        break;
    case kState_Move:
        if (idleDelay > 0.0f) {
            idleDelay -= dt;
            if (idleDelay <= 0.0f)
                SetState(kState_Idle);
        }
        if (poiDelay > 0.0f) {
            poiDelay -= dt;
            if (poiDelay <= 0.0f) {
                if (!percher)
                    SetState(kState_Approach);
                else if (FindPerch())
                    SetState(kState_Perch);
            }
        }
        break;
    default:
        break;
    }
    return false;
}

// Part of Think (0x00020310): a fleeing critter turns aside by a random
// angle (either way, whichever ends up further from the player) when that
// still keeps half the safe distance.
void CritterAI::QuickTurn(const Entity& player)
{
    const float angle = RandomBetween(settings, AIProp::Critter_QuickTurnAngleMin, AIProp::Critter_QuickTurnAngleMax);
    if (std::fabs(angle) < 1.0e-5f)
        return;
    const ape::Vec3& position = transform.translation;
    const ape::Vec3& p = player.Position();
    const ape::Vec3 away = {p.x - position.x, p.y - position.y, p.z - position.z, 0.0f};
    auto candidate = [&](float radians) {
        ape::Matrix34 rotation;
        ape::Matrix34RotationY(radians, rotation);
        const ape::Vec3 turned = {away.x * rotation.xAxis.x + away.y * rotation.yAxis.x + away.z * rotation.zAxis.x,
                                  away.x * rotation.xAxis.y + away.y * rotation.yAxis.y + away.z * rotation.zAxis.y,
                                  away.x * rotation.xAxis.z + away.y * rotation.yAxis.z + away.z * rotation.zAxis.z,
                                  0.0f};
        return FleePoint(turned);
    };
    const float radians = angle * 0.017453292f;
    ape::Vec3 best = candidate(radians);
    float bestD2 = DistanceSq(p, best);
    const ape::Vec3 other = candidate(-radians);
    const float otherD2 = DistanceSq(p, other);
    if (bestD2 <= otherD2) {
        best = other;
        bestD2 = otherD2;
    }
    const float halfSafe = PropFloat(settings, AIProp::Critter_StopFleeingPlayerDist) * 0.5f;
    if (halfSafe * halfSafe <= bestD2) {
        SetHomePosition(best);
        SetState(kState_FleeTurn);
    }
}

// @func 0x000210F8 CritterAI::UpdateFlightClip
void CritterAI::UpdateFlightClip()
{
    constexpr uint32_t kClip_FlyUp = 0x739C20CAu;       // NOTE: names from use
    constexpr uint32_t kClip_FlyDown = 0x8797408Au;
    constexpr uint32_t kClip_BankRight = 0xBFE11843u;
    constexpr uint32_t kClip_BankLeft = 0x3C985140u;
    uint32_t clip = moveClip;
    if (CharacterHasClip(characterComp, kClip_FlyUp))
        clip = transform.translation.y < homePosition.y ? kClip_FlyUp : kClip_FlyDown;
    if (CharacterHasClip(characterComp, kClip_BankRight)) {
        const float ahead = facing.x * transform.zAxis.x + facing.z * transform.zAxis.z;
        const float side = facing.x * transform.xAxis.x + facing.z * transform.xAxis.z;
        if (ahead < 0.96f) {
            if (side > 0.01f)
                clip = kClip_BankRight;
            else if (side < -0.01f)
                clip = kClip_BankLeft;
        }
    }
    if (clip != currentClip && CharacterPlayClip(characterComp, clip))
        currentClip = clip;
}

// @func 0x00022A38 CritterAI::UpdateState
// Facing per state (towards home, the target or the player), the landing
// glide onto a perch, the charge goal and the wait at a point of interest.
void CritterAI::UpdateState(float dt)
{
    const ape::Vec3& position = transform.translation;
    auto faceTowards = [this, &position](const ape::Vec3& point) {
        SetFacing({point.x - position.x, point.y - position.y, point.z - position.z, 0.0f});
    };
    switch (state) {
    case kState_Move:
    case kState_GoTo:
    case kState_Approach:
    case kState_Return:
    case kState_Flee:
    case kState_Charge:
    case kState_Perch: {
        if (flags0 & kF0_NavMoving)
            break;
        const ape::Vec3 toHome = {homePosition.x - position.x, homePosition.y - position.y,
                                  homePosition.z - position.z, 0.0f};
        SetFacing(toHome);
        if (flags1 & kF1_FreeFacing) {
            // Close to the goal a flyer keeps its heading rather than turning
            // round to a point behind it.
            const float lock = FacingLockRadius();
            const ape::Vec3& forward = transform.zAxis;
            const float dist2 = toHome.x * toHome.x + toHome.y * toHome.y + toHome.z * toHome.z;
            if (dist2 < lock * lock && facing.x * forward.x + facing.y * forward.y + facing.z * forward.z < 0.0f &&
                std::fabs(toHome.y) < lock)
                SetFacing(forward);
            if (PropBool(settings, AIProp::Critter_UseFlightAnims))
                UpdateFlightClip();
        }
        break;
    }
    case kState_Attack:
        if (target)
            faceTowards(target->Position());
        break;
    case kState_Notice:
        if (Entity* player = PlayerCharacter())
            faceTowards(player->Position());
        break;
    default:
        break;
    }

    switch (state) {
    case kState_Land: {
        const float t = CharacterClipProgress(characterComp) * dt * 10.0f;
        transform.translation.x += (poiPosition.x - transform.translation.x) * t;
        transform.translation.y += (poiPosition.y - transform.translation.y) * t;
        transform.translation.z += (poiPosition.z - transform.translation.z) * t;
        SetFacing(poiDirection);
        break;
    }
    case kState_Charge:
        if (target)
            SetHomePosition(claimSlot ? claimSlot->position : target->Position());
        break;
    case kState_Wait:
        faceTowards(poiPosition);
        break;
    default:
        break;
    }
    stateTime += dt;
}

// TODO(0x00020100 CritterAI::FindPerch): picks a point on a rail spline
// near the critter and the side to perch on.
bool CritterAI::FindPerch()
{
    return false;
}

// @func 0x0001E7F0 CritterAI::OnMessage
void CritterAI::OnMessage(const entity::EntityMessage& msg)
{
    Entity* sender = MessageSender(msg);
    switch (msg.id) {
    case kMsg_Kill:
        if (state != kState_Dead)
            SetState(kState_Dead);
        return;
    case kMsg_Hit1:
    case kMsg_Hit2:
    case kMsg_Hit3: {
        if (state == kState_Dead)
            return;
        const int next = PropBool(settings, AIProp::Critter_OneHitKill) ? kState_Dead : kState_Hit;
        if (state == next || sender == entity)
            return;
        if (sender) {
            auto* senderAI = dynamic_cast<AIController*>(sender->listener);
            if (!senderAI || senderAI->IsAlly() || senderAI->IsListedActive()) {
                const ape::Vec3 toward = {sender->Position().x - transform.translation.x,
                                          sender->Position().y - transform.translation.y,
                                          sender->Position().z - transform.translation.z, 0.0f};
                FaceDirection(toward);
            }
        }
        attacker = sender;
        hitArgument = msg.argument;
        SetState(next);
        return;
    }
    case kMsg_Scare1:
    case kMsg_Scare2: {
        if (state == kState_Dead || !sender)
            return;
        const ape::Vec3 away = {sender->Position().x - transform.translation.x,
                                sender->Position().y - transform.translation.y,
                                sender->Position().z - transform.translation.z, 0.0f};
        SetState(kState_Flee);
        SetHomePosition(FleePoint(away));
        return;
    }
    case kMsg_PointOfInterest:
        if (!sender || state == 6 || state == 5)
            return;
        poiPosition = sender->Position();
        poiPosition.y += ape::RandomUnit(ape::g_gameRandom) * 3.0f + 0.5f;
        poiDelay = ape::RandomUnit(ape::g_gameRandom) * 1.9f + 0.1f;
        return;
    case kMsg_Distract:
        if (state != kState_Move)
            poiDelay = ape::RandomUnit(ape::g_gameRandom) * 1.9f + 0.1f;
        return;
    default:
        AIController::OnMessage(msg);
        return;
    }
}

// @func 0x0001ECD0 CritterAI::FacingLockRadius
float CritterAI::FacingLockRadius()
{
    if ((flags1 & kF1_FreeFacing) && PropBool(settings, AIProp::Critter_PerchOnRails))
        return 6.0f;
    return 0.0f;
}

// @func 0x0001F2F8 CritterAI::StateName
const char* CritterAI::StateName(int s)
{
    return ape::TempFormat("%d", s);
}

// @func 0x0001F330 CritterAI::SetMoveClip
void CritterAI::SetMoveClip(uint32_t clipHash)
{
    if (moveClip == clipHash)
        return;
    moveClip = clipHash;
    if (state == kState_Move && CharacterPlayClip(characterComp, clipHash))
        currentClip = clipHash;
}

// @func 0x0001F3D8 CritterAI::AllowsInteraction
bool CritterAI::AllowsInteraction()
{
    return state != kState_Dead;
}

// @func 0x000232A8 CritterAI::ExitState
void CritterAI::ExitState()
{
    switch (state) {
    case kState_Idle:
        idleDelay = RandomBetween(settings, AIProp::Critter_IdleDelayMin, AIProp::Critter_IdleDelayMax);
        break;
    case 3: case 4: case 5: case 7: case 8: case 10:
        flags0 &= static_cast<uint8_t>(~(kF0_Leashed | kF0_NavMoving));
        break;
    case 0x0F:
        flags0 &= static_cast<uint8_t>(~kF0_Leashed);
        break;
    case 0x12:
        flags1 = static_cast<uint8_t>((flags1 & ~kF1_FlatFacing) |
                                      (PropBool(settings, AIProp::Critter_OrientYUp) ? kF1_FlatFacing : 0));
        break;
    default:
        break;
    }
}

// @func 0x00023828 CritterAI::OnProxyPointReached
void CritterAI::OnProxyPointReached()
{
    SetState(kState_Idle);
    // NOTE: the console reads the point's properties without a null check.
    const PropertyBlock* props =
        proxyPoint ? static_cast<const PropertyBlock*>(proxyPoint->properties) : nullptr;
    const char* anim = PropString(props, AIProp::ProxyPt_Anim);
    const uint32_t clip = anim ? ape::HashNameNoCase(anim) : kClip_Idle;
    if (CharacterPlayClip(characterComp, clip))
        currentClip = clip;
    AIController::OnProxyPointReached();
}

// @func 0x00023F0C CritterAI::UsesNavigation
bool CritterAI::UsesNavigation()
{
    return PropBool(settings, AIProp::Critter_UseNavigation);
}

} // namespace ai
} // namespace levels
} // namespace sly
