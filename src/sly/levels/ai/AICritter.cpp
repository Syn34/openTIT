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
#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeInstances.h"
#include "ape/math/ApeMath.h"
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
    wanderMargin = 0.0f;
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

// @func 0x0001D1E0 CritterAI::GroundProbe
// The ground under the critter: the first static surface between `top` and
// `bottom`.
bool CritterAI::GroundProbe(const ape::Vec3& top, const ape::Vec3& bottom, ape::Vec3& hit)
{
    float fraction = 1.0f;
    if (!PhysicsRayCastStatic(top, bottom, fraction))
        return false;
    hit = {(bottom.x - top.x) * fraction + top.x, (bottom.y - top.y) * fraction + top.y,
           (bottom.z - top.z) * fraction + top.z, hit.pad};
    return true;
}

// @func 0x0001E420 CritterAI::FleePoint
// `toThreat` points from the critter to what it flees from. The point lies
// StopFleeingPlayerDist the other way (level unless the critter flies);
// a point that leaves the critter's area (or, with navigation, a nav mesh
// point too close to the threat) is replaced by a random wander point.
ape::Vec3 CritterAI::FleePoint(const ape::Vec3& toThreat)
{
    const ape::Vec3& position = transform.translation;
    ape::Vec3 away = {-toThreat.x, -toThreat.y, -toThreat.z, 0.0f};
    if (!(flags1 & kF1_FreeFacing))
        away.y = 0.0f;
    const float inv = 1.0f / std::sqrt(away.y * away.y + away.x * away.x + away.z * away.z);
    const float distance = PropFloat(settings, AIProp::Critter_StopFleeingPlayerDist);
    ape::Vec3 point = {away.x * inv * distance + position.x, away.y * inv * distance + position.y,
                       away.z * inv * distance + position.z, 0.0f};

    if (PropBool(settings, AIProp::Critter_UseNavigation)) {
        const ape::Vec3 onMesh = NavSnapPoint(point);
        const ape::Vec3 threat = {position.x + toThreat.x, position.y + toThreat.y, position.z + toThreat.z, 0.0f};
        const float keep = PropFloat(settings, AIProp::Critter_FleePlayerDist) + 0.4f;
        if (DistanceSq(onMesh, threat) < keep * keep)
            return RandomWanderPoint();
        return onMesh;
    }

    if (!HasSpawner(entity))
        return point;
    ape::world::ApeObj* area = SpawnerAltObject(entity);
    if (area && !PropBool(settings, AIProp::Critter_AlwaysUseBoundingRadius)) {
        // NOTE: the console takes the point through the area's world matrix
        // (not its inverse) before testing it against the area's box.
        const ape::Matrix34& w = area->GetWorld();
        const ape::Vec3 inArea = {point.x * w.xAxis.x + point.y * w.yAxis.x + point.z * w.zAxis.x + w.translation.x,
                                  point.x * w.xAxis.y + point.y * w.yAxis.y + point.z * w.zAxis.y + w.translation.y,
                                  point.x * w.xAxis.z + point.y * w.yAxis.z + point.z * w.zAxis.z + w.translation.z,
                                  0.0f};
        if (!ape::AabbContainsPoint(area->GetDef()->GetBounds(), inArea))
            return RandomWanderPoint();
        return point;
    }
    // Within the bounding radius of the spawn point (horizontally unless
    // the critter flies).
    const ape::Vec3 home = SpawnerSpawnPosition(entity);
    const float dx = home.x - point.x;
    const float dy = (flags1 & kF1_FreeFacing) ? home.y - point.y : 0.0f;
    const float dz = home.z - point.z;
    const float radius = PropFloat(settings, AIProp::Critter_BoundingRadius);
    if (dx * dx + dy * dy + dz * dz > radius * radius)
        return RandomWanderPoint();
    return point;
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

// @func 0x0001E068 CritterAI::RandomWanderPoint
// NOTE: the console draws the three random numbers with its own inlined
// steps of the generator; three ordinary draws are used here.
ape::Vec3 CritterAI::RandomWanderPoint()
{
    ape::Vec3 point;
    ape::world::ApeObj* area = SpawnerAltObject(entity);
    if (!area || PropBool(settings, AIProp::Critter_AlwaysUseBoundingRadius)) {
        const float radius = PropFloat(settings, AIProp::Critter_BoundingRadius) - wanderMargin;
        float x = ape::RandomRange(-radius, radius, ape::g_gameRandom);
        float y = ape::RandomRange(wanderMargin, radius, ape::g_gameRandom);
        float z = ape::RandomRange(-radius, radius, ape::g_gameRandom);
        if (x * x + z * z > radius * radius) {
            x *= 0.5f;
            y *= 0.5f;
            z *= 0.5f;
        }
        const ape::Vec3& origin = entity->objectRestoreTransform.translation;
        point = {origin.x + x, origin.y + y, origin.z + z, 0.0f};
    } else {
        const ape::Aabb& box = area->GetDef()->GetBounds();
        const float m = wanderMargin;
        const float x = ape::RandomRange(box.min.x + m, box.max.x - m, ape::g_gameRandom);
        const float y = ape::RandomRange(box.min.y + m, box.max.y - m, ape::g_gameRandom);
        const float z = ape::RandomRange(box.min.z + m, box.max.z - m, ape::g_gameRandom);
        const ape::Matrix34& w = area->GetWorld();
        point = {x * w.xAxis.x + y * w.yAxis.x + z * w.zAxis.x + w.translation.x,
                 x * w.xAxis.y + y * w.yAxis.y + z * w.zAxis.y + w.translation.y,
                 x * w.xAxis.z + y * w.yAxis.z + z * w.zAxis.z + w.translation.z, 0.0f};
    }
    if (!(flags1 & kF1_FreeFacing))
        point.y = transform.translation.y;
    return point;
}

namespace {

// @func 0x000239E4 IsPerchSpline
// Splines a critter may perch on: visible ones of kinds 2, 3,
// 7, 10 and 12; a spline hanging under a hidden object or under another
// spline only counts with its property flag 6.
bool IsPerchSpline(ape::world::ApeSplineInstance* spline, void* /*user*/)
{
    switch (spline->GetKind()) {
    case 2: case 3: case 7: case 10: case 12:
        break;
    default:
        return false;
    }
    if (!spline->IsVisible())
        return false;
    const ape::world::ApeObj* parent = spline->GetParent();
    if (parent && !spline->GetPropertyFlag6() &&
        (!parent->IsVisible() || parent->GetType() == ape::world::kObjType_SplineInstance))
        return false;
    return true;
}

} // namespace

// @func 0x00020100 CritterAI::FindPerch
// Looks for the perch spline closest to a point 20 units ahead, picks a
// random point on it and the side facing away from the critter.
bool CritterAI::FindPerch()
{
    constexpr float kLookAhead = 20.0f;
    const ape::Vec3 ahead = {transform.translation.x + transform.zAxis.x * kLookAhead,
                             transform.translation.y + transform.zAxis.y * kLookAhead,
                             transform.translation.z + transform.zAxis.z * kLookAhead, 0.0f};
    float param = 0.0f;
    float distance = 0.0f;
    ape::world::ApeSplineInstance* rail = ape::world::FindClosestSpline(ahead, &param, &distance, IsPerchSpline, nullptr);
    if (!rail)
        return false;

    param = ape::RandomUnit(ape::g_gameRandom);
    ape::Vec3 tangent{};
    rail->Evaluate(param, poiPosition, tangent, false);
    // Across the rail (tangent x world up), on the far side from the critter.
    const ape::Vec3 up = {0.0f, 1.0f, 0.0f, 0.0f};
    poiDirection = {tangent.y * up.z - tangent.z * up.y, tangent.z * up.x - tangent.x * up.z,
                    tangent.x * up.y - tangent.y * up.x, poiDirection.pad};
    const float side = (poiPosition.x - transform.translation.x) * poiDirection.x +
                       (poiPosition.y - transform.translation.y) * poiDirection.y +
                       (poiPosition.z - transform.translation.z) * poiDirection.z;
    if (side < 0.0f)
        poiDirection = {-poiDirection.x, -poiDirection.y, -poiDirection.z, poiDirection.pad};
    return true;
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

namespace {

// Clips of the states (NOTE: names unknown except "Idle").
constexpr uint32_t kClip_GoTo = 0x376E1DE7u;
constexpr uint32_t kClip_Wait = 0x0001362Au;
constexpr uint32_t kClip_Return = 0x9C2D2710u;
constexpr uint32_t kClip_Run = 0x2874B3ADu;
constexpr uint32_t kClip_Attack = 0x73B1E248u;
constexpr uint32_t kClip_Notice = 0x04B72EEEu;
constexpr uint32_t kClip_Dead = 0x03DE9A54u;
constexpr uint32_t kClip_Land = 0x0487F222u;
constexpr uint32_t kClip_TakeOff = 0x1C0A21EBu;
constexpr uint32_t kClip_Hit = 0xE0F679CCu;

// Inverse of a rotation + translation transform.
ape::Matrix34 InverseRigid(const ape::Matrix34& m)
{
    ape::Matrix34 r;
    r.xAxis = {m.xAxis.x, m.yAxis.x, m.zAxis.x, 0.0f};
    r.yAxis = {m.xAxis.y, m.yAxis.y, m.zAxis.y, 0.0f};
    r.zAxis = {m.xAxis.z, m.yAxis.z, m.zAxis.z, 0.0f};
    const ape::Vec3& t = m.translation;
    r.translation = {-(t.x * r.xAxis.x + t.y * r.yAxis.x + t.z * r.zAxis.x),
                     -(t.x * r.xAxis.y + t.y * r.yAxis.y + t.z * r.zAxis.y),
                     -(t.x * r.xAxis.z + t.y * r.yAxis.z + t.z * r.zAxis.z), 0.0f};
    return r;
}

ape::Vec3 Cross(const ape::Vec3& a, const ape::Vec3& b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, 0.0f};
}

} // namespace

// @func 0x000212B8 CritterAI::ClampToWanderArea
ape::Vec3 CritterAI::ClampToWanderArea(const ape::Vec3& point)
{
    ape::Vec3 out = point;
    ape::world::ApeObj* area = SpawnerAltObject(entity);
    if (!area || PropBool(settings, AIProp::Critter_AlwaysUseBoundingRadius)) {
        const ape::Vec3& origin = entity->objectRestoreTransform.translation;
        const ape::Vec3 toOrigin = {origin.x - point.x, origin.y - point.y, origin.z - point.z, 0.0f};
        const float radius = PropFloat(settings, AIProp::Critter_BoundingRadius) - wanderMargin;
        const float dist = std::sqrt(toOrigin.x * toOrigin.x + toOrigin.y * toOrigin.y + toOrigin.z * toOrigin.z);
        if (dist > radius) {
            // NOTE: the console steps along the unnormalised offset, so the
            // point lands on the radius only for unit distances (kept).
            const float step = dist - radius;
            out.x += toOrigin.x * step;
            out.y += toOrigin.y * step;
            out.z += toOrigin.z * step;
        }
        return out;
    }
    // Clamp between the two shrunk box corners in world space.
    const ape::Aabb& box = area->GetDef()->GetBounds();
    const float m = wanderMargin;
    const ape::Matrix34& w = area->GetWorld();
    auto toWorld = [&w](float x, float y, float z) {
        return ape::Vec3{x * w.xAxis.x + y * w.yAxis.x + z * w.zAxis.x + w.translation.x,
                         x * w.xAxis.y + y * w.yAxis.y + z * w.zAxis.y + w.translation.y,
                         x * w.xAxis.z + y * w.yAxis.z + z * w.zAxis.z + w.translation.z, 0.0f};
    };
    const ape::Vec3 upper = toWorld(box.max.x - m, box.max.y - m, box.max.z - m);
    const ape::Vec3 lower = toWorld(box.min.x + m, box.min.y + m, box.min.z + m);
    out.x = std::max(std::min(out.x, upper.x), lower.x);
    out.y = std::max(std::min(out.y, upper.y), lower.y);
    out.z = std::max(std::min(out.z, upper.z), lower.z);
    return out;
}

// @func 0x000215D8 CritterAI::EnterState
void CritterAI::EnterState()
{
    AIController::EnterState();
    // Moving states use the move clip, or the flight clips for flyers that have them.
    auto playMoveClip = [this](uint32_t clip) {
        if (PropBool(settings, AIProp::Critter_UseFlightAnims))
            UpdateFlightClip();
        else
            PlayClip(clip);
    };
    auto releaseClaim = [this]() {
        if (claimSlot) {
            claimSlot->owner = nullptr;
            claimSlot = nullptr;
        }
    };

    switch (state) {
    case kState_Spawn: {
        // The critter starts where its spawn clip ends when the spawn point
        // is the destination: step back by the rest of the clip's motion.
        PlayClip(PropHash(settings, AIProp::Critter_SpawnAnimClip));
        ape::Matrix34 start = entity->objectRestoreTransform;
        if (PropBool(settings, AIProp::Critter_SpawnPosIsDst)) {
            const float remaining = (1.0f - entity->GetAnimProgress(0)) * entity->GetAnimDuration(0);
            ape::Matrix34 rest;
            entity->RootMotionDelta(rest, false, 0, remaining);
            ape::Matrix34PreMultiply(start, InverseRigid(rest));
        }
        SetTransform(start);
        entity->SetTransform(start);
        entity->SetLocalTransform(start);
        break;
    }
    case kState_Idle:
    case kState_ScriptAnim:
        PlayClip(idleClip);
        break;
    case kState_Move:
        releaseClaim();
        if (poiDelay <= 0.0f)
            poiDelay = ape::RandomRange(PropFloat(settings, AIProp::Critter_POIDelayMin),
                                        PropFloat(settings, AIProp::Critter_POIDelayMax), ape::g_gameRandom);
        SetHomePosition(RandomWanderPoint());
        leashRadius = 1.0f;
        BeginMove();
        if (PropBool(settings, AIProp::Critter_UseFlightAnims))
            UpdateFlightClip();
        else if (currentClip != moveClip)
            PlayClip(moveClip);
        break;
    case kState_GoTo:
        releaseClaim();
        leashRadius = 1.0f;
        BeginMove();
        playMoveClip(moveClip);
        break;
    case kState_Approach: {
        // A point at POITargetDist from the point of interest, on the side the
        // critter comes from, spread randomly up to a right angle.
        const float dist = PropFloat(settings, AIProp::Critter_POITargetDist);
        leashRadius = dist;
        const ape::Vec3& position = transform.translation;
        ape::Vec3 dir = {position.x - poiPosition.x, position.y - poiPosition.y, position.z - poiPosition.z, 0.0f};
        const float inv = 1.0f / std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        dir = {dir.x * inv, dir.y * inv, dir.z * inv, 0.0f};
        const float spread = ape::RandomUnit(ape::g_gameRandom) * 2.0f - 1.0f;
        const float along = 1.0f - std::fabs(spread);
        const ape::Vec3 side = Cross(dir, kUp);
        SetHomePosition({poiPosition.x + (side.x * spread + dir.x * along) * dist,
                         poiPosition.y + (side.y * spread + dir.y * along) * dist,
                         poiPosition.z + (side.z * spread + dir.z * along) * dist, 0.0f});
        BeginMove();
        playMoveClip(kClip_GoTo);
        break;
    }
    case kState_Wait: {
        PlayClip(kClip_Wait);
        poiDelay = RandomBetween(settings, AIProp::Critter_POIDelayMin, AIProp::Critter_POIDelayMax);
        const ape::Vec3& position = transform.translation;
        SetFacing({poiPosition.x - position.x, poiPosition.y - position.y, poiPosition.z - position.z, 0.0f});
        break;
    }
    case kState_Return:
        SetHomePosition(RandomWanderPoint());
        leashRadius = 1.0f;
        BeginMove();
        PlayClip(kClip_Return);
        break;
    case kState_Flee:
        leashRadius = 0.2f;
        BeginMove();
        PlayClip(kClip_Run);
        quickTurnDelay = RandomBetween(settings, AIProp::Critter_QuickTurnDelayMin,
                                       AIProp::Critter_QuickTurnDelayMax);
        break;
    case kState_FleeTurn:
        leashRadius = 0.2f;
        BeginMove();
        PlayClip(kClip_Run);
        break;
    case kState_Charge:
        if (target)
            SetHomePosition(ClampToWanderArea(target->Position()));
        leashRadius = 1.0f;
        BeginMove();
        PlayClip(kClip_Run);
        break;
    case kState_Attack:
        PlayClip(kClip_Attack);
        break;
    case kState_Notice:
        PlayClip(kClip_Notice);
        break;
    case kState_Dead: {
        PostEntityKilledEvent(entity, attacker, hitArgument);
        PlayClip(kClip_Dead);
        if (void* rigidBody = EntityRigidBody(entity)) {
            if (entity::RigidBodyInWorld(rigidBody))
                entity::RigidBodyRemoveFromWorld(rigidBody);
        }
        flags1 &= static_cast<uint8_t>(~kF1_FreeFacing);
        if (!(flags1 & kF1_FlatFacing)) {
            // Stand the body upright, keeping its heading.
            ape::Matrix34 upright = transform;
            upright.yAxis = kUp;
            upright.xAxis = Cross(kUp, transform.zAxis);
            upright.zAxis = Cross(upright.xAxis, kUp);
            entity->SetTransform(upright);
            flags1 |= kF1_FlatFacing;
        }
        break;
    }
    case kState_Gone:
        PlayClip(PropHash(settings, AIProp::Critter_FleeAnimClip));
        facing = HasSpawner(entity) ? SpawnerSpawnForward(entity) : entity->objectRestoreTransform.zAxis;
        break;
    case kState_Perch:
        // Approach the perch point from 5 m out along its direction.
        flags0 |= kF0_Leashed;
        leashRadius = 5.0f;
        SetHomePosition({poiPosition.x - poiDirection.x * 5.0f, poiPosition.y - poiDirection.y * 5.0f,
                         poiPosition.z - poiDirection.z * 5.0f, 0.0f});
        playMoveClip(moveClip);
        break;
    case kState_Land:
        PlayClip(kClip_Land);
        // The landing glide is steered by UpdateState: no root motion.
        entity->rootMotionScale = {};
        entity->rootMotionWeight = 0.0f;
        entity->flags2 |= entity::kEnt2_RootMotionDamped;
        flags1 |= kF1_FlatFacing;
        poiDelay = RandomBetween(settings, AIProp::Critter_POIDelayMin, AIProp::Critter_POIDelayMax);
        SetFacing(poiDirection);
        break;
    case kState_Perched:
        poiDelay = RandomBetween(settings, AIProp::Critter_POIDelayMin, AIProp::Critter_POIDelayMax);
        PlayClip(kClip_Idle);
        break;
    case kState_TakeOff:
        SetFacing({facing.x, 0.5f, facing.z, 0.0f});
        PlayClip(kClip_TakeOff);
        break;
    case kState_Hit:
        PlayClip(kClip_Hit);
        break;
    default:
        break;
    }
}

} // namespace ai
} // namespace levels
} // namespace sly
