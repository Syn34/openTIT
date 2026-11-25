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

// TODO(0x0001D548 CritterAI::SnapToGround)
void CritterAI::SnapToGround(bool fromSpawnHeight)
{
    (void)fromSpawnHeight;
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
