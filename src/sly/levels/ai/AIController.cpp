// openTIT - Level AI controller base class and AI manager.
#include "sly/levels/ai/AIController.h"
#include "sly/physics/PhysicsQuery.h"

#include "ape/anim/ApeAnimClip.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/EntityHooks.h"
#include "ape/world/ApeDef.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHits.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/game/WorldInterface.h"
#include "sly/levels/ai/AIAnimEventKnowledge.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AIProxyPoints.h"
#include "sly/levels/ai/AITuning.h"
#include "sly/levels/nav/NavGraph.h"
#include "sly/knowledge/Knowledge.h"

#include <cmath>
#include <limits>

namespace sly {
namespace levels {
namespace ai {

namespace {

AIManager* g_aiManager = nullptr;

// The object the knowledge system is created with (console 0x00F6578C, a
// one-byte object). NOTE: it carries no state.
struct KnowledgeSystemToken;
KnowledgeSystemToken* g_knowledgeToken = nullptr;
struct KnowledgeSystemToken {
    KnowledgeSystemToken() { g_knowledgeToken = this; }
};
ape::anim::AnimClipManager* g_aiAnimClips = nullptr;   // 0x00F557F8 (+4)

// Control types of the AI anim sets (0x00AC000C) and their sync type (0x00AC0080).
const ape::anim::AnimNamedType kAIControlTypes[] = {
    {"Custom", 0x77297F71u},
    {"AISpeed", 0xEFBDD8FFu},
    {"AITargetAngle", 0xC519BC9Au},
    {"AIJumpProgress", 0xC86A9C83u},
    {"AIJumpLaunchDegree", 0x54D87795u},
    {"AITurnDegree", 0xA861BFB1u},
    {"AIStrafeDegree", 0xBCCE5CC3u},
    {"AITargetDist", 0xA34430DFu},
    {"AIHitCount", 0x2E515184u},
    {"AITargetAtkTypeCount", 0xE6F2F0B6u},
    {"AngleFromTarget", 0x07D00AAEu},
    {"DistFromTarget", 0x30B83BE1u},
};
constexpr uint32_t kAIControlTypeCount = sizeof(kAIControlTypes) / sizeof(kAIControlTypes[0]);
const ape::anim::AnimNamedType kAISyncTypes[] = {
    {"AITargetAnim", 0xA342E54Au},
};

constexpr float kRadToDeg = 57.2957763671875f;
constexpr float kDegToRad = 0.01745329238474369f;
constexpr float kPi = 3.1415927410125732f;

// Anim script variables bound when the AI starts a script on its entity.
constexpr uint32_t kScriptVar_This = 0x0027469Eu;       // "this": the entity's world object
constexpr uint32_t kScriptVar_Transform = 0x001E8ECCu;  // NOTE: name unknown; matrix variable
constexpr uint32_t kScriptLibrary_Shared = 0x7719A548u; // NOTE: name unknown; fallback library

// Controller messages (EntityMessage::id). NOTE: names unknown.
constexpr uint32_t kMsg_SetFlag = 0x7B5D8742u;
constexpr uint32_t kMsg_ClearFlag = 0xFB303547u;
constexpr uint32_t kMsg_BodyOn = 0x76502F44u;
constexpr uint32_t kMsg_BodyOff = 0x55BE125Fu;
constexpr uint32_t kMsg_Comp0DOff = 0x9AB924F0u;
constexpr uint32_t kMsg_Comp0DOn = 0x9087CCEBu;
constexpr uint32_t kMsg_NotifyGame = 0x9A986484u;

const ape::Vec3 kUp = {0.0f, 1.0f, 0.0f, 0.0f};   // 0x01164B60

float DistanceSq(const ape::Vec3& a, const ape::Vec3& b)
{
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

// One step of the shared generator; returns its lowest seed bit (the bit the
// console tests for a coin flip).
bool RandomBit()
{
    ape::RandomUnit(ape::g_gameRandom);
    return (ape::g_gameRandom.seed & 1u) != 0;
}

// Rebuilds the x and y axes from the z axis and normalises them.
void Orthonormalize(ape::Matrix34& m)
{
    ape::Vec3 x, y;
    ape::Vec3Cross(x, m.yAxis, m.zAxis);
    m.xAxis = x;
    ape::Vec3Cross(y, m.zAxis, m.xAxis);
    m.yAxis = y;
    ape::Matrix34NormalizeAxes(m);
}

// Script lookup in one library (null when the library or script is missing).
ape::ApeAnimScript* FindLibraryScript(uint32_t library, uint32_t scriptHash)
{
    ape::ApeAnimScriptLibrary* lib = ape::ApeAnimScriptLibrary::Get(library);
    return lib ? lib->FindScript(scriptHash) : nullptr;
}

// New instance of `scriptHash` (level library first) bound to the entity.
ape::ApeAnimScriptInstance* CreateEntityScript(Entity* entity, uint32_t scriptHash)
{
    ape::ApeAnimScript* script = FindLibraryScript(0, scriptHash);
    if (!script)
        script = FindLibraryScript(kScriptLibrary_Shared, scriptHash);
    if (!script)
        return nullptr;
    return new ape::ApeAnimScriptInstance(script);
}

void BindEntityScript(ape::ApeAnimScriptInstance* instance, Entity* entity)
{
    ape::ApeAnimScriptVar* var = instance->FindVar(kScriptVar_This);
    if (var && var->GetType() == ape::AnimScriptVarType::Object)
        static_cast<ape::ApeAnimScriptObjectVar*>(var)->SetObject(
            EntityScriptObject(entity));
    var = instance->FindVar(kScriptVar_Transform);
    if (var && var->GetType() == ape::AnimScriptVarType::Matrix)
        static_cast<ape::ApeAnimScriptMatrixVar*>(var)->SetMatrix(entity->transform);
}

} // namespace

AIManager* GetAIManager() { return g_aiManager; }
void SetAIManager(AIManager* manager) { g_aiManager = manager; }

// Releases the NavPower point handle.
// @func 0x00738364 AIController::~AIController
// @func 0x0073839C AIController::~AIController (deleting)
AIController::~AIController()
{
    if (navPoint)
        NavPointRelease(navPoint);
}

// -- geometry helpers -----------------------------------------------------------------

// @func 0x00013E40 ClosestPointOnSegment
// @func 0x00019248 ClosestPointOnSegment (copy 2)
ape::Vec3 ClosestPointOnSegment(const ape::Vec3& a, const ape::Vec3& b, const ape::Vec3& p)
{
    ape::Vec3 dir = {b.x - a.x, b.y - a.y, b.z - a.z, 0.0f};
    const float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (length < 1e-05f)
        return a;
    const float inv = 1.0f / length;
    dir.x *= inv;
    dir.y *= inv;
    dir.z *= inv;
    const float alongA = dir.x * (p.x - a.x) + dir.y * (p.y - a.y) + dir.z * (p.z - a.z);
    if (alongA <= 0.0f)
        return a;
    // Distance along the segment measured from b (negative inside).
    const float alongB = dir.x * (p.x - b.x) + dir.y * (p.y - b.y) + dir.z * (p.z - b.z);
    if (alongB >= 0.0f)
        return b;
    return {dir.x * alongB + b.x, dir.y * alongB + b.y, dir.z * alongB + b.z, 0.0f};
}

// @func 0x00016680 SignedAngle
float SignedAngle(const ape::Vec3& from, const ape::Vec3& to)
{
    float cosine = from.x * to.x + from.y * to.y + from.z * to.z;
    if (cosine > 1.0f)
        cosine = 1.0f;
    if (cosine < -1.0f)
        cosine = -1.0f;
    float angle = std::acos(cosine);
    // Side test: from . (to x up).
    const float side = from.z * (to.x * kUp.y - to.y * kUp.x) + from.x * (to.y * kUp.z - to.z * kUp.y) +
                       from.y * (to.z * kUp.x - to.x * kUp.z);
    if (side > 0.0f)
        angle = -angle;
    // NOTE: the console wraps by pi, not 2 pi (acos never leaves [0, pi], so
    // this has no effect).
    if (angle > kPi)
        angle -= kPi;
    else if (angle < -kPi)
        angle += kPi;
    return angle;
}

// -- proxy point chains ------------------------------------------------------------------

// @func 0x000154AC ProxyPointNext
AIProxyPoint* ProxyPointNext(const AIProxyPoint* point)
{
    AIProxyPointTable* table = GetAIProxyPointTable();
    const bool isHead = point->head == point->self;
    if (point->prev == point->self || !isHead)
        return table->AtIndex(point->next);
    // From the head: go back to its first child (the one whose prev is the head).
    AIProxyPoint* p = table->AtIndex(point->prev);
    while (p && p->prev != p->head)
        p = table->AtIndex(p->prev);
    return p;
}

// @func 0x000155E0 ProxyPointRandom
AIProxyPoint* ProxyPointRandom(const AIProxyPoint* point)
{
    AIProxyPointTable* table = GetAIProxyPointTable();
    AIProxyPoint* p = table->AtIndex(point->head);
    const int steps = ape::RandomInt(ape::g_gameRandom, 0, p ? p->childCount : 0);
    // NOTE: the walk stops early when it comes back to `point`.
    for (int i = 0; i < steps && p; ++i) {
        if (p == point)
            return p;
        p = table->AtIndex(p->prev);
    }
    return p;
}

// -- AIController ----------------------------------------------------------------------

// @func 0x00011C10 AIController::SetFacing
void AIController::SetFacing(const ape::Vec3& direction)
{
    facing = direction;
    NormalizeFacing();
}

void AIController::NormalizeFacing()
{
    if (!(flags1 & kF1_FreeFacing) && ((flags1 & kF1_FlatFacing) || !CanPitch()))
        facing.y = 0.0f;
    const float inv = 1.0f / std::sqrt(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
    facing.x *= inv;
    facing.y *= inv;
    facing.z *= inv;
}

void AIManager::Register(AIController* controller)
{
    if (controllerCount >= kMaxControllers)
        return;
    controllers[controllerCount++] = controller;
    if (controller->IsListedActive())
        active[activeCount++] = controller;
    if (controller->IsAlly() && allyCount < kMaxAllies)
        allies[allyCount++] = controller->character;
}

// @func 0x00011D08 AIController::Attach
void AIController::Attach(Entity* newEntity)
{
    entity = newEntity;
    entity->listener = this;
    characterComp = entity->GetCharacterComponent();
    body = entity->GetComponent00();
    const bool bodyOn = body && BodyIsEnabled(body);
    flags1 = static_cast<uint8_t>((flags1 & ~kF1_BodyEnabled) | (bodyOn ? kF1_BodyEnabled : 0));
    character = (entity->typeBits[0] & 0x8u) ? entity : nullptr;
    partner = nullptr;
    navAgent = nullptr;
    transform = entity->transform;
    entity->flags |= 0x10000u;

    if (RenderZoomActive())
        EntityApplyRenderZoom(entity, RenderZoom());

    // Patrol start: the point the spawner hands out, else the nearest point.
    AIProxyPointTable* points = GetAIProxyPointTable();
    proxyPoint = points ? points->AtIndex(ProxyPointIndexFromSpawner(entity)) : nullptr;
    if (!proxyPoint && points) {
        float best = std::numeric_limits<float>::max();
        int bestIndex = -1;
        for (int i = 0; i < points->Count(); ++i) {
            const float d = DistanceSq(transform.translation, InstancePosition(points->AtIndex(i)->object));
            if (d < best) {
                best = d;
                bestIndex = i;
            }
        }
        proxyPoint = points->AtIndex(bestIndex);
    }
    bool reverse = false;
    if (proxyPoint && PropBool(static_cast<const PropertyBlock*>(proxyPoint->properties), AIProp::ProxyPt_Reversible))
        reverse = RandomBit();

    turnRate = 0.0f;
    flags2 |= kF2_SightDirty;
    flags0 = reverse ? kF0_PatrolReverse : 0;
    flags1 = static_cast<uint8_t>((flags1 & ~(kF1_Bit1 | kF1_FreeFacing)) | kF1_FlatFacing);
    homePosition = transform.translation;
    stateScript = nullptr;
    if (body && !IsAlly()) {
        BodyRequestStop(body);
        bodyMotion = BodyMotionType(body);
    }
    target = nullptr;
    flags1 &= static_cast<uint8_t>(~(kF1_HitLocked | kF1_Hit));
    leashRadius = 1.0f;
    claimSlot = nullptr;
    currentClip = 0;
    stateRepeat = 0;
    state = -1;
    field_1C = -1;
    for (int32_t& s : stateStack)
        s = 0;
    stateDepth = 0;
    lastPosition = transform.translation;
    targetDir = transform.zAxis;
    SetFacing(transform.zAxis);
    if (UsesNavigation())
        CreateNavAgent();

    if (AIManager* manager = GetAIManager())
        manager->Register(this);
}

// @func 0x00012460 AIController::DropTarget
void AIController::DropTarget()
{
    if (!target)
        return;
    if (!IsAlly())
        target->Release(false);
    target = nullptr;
    EntitySetEngaged(entity, false);
    AnimControllerSetEngaged(entity, false);
}

// @func 0x00012544 AIManager::Unregister
void AIManager::Unregister(AIController* controller)
{
    for (int i = 0; i < controllerCount; ++i) {
        bool removedActive = false;
        if (i < activeCount && active[i] == controller) {
            --activeCount;
            active[i] = active[activeCount];
            active[activeCount] = nullptr;
            removedActive = true;
        }
        AIController* other = controllers[i];
        if (other->partner == controller)
            other->partner = nullptr;
        if (other == controller) {
            --controllerCount;
            controllers[i] = controllers[controllerCount];
            controllers[controllerCount] = nullptr;
            --i;
        } else if (removedActive) {
            --i;
        }
    }

    if (!controller->IsAlly()) {
        // Allies chasing the controller's entity give up.
        for (int i = 0; i < allyCount; ++i) {
            auto* ally = dynamic_cast<AIController*>(allies[i]->listener);
            if (ally && ally->target && ally->target == controller->entity)
                ally->DropTarget();
        }
    } else {
        for (int i = 0; i < allyCount; ++i) {
            if (allies[i] == controller->character) {
                --allyCount;
                allies[i] = allies[allyCount];
                allies[allyCount] = nullptr;
                return;
            }
        }
    }
}

// @func 0x0001285C AIManager::ForgetSighting
void AIManager::ForgetSighting(Entity* forget)
{
    for (int i = 0; i < sightingCount; ++i) {
        if (sightings[i]->entity == forget) {
            --sightingCount;
            std::swap(sightings[i], sightings[sightingCount]);
            --i;
        }
    }
}

// @func 0x0001BCF4 AIManager::ForgetSighting (kind)
void AIManager::ForgetSighting(Entity* forget, uint32_t kind)
{
    for (int i = 0; i < sightingCount; ++i) {
        AISighting* sighting = sightings[i];
        if (sighting->entity == forget && forget->GetBone(sighting->bone) && sighting->kind == kind) {
            --sightingCount;
            std::swap(sightings[i], sightings[sightingCount]);
            --i;
        }
    }
}

// @func 0x0001AE6C AIManager::FindNearestEnemy
Entity* AIManager::FindNearestEnemy(float radius, const ape::Vec3& position, bool engagedOnly) const
{
    Entity* best = nullptr;
    float bestDist = radius * radius;
    for (int i = 0; i < controllerCount; ++i) {
        AIController* controller = controllers[i];
        if (!controller->IsListedActive() || controller->IsCharacterDead())
            continue;
        if (engagedOnly && !controller->IsEngaged())
            continue;
        const float d = DistanceSq(position, controller->entity->Position());
        if (d < bestDist) {
            bestDist = d;
            best = controller->entity;
        }
    }
    return best;
}

// @func 0x00012928 AIController::Detach
void AIController::Detach()
{
    DropTarget();
    flags0 &= static_cast<uint8_t>(~(kF0_Leashed | kF0_NavMoving));
    if (NavPointIsValid(navPoint))
        NavPointRelease(navPoint);
    if (navAgent) {
        NavAgentDestroy(navAgent);
        navAgent = nullptr;
    }
    if (AIManager* manager = GetAIManager()) {
        manager->ForgetSighting(entity);
        manager->Unregister(this);
    }
    if (entity)
        entity->listener = nullptr;
    if (stateScript) {
        ReleaseAnimScript(stateScript);
        stateScript = nullptr;
    }
}

// @func 0x00012B78 AIController::SetActive
void AIController::SetActive(bool activeNow)
{
    if (body) {
        BodySync(body);
        if (!activeNow) {
            BodySetEnabled(body, false);
        } else {
            const bool wanted = (flags1 & kF1_BodyEnabled) != 0;
            if (BodyIsEnabled(body) != wanted)
                BodySetEnabled(body, wanted);
        }
    }
    if (!activeNow) {
        EntityResetAnimMode(entity);
        partner = nullptr;
        DropTarget();
        ExitState();
    } else {
        EnterState();
    }
}

// @func 0x00012D60 AIController::IsWithinLeash
bool AIController::IsWithinLeash() const
{
    if (!(flags0 & kF0_Leashed) || leashRadius <= 0.0f)
        return false;
    return DistanceSq(homePosition, transform.translation) < leashRadius * leashRadius;
}

// @func 0x00012DE0 AIController::SetTransform
void AIController::SetTransform(const ape::Matrix34& m)
{
    transform = m;
}

// @func 0x00012E44 AIController::SetPosition
void AIController::SetPosition(const ape::Vec3& position)
{
    transform.translation.x = position.x;
    transform.translation.y = position.y;
    transform.translation.z = position.z;
}

// @func 0x00012E70 AIController::Update
void AIController::Update(float dt)
{
    if (body)
        SetPosition(BodyPosition(body));
    // Fell out of the level: take the entity's transform back.
    if (AIManager* manager = GetAIManager()) {
        if (transform.translation.y < manager->killHeight)
            transform = entity->transform;
    }
    if (target && !target->IsAlive()) {
        if (!IsAlly())
            target->Release(false);
        target = nullptr;
    }

    PreUpdate(dt);
    if (!Think(dt) && characterComp &&
        ((flags1 & kF1_ForceRethink) || CharacterClipFinished(characterComp) || IsWithinLeash()))
        ChooseState();
    UpdateState(dt);
    if (!(flags0 & kF0_NoApply))
        ApplyTransform(dt);
    UpdateTargetInfo(dt);
    UpdateMovement(dt);
    lastPosition = transform.translation;
}

// @func 0x000131C8 AIController::ApplyTransform
void AIController::ApplyTransform(float dt)
{
    (void)dt;
    if (entity->flags & (0x40u | 0x20u))
        return;
    UpdateNavigation();
    Orthonormalize(transform);
    entity->transform = transform;
}

// @func 0x00013358 AIController::TickEntity
void AIController::TickEntity(float dt)
{
    if (IsAlly())
        return;
    if (!(entity->flags & (0x40u | 0x20u)))
        EntityTick(entity, dt);
}

// @func 0x000133F8 AIController::PreUpdate
void AIController::PreUpdate(float dt)
{
    (void)dt;
}

// @func 0x000133FC AIController::OnMessage
void AIController::OnMessage(const entity::EntityMessage& msg)
{
    switch (msg.id) {
    case kMsg_SetFlag:
        flags0 |= kF0_Messaged;
        break;
    case kMsg_BodyOn:
        if (body) {
            flags1 |= kF1_BodyEnabled;
            BodySetEnabled(body, true);
        }
        break;
    case kMsg_BodyOff:
        if (body) {
            flags1 &= static_cast<uint8_t>(~kF1_BodyEnabled);
            BodySetEnabled(body, false);
        }
        break;
    case kMsg_ClearFlag:
        flags0 &= static_cast<uint8_t>(~kF0_Messaged);
        break;
    case kMsg_Comp0DOff:
        if (EntityComponent* comp = entity->GetComponent0D())
            comp->SetOption(false);
        break;
    case kMsg_NotifyGame:
        GameStateNotify();
        break;
    case kMsg_Comp0DOn:
        if (EntityComponent* comp = entity->GetComponent0D())
            comp->SetOption(true);
        break;
    default:
        break;
    }
}

// @func 0x000135BC AIController::Teleport
void AIController::Teleport(const ape::Matrix34& m, bool moveController)
{
    // NOTE: the console also asks NavPower for the nav mesh point nearest the
    // new position here but does not use the answer.
    entity->objectRestoreTransform = m;
    if (moveController) {
        transform = m;
        EntitySetTransform(entity, m);
        entity->SetLocalTransform(m);
    }
}

// @func 0x000137C4 AIController::GetTargetPosition
ape::Vec3 AIController::GetTargetPosition()
{
    if (target && target != PlayerCharacter())
        return target->GetCenter();
    AIManager* manager = GetAIManager();
    return manager ? manager->lastPlayerPosition : ape::Vec3{};
}

// @func 0x00013F78 AIController::PlayClip
void AIController::PlayClip(uint32_t clipHash)
{
    if (CharacterPlayClip(characterComp, clipHash))
        currentClip = clipHash;
}

// @func 0x00014004 AIController::StartOneShotScript
void AIController::StartOneShotScript(uint32_t scriptHash)
{
    ape::ApeAnimScriptInstance* instance = CreateEntityScript(entity, scriptHash);
    if (!instance)
        return;
    BindEntityScript(instance, entity);
    // NOTE: flag word bit 9 on the console; read as "drop when finished".
    instance->SetRemoveWhenFinished(true);
    ape::ApeAnimScriptInstance::Register(instance);
}

// @func 0x000143C8 AIController::StartStateScript
void AIController::StartStateScript(uint32_t scriptHash)
{
    ape::ApeAnimScript* script = FindLibraryScript(0, scriptHash);
    if (!script)
        script = FindLibraryScript(kScriptLibrary_Shared, scriptHash);
    if (!script)
        return;
    if (stateScript)
        ReleaseAnimScript(stateScript);
    stateScript = new ape::ApeAnimScriptInstance(script);
    BindEntityScript(stateScript, entity);
    stateScript->SetRemoveWhenFinished(false);
    ape::ApeAnimScriptInstance::Register(stateScript);
}

// TODO(0x000147B8 AIController::NavSnapPoint): NavPower nearest point query.
ape::Vec3 AIController::NavSnapPoint(const ape::Vec3& point)
{
    return point;
}

// TODO(0x00015AAC AIController::TargetReachable): NavPower path query.
bool AIController::TargetReachable()
{
    return true;
}

// @func 0x00014DA8 AIController::BeginMove
void AIController::BeginMove()
{
    flags0 |= kF0_Leashed;
    facing = transform.zAxis;
    if (!(flags1 & kF1_FreeFacing) && ((flags1 & kF1_FlatFacing) || !CanPitch()))
        facing.y = 0.0f;
    const float inv = 1.0f / std::sqrt(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
    facing = {facing.x * inv, facing.y * inv, facing.z * inv, 0.0f};
    if (!navAgent)
        return;
    NavManager* nav = GetNavManager();
    if (!nav || !nav->graphData) {
        flags0 |= kF0_NavRetry;   // asked again once a graph is loaded
        return;
    }
    flags0 = static_cast<uint8_t>((flags0 & ~kF0_NavRetry) | kF0_NavMoving);
    const ape::Vec3 position = transform.translation;
    NavAgentSetPosition(navAgent, position);
    NavAgentRequestGoal(navAgent, homePosition);
    homePosition = NavAgentGoal(navAgent);
    if (IsAlly())
        return;
    // A first path point more than 2 m away becomes the steering goal.
    const ape::Vec3 next = NavAgentNextPoint(navAgent);
    const float dx = next.x - position.x, dy = next.y - position.y, dz = next.z - position.z;
    if (dx * dx + dy * dy + dz * dz > 4.0f) {
        navGoal = next;
        OnNavGoalChanged();
    }
}

// @func 0x00015944 AIController::SetHomePosition
void AIController::SetHomePosition(const ape::Vec3& position)
{
    homePosition = position;
    if ((flags0 & kF0_NavMoving) && navAgent) {
        NavAgentRequestGoal(navAgent, position);
        homePosition = NavAgentGoal(navAgent);
    }
}

// @func 0x000159EC AIController::ReturnToProxyPoint
void AIController::ReturnToProxyPoint()
{
    if (proxyPoint)
        SetHomePosition(InstancePosition(proxyPoint->object));
}

// @func 0x00015F90 AIController::FaceDirection
void AIController::FaceDirection(const ape::Vec3& direction)
{
    if (direction.x == 0.0f && direction.z == 0.0f)
        return;
    transform.yAxis = kUp;
    const float inv = 1.0f / std::sqrt(direction.z * direction.z + direction.x * direction.x + 0.0f);
    const ape::Vec3 z = {direction.x * inv, 0.0f * inv, direction.z * inv, 0.0f};
    transform.zAxis = z;
    // x = up x z, normalised.
    const float xx = kUp.y * z.z - kUp.z * z.y;
    const float xy = kUp.z * z.x - kUp.x * z.z;
    const float xz = kUp.x * z.y - kUp.y * z.x;
    const float xInv = 1.0f / std::sqrt(xz * xz + xx * xx + xy * xy);
    transform.xAxis = {xInv * xx, xInv * xy, xInv * xz, 0.0f};
}

// @func 0x00738528 AIController::IsCharacterDead
bool AIController::IsCharacterDead()
{
    return character && CharacterHealth(character) <= 0.0f;
}

// @func 0x00738574 AIController::StateName
const char* AIController::StateName(int s)
{
    return ape::TempFormat("%d", s);
}

// @func 0x000149D0 AIController::ChooseState
void AIController::ChooseState() {}

// @func 0x000149D8 AIController::Think
bool AIController::Think(float dt)
{
    (void)dt;
    return false;
}

// @func 0x000149E0 AIController::SetState
void AIController::SetState(int newState)
{
    if (state != -1)
        ExitState();
    turnRate = 0.0f;
    entity->flags2 &= static_cast<uint16_t>(~0x0800u);
    if (character) {
        CharacterClearAIHold(character);
    }
    if (body)
        BodySetEnabled(body, true);
    if (AIManager* manager = GetAIManager())
        manager->ForgetSighting(entity);
    flags0 &= static_cast<uint8_t>(~kF0_Messaged);
    if (stateScript) {
        ReleaseAnimScript(stateScript);
        stateScript = nullptr;
    }
    if (state != newState)
        EntityResetAnimMode(entity);
    flags1 &= static_cast<uint8_t>(~kF1_HitLocked);
    stateRepeat = (state == newState) ? stateRepeat + 1 : 0;
    state = newState;
    stateStack[stateDepth] = newState;
    EnterState();
}

// @func 0x00014C78 AIController::EnterState
void AIController::EnterState()
{
    stateTime = 0.0f;
    stateRandom = ape::RandomUnit(ape::g_gameRandom);
    flags1 &= static_cast<uint8_t>(~kF1_ForceRethink);
}

// @func 0x00014CF8 AIController::UpdateState
void AIController::UpdateState(float dt)
{
    stateTime += dt;
}

// @func 0x00014D08 AIController::ExitState
void AIController::ExitState() {}

// TODO(0x0001199C AIController::CreateNavAgent): builds the NavPower agent
// from the controller transform (slots 15-18 and 61 supply its settings) and
// snaps the nav point to the agent position.
void AIController::CreateNavAgent() {}

// TODO(0x00015040 AIController::UpdateNavigation): follows the NavPower path
// (agent position/velocity queries, nav point update); needs the NavPower
// agent API.
void AIController::UpdateNavigation() {}

// @func 0x00014D0C AIController::PushState
void AIController::PushState(int newState)
{
    if (stateDepth + 1 < 4) {
        ++stateDepth;
        SetState(newState);
    }
}

// @func 0x00014D74 AIController::IsBodyKeyframed
bool AIController::IsBodyKeyframed() const
{
    return body && BodyMotionType(body) == 1;
}

// @func 0x0001574C AIController::AdvancePatrol
void AIController::AdvancePatrol()
{
    if (!proxyPoint)
        return;
    OnProxyPointReached();
    AIProxyPointTable* table = GetAIProxyPointTable();
    AIProxyPoint* point = proxyPoint;
    AIProxyPoint* head = table->AtIndex(point->head);
    const uint32_t pathType = head ? PropHash(static_cast<const PropertyBlock*>(head->properties),
                                              AIProp::ProxyPt_PathType)
                                   : 0;
    if (pathType == kPathType_Random) {
        proxyPoint = ProxyPointRandom(point);
        return;
    }
    if (pathType == kPathType_PingPong) {
        // Turn around at either end of the path.
        const bool reverse = (flags0 & kF0_PatrolReverse) != 0;
        const bool atEnd = !reverse ? point->prev == point->head
                                    : (point->head == point->self && point->prev != point->self);
        if (atEnd)
            flags0 = static_cast<uint8_t>((flags0 & ~kF0_PatrolReverse) | (reverse ? 0 : kF0_PatrolReverse));
    }
    if (!(flags0 & kF0_PatrolReverse)) {
        proxyPoint = table->AtIndex(point->prev);
    } else {
        proxyPoint = ProxyPointNext(point);
    }
}

// @func 0x00016088 AIController::OnProxyPointReached
void AIController::OnProxyPointReached()
{
    if (!proxyPoint)
        return;
    const PropertyBlock* props = static_cast<const PropertyBlock*>(proxyPoint->properties);
    const char* script = PropString(props, AIProp::ProxyPt_Script);
    const uint32_t scriptHash = script ? ape::HashNameNoCase(script) : 0;
    if (scriptHash != 0)
        StartOneShotScript(scriptHash);
    const uint32_t key = ProxyPointMessageKey();
    PostScriptMessage(entity, key, key, WorldObjectKeyOf(proxyPoint->object));
}

// @func 0x00016124 AIController::UpdateTargetInfo
void AIController::UpdateTargetInfo(float dt)
{
    (void)dt;
    if (target && target->IsAlive()) {
        const ape::Vec3 to = target->GetCenter();
        const ape::Vec3 from = entity->GetCenter();
        const float dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
        const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        targetDist = dist;
        targetDir = {dx / dist, dy / dist, dz / dist, 0.0f};
    }
    flags2 |= kF2_SightDirty;
}

// @func 0x00016218 AIController::SetTarget
void AIController::SetTarget(Entity* newTarget)
{
    if (!newTarget || !newTarget->IsAlive() || AITargetingSuppressed())
        return;
    if (target != newTarget && !IsAlly()) {
        if (target && target->IsAlive())
            target->Release(false);
        newTarget->AddRef();
    }
    EntitySetEngaged(entity, true);
    AnimControllerSetEngaged(entity, true);
    target = newTarget;

    const ape::Vec3 to = newTarget->GetCenter();
    if (newTarget == PlayerCharacter()) {
        if (AIManager* manager = GetAIManager()) {
            manager->lastPlayerPosition = to;
            manager->timeSinceSeen = 0.0f;
        }
    }
    const ape::Vec3 from = entity->GetCenter();
    const float dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    targetDist = dist;
    targetDir = {dx / dist, dy / dist, dz / dist, 0.0f};
}

// @func 0x0001647C AIController::IsSeeThroughLayer
bool AIController::IsSeeThroughLayer(int s)
{
    if ((s >= 1 && s <= 4) || s == 8 || s == 12 || s == 0x1C)
        return false;
    return true;
}

// @func 0x000164D4 AIController::IsTargetVisible
bool AIController::IsTargetVisible()
{
    if (flags2 & kF2_SightDirty) {
        bool visible = false;
        if (target) {
            const ape::Vec3 from = entity->GetCenter();
            const ape::Vec3 to = target->GetCenter();
            visible = TestLineOfSight(from, to, target);
        }
        flags2 = static_cast<uint8_t>((flags2 & ~(kF2_SightDirty | kF2_TargetVisible)) |
                                      (visible ? kF2_TargetVisible : 0));
    }
    return (flags2 & kF2_TargetVisible) != 0;
}

// @func 0x000138B0 AIController::TestLineOfSight
// The ray is blocked by anything except: layers the behaviour sees through,
// the controlled entity, `other` itself and AI-controlled entities (those
// with a listener). Entities of class bit 4 are never visible.
bool AIController::TestLineOfSight(const ape::Vec3& from, const ape::Vec3& to, Entity* other)
{
    if (other->classBits & 0x10)
        return false;
    physics::RayHit hits[4];
    const int count = physics::CastRayAll(from, to, hits, 4);
    for (int i = 0; i < count; ++i) {
        const physics::RayHit& hit = hits[i];
        if (IsSeeThroughLayer(int(hit.collisionLayer)))
            continue;
        if (!hit.entity)
            return false;   // level geometry
        if (hit.entity == entity || hit.entity == other || hit.entity->listener)
            continue;
        return false;
    }
    return true;
}

// @func 0x00016A40 AIController::UpdateMovement
void AIController::UpdateMovement(float dt)
{
    // Root motion of the playing animation, filtered and scaled.
    ape::Matrix34 delta;
    ape::Matrix34FromTranslation(delta, ape::Vec3{});
    EntityRootMotion(entity, dt, delta);
    if ((flags1 & kF1_FreeFacing) && (flags1 & kF1_FlatFacing)) {
        // Climb along the facing's pitch.
        delta.translation.y = (std::fabs(delta.translation.x) + std::fabs(delta.translation.z)) * facing.y;
    }
    ape::Vec3 step = FilterRootMotion(delta.translation);
    const ape::Vec3 scale = EntityObjectScale(entity);
    step.x *= scale.x;
    step.y *= scale.y;
    step.z *= scale.z;
    const float motionScale = RootMotionScale();
    delta.translation = {step.x * motionScale, step.y * motionScale, step.z * motionScale, 0.0f};
    // NOTE: in-place multiply 0x005541F0; delta assumed to be applied first.
    ape::Matrix34 moved;
    ape::world::Matrix34Mul(moved, delta, transform);
    transform = moved;

    // Turn the transform towards the facing direction.
    if (turnRate > 0.0f) {
        if (flags0 & kF0_Leashed) {
            if (DistanceSq(homePosition, transform.translation) < 1.0f)
                turnRate *= 1.1f;
            const float threshold = TurnInPlaceAngle();
            if (threshold > 0.0f && std::fabs(SignedAngle(facing, transform.zAxis)) > threshold)
                OnTurnInPlace();
        }
        const float maxStep = turnRate * dt;
        const float facingInv = 1.0f / std::sqrt(facing.x * facing.x + facing.z * facing.z);
        const float zInv = 1.0f / std::sqrt(transform.zAxis.x * transform.zAxis.x +
                                            transform.zAxis.z * transform.zAxis.z);
        const ape::Vec3 flatFacing = {facing.x * facingInv, 0.0f, facing.z * facingInv, 0.0f};
        const ape::Vec3 flatZ = {transform.zAxis.x * zInv, 0.0f, transform.zAxis.z * zInv, 0.0f};
        float angle = SignedAngle(flatFacing, flatZ) * kRadToDeg;
        angle = std::fmax(std::fmin(angle, maxStep), -maxStep);
        ape::Matrix34 rotation;
        ape::MatrixAxisAngle(angle * kDegToRad, rotation, kUp);
        rotation.translation = ape::Vec3{};
        ape::Matrix34 turned;
        ape::world::Matrix34Mul(turned, rotation, transform);
        transform = turned;
    }

    if (!body || !BodyIsEnabled(body)) {
        Orthonormalize(transform);
        if (WritesEntityTransform())
            entity->transform = transform;
        else
            EntitySetTransform(entity, transform);
        return;
    }
    // Physics driven: ask the body for the velocity that gets it here.
    const float inv = 1.0f / dt;
    const ape::Vec3 bodyPos = BodyPosition(body);
    float vy = (transform.translation.y - bodyPos.y) * inv;
    if (ShouldApplyGravity({0.0f, vy, 0.0f, 0.0f}))
        vy += GravitySpeed();
    BodyRequestVelocity(body, {(transform.translation.x - bodyPos.x) * inv, vy,
                               (transform.translation.z - bodyPos.z) * inv, 0.0f});
    bodyMotion = BodyMotionType(body);
}

// -- AIManager ---------------------------------------------------------------------------

// @func 0x000186CC AIManager::AIManager
AIManager::AIManager()
{
    SetAIManager(this);
    // The knowledge system is a token object; the knowledge module keeps it.
    knowledge::CreateKnowledgeSystem(new KnowledgeSystemToken());
    // Anim sets of the AI: 256 slots, the AI control types and one sync type,
    // with the anim event knowledge callbacks.
    // NOTE: the console also installs an empty callback (field 0x11C) and the
    // slot release 0x000653CC (not ported).
    g_aiAnimClips = new ape::anim::AnimClipManager(0x100, kAIControlTypes, kAIControlTypeCount, kAISyncTypes, 1,
                                                   AnimEventKnowledgeUserData, nullptr,
                                                   AnimEventKnowledgeAcquireSlot, nullptr);
    SetAIProxyPointTable(new AIProxyPointTable(0x200));
    CreateNavManager();
    SetAIPriorityTargets(&priorityTargets);
}

// @func 0x00018B30 ShutdownAISystems
void ShutdownAISystems()
{
    SetAIManager(nullptr);
    DestroyNavManager();
    if (AIProxyPointTable* table = GetAIProxyPointTable()) {
        SetAIProxyPointTable(nullptr);
        delete table;
    }
    delete g_aiAnimClips;
    g_aiAnimClips = nullptr;
    if (g_knowledgeToken) {
        knowledge::DestroyKnowledgeSystem();
        delete g_knowledgeToken;
        g_knowledgeToken = nullptr;
    }
}

ape::anim::AnimClipManager* AIAnimClips() { return g_aiAnimClips; }


// @func 0x00018DD0 AIManager::Init
void AIManager::Init()
{
    knowledge::KnowledgeInit();
    if (g_aiAnimClips)
        g_aiAnimClips->LoadAnimSetFiles();
    controllerCount = 0;
    activeCount = 0;
    thinkCursor = 0;
    allyCount = 0;
    alert = false;
    field_44 = 0;
    field_48 = 0;
    timeSinceSeen = 900.0f;
    lastPlayerPosition = {0.0f, 0.0f, 0.0f, 0.0f};
    field_24 = 0;
    // The surround directions: forward turned about the up axis in eighths
    // of a full turn.
    constexpr float kTwoPi = 6.2831854820251465f;
    for (int i = 0; i < kRecord50Count; ++i) {
        ape::Matrix34 turn;
        ape::MatrixAxisAngle(float(i) * 0.125f * kTwoPi, turn, kUp);
        const ape::Vec3 forward = {0.0f, 0.0f, 1.0f, 0.0f};   // NOTE: console global 0x01164B70, assumed +Z
        ape::Vec3& dir = records50[i].direction;
        dir.x = forward.x * turn.xAxis.x + forward.y * turn.yAxis.x + forward.z * turn.zAxis.x;
        dir.y = forward.x * turn.xAxis.y + forward.y * turn.yAxis.y + forward.z * turn.zAxis.y;
        dir.z = forward.x * turn.xAxis.z + forward.y * turn.yAxis.z + forward.z * turn.zAxis.z;
    }
    ResetSightings();
}

// @func 0x000191B0 AIManager::OnLevelLoaded
void AIManager::OnLevelLoaded()
{
    if (AIProxyPointTable* table = GetAIProxyPointTable())
        table->BuildChains();
    killHeight = LevelFloorDepth() * -0.5f + 20.0f;
}

// @func 0x0001A7C0 AIManager::PrunePriorityTargets
void AIManager::PrunePriorityTargets()
{
    priorityTargets.RemoveDead();
}

// @func 0x0001AC80 AIManager::PickTarget
Entity* AIManager::PickTarget(const AIController* controller) const
{
    if (targetingPause > 0.0f)
        return nullptr;
    const ape::Vec3& from = controller->transform.translation;
    const int targetCount = priorityTargets.Count();
    if (targetCount > 0) {
        Entity* best = nullptr;
        float bestDist = 1e+06f;
        for (int i = 0; i < targetCount; ++i) {
            Entity* candidate = AsEntity(priorityTargets.At(i));
            const float d = DistanceSq(candidate->Position(), from);
            if (d < bestDist) {
                bestDist = d;
                // Only player type entities count; a nearer other one blocks.
                best = (candidate->typeBits[0] & 0x2u) ? candidate : nullptr;
            }
        }
        return best;
    }
    if (allyCount == 1)
        return allies[0];
    Entity* best = PlayerCharacter();
    if (!best)
        return nullptr;
    float bestDist = DistanceSq(best->Position(), from);
    for (int i = 0; i < allyCount; ++i) {
        const float d = DistanceSq(allies[i]->Position(), from);
        if (d < bestDist) {
            bestDist = d;
            best = allies[i];
        }
    }
    return best;
}

// @func 0x0001B2F4 AIManager::SetEntitiesVisible
void AIManager::SetEntitiesVisible(bool visible)
{
    for (int i = 0; i < controllerCount; ++i) {
        if (!controllers[i]->IsAlly())
            controllers[i]->entity->SetVisibilityDeferred(visible, false);
    }
}

// @func 0x0001B3BC AIManager::BroadcastScriptCommand
void AIManager::BroadcastScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value)
{
    for (int i = 0; i < activeCount; ++i)
        active[i]->OnScriptCommand(command, arg1, arg2, value);
}

// @func 0x0001B4A0 AlertEnemies
void AlertEnemies(Entity* source, int noise, bool loud)
{
    constexpr uint32_t kGroup_Enemies = 0xCB186A06u;
    constexpr uint32_t kEvent_Noise = 0x795BCF46u;       // NOTE: event names unknown
    constexpr uint32_t kEvent_LoudNoise = 0x39EE3E3Au;
    float range = 0.0f;
    if (noise == kNoise_Normal)
        range = 15.0f;
    else if (noise == kNoise_Footsteps)
        range = g_aiAudioAlertRangeFootsteps;
    else if (noise == kNoise_Bottle)
        range = g_aiAudioAlertRangeBottle;
    BroadcastInRange(source, range, kGroup_Enemies, loud ? kEvent_LoudNoise : kEvent_Noise, 0);
}

// @func 0x0001BE50 AIThreatState::ConsumeChaseVO
bool AIThreatState::ConsumeChaseVO()
{
    const bool ready = chaseVOTimer < 0.0f;
    if (ready)
        chaseVOTimer = g_aiChaseVOFrequency;
    return ready;
}

// @func 0x0001BE8C AIThreatState::RestartAttackRecovery
void AIThreatState::RestartAttackRecovery()
{
    attackRecovery = g_aiAttackPlayerRecoveryTime;
}

// @func 0x0001BEA0 AIThreatState::NotifyListener
void AIThreatState::NotifyListener()
{
    if (listener)
        listener->OnNotify();
}

// @func 0x0001BEE8 AIThreatState::NotifyListener (value)
void AIThreatState::NotifyListener(uint32_t value)
{
    if (listener)
        listener->OnValue(value);
}

void AIManager::ResetSightings()
{
    sightingCount = 0;
    for (int i = 0; i < kMaxSightings; ++i)
        sightings[i] = &sightingPool[i];
}

// @func 0x0001A840 AIManager::Reset
void AIManager::Reset()
{
    if (AIProxyPointTable* table = GetAIProxyPointTable())
        table->Clear();
    AIObjectPoolReset();
    KnowledgeReset();
    if (NavManager* nav = GetNavManager())
        ResetNavManager(*nav);
    ResetSightings();
}

// @func 0x0001AA34 AIManager::OnLevelStart
void AIManager::OnLevelStart()
{
    controllerCount = 0;
    activeCount = 0;
    KnowledgeLevelStart();
    AIObjectPoolLevelStart();
    ResetSightings();
}

// @func 0x0001AB3C AIManager::OnLevelEnd
void AIManager::OnLevelEnd()
{
    controllerCount = 0;
    activeCount = 0;
    if (AIProxyPointTable* table = GetAIProxyPointTable())
        table->Clear();
    KnowledgeLevelEnd();
    AIObjectPoolLevelEnd();
    ResetSightings();
}

namespace {

ape::Vec3 TransformPoint(const ape::Matrix34& m, const ape::Vec3& v)
{
    return {v.x * m.xAxis.x + v.y * m.yAxis.x + v.z * m.zAxis.x + m.translation.x,
            v.x * m.xAxis.y + v.y * m.yAxis.y + v.z * m.zAxis.y + m.translation.y,
            v.x * m.xAxis.z + v.y * m.yAxis.z + v.z * m.zAxis.z + m.translation.z, 0.0f};
}

// Frame an attack volume follows: its bone, else the attacker.
ape::Matrix34 AttackFrame(const AISighting& attack)
{
    if (attack.bone < 0)
        return attack.entity->transform;
    return attack.entity->BoneTransform(attack.bone, true, true);
}

// Class bit of the attacker that marks hits as coming from a player.
// NOTE: bit 0 of the class bits, read as "player".
constexpr uint64_t kClass_Player = 0x01;

} // namespace

// @func 0x0001B8C0 AIManager::AddAttack
void AIManager::AddAttack(Entity* attacker, Entity* target, uint32_t attachName, const AIAttackDesc& desc)
{
    if (sightingCount + 1 >= kMaxSightings)
        return;
    AISighting& attack = *sightings[sightingCount];
    attack.entity = attacker;
    attack.target = target;
    attack.damage = desc.damage;
    attack.push = desc.push;
    attack.radius = desc.radius;
    attack.offset = desc.offset;
    attack.kind = attachName;
    // A locator of the attacker's object names the point relative to its
    // bone; otherwise the name is a bone.
    ape::world::ApeLocator* locator = nullptr;
    if (ape::world::ApeDef* def = attacker->object ? attacker->object->GetDef() : nullptr) {
        if (def->m_locators)
            locator = def->m_locators->Find(attachName);
    }
    if (locator) {
        attack.offset = TransformPoint(locator->transform, attack.offset);
        attack.bone = locator->bone;
    } else {
        attack.bone = attacker->FindBone(attachName);
    }
    attack.field_34 = desc.field_34;
    attack.reaction = desc.reaction;
    attack.position = TransformPoint(AttackFrame(attack), attack.offset);
    ++sightingCount;
}

// @func 0x000193C0 AIManager::UpdateAttacks
void AIManager::UpdateAttacks()
{
    for (int i = 0; i < sightingCount; ++i) {
        AISighting& attack = *sightings[i];
        bool done = true;
        if (attack.target->IsAlive()) {
            done = false;
            const ape::Vec3 targetCenter = attack.target->GetCenter();
            const ape::Vec3 previous = attack.position;
            attack.position = TransformPoint(AttackFrame(attack), attack.offset);
            // Nearest point of this frame's sweep to the target.
            const ape::Vec3 nearest = ClosestPointOnSegment(attack.position, previous, targetCenter);
            const ape::Vec3 toTarget = {targetCenter.x - nearest.x, targetCenter.y - nearest.y,
                                        targetCenter.z - nearest.z, 0.0f};
            bool reached;
            if (entity::EntityComponent* body = attack.target->GetComponent00()) {
                // NOTE: the vector the hook returns is used as the body's
                // radius (x) and height (y) here.
                const ape::Vec3 extent = entity::PhysicsCompUpVector(body);
                reached = std::fabs(toTarget.y) < extent.y * 0.5f + attack.radius &&
                          toTarget.x * toTarget.x + toTarget.z * toTarget.z <
                              (extent.x + attack.radius) * (extent.x + attack.radius);
            } else {
                const float reach = attack.radius + 0.5f;
                reached = toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z < reach * reach;
            }
            // NOTE: the target is taken to be a scripted entity, as on the console.
            auto* victim = static_cast<entity::ScriptedEntity*>(attack.target);
            if (reached && !victim->IsInvulnerable()) {
                const float length = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y +
                                               toTarget.z * toTarget.z);
                const float inv = 1.0f / length;
                entity::EntityHitRecord hit;
                hit.source = attack.entity;
                hit.reaction = attack.reaction;
                hit.damageBits = 0;
                hit.flags = uint16_t(1u << 13);   // melee
                if (attack.entity && (attack.entity->classBits & kClass_Player))
                    hit.flags |= entity::EntityHitRecord::kHit_FromPlayer;
                hit.vec10 = targetCenter;
                hit.data.impact.direction = {toTarget.x * inv, toTarget.y * inv, toTarget.z * inv, 0.0f};
                hit.data.impact.direction2 = {0.0f, 0.0f, 0.0f, 0.0f};
                hit.data.impact.damage = attack.damage;
                hit.data.impact.push = attack.push;
                hit.data.impact.flags_48 = 0;
                hit.data.impact.field_4C = 0;
                victim->OnHitCategory1(hit);
                done = true;
            }
        }
        if (done) {
            --sightingCount;
            std::swap(sightings[i], sightings[sightingCount]);
            --i;
        }
    }
}

// @func 0x00019210 AIManager::ResolveJumpLinks
void AIManager::ResolveJumpLinks()
{
    if (NavManager* nav = GetNavManager())
        ResolveJumpLinkObjects(*nav);
}

// @func 0x0001AC68 AIManager::ClearSightings
void AIManager::ClearSightings()
{
    sightingCount = 0;
}

// @func 0x00734470 PlayerCharacter
Entity* PlayerCharacter()
{
    Entity* player = entity::PlayerEntity(0);
    return player && player->HasTypeBit(1) ? player : nullptr;
}

} // namespace ai
} // namespace levels
} // namespace sly
