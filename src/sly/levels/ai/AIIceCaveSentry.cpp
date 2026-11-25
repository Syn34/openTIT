// openTIT - Grizz's ice cave sentry (SentryAIVariant, vtable 0x00B0EDA0).
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMath.h"
#include "sly/entity/Entity.h"
#include "sly/game/GameEvent.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AITuning.h"

#include <cmath>

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr int kState_Idle = 1;
constexpr int kState_Alerted = 4;   // NOTE: entered when damage with flag bit 12 lands

constexpr uint32_t kClip_Impatient = 0x81DD9D41u;   // NOTE: clip names unknown
constexpr uint32_t kClip_Look = 0x0025695Bu;

// Component broadcasts (entity slot 32) on a hit.
constexpr uint32_t kMode_ButtHit = 0x5640C070u;
constexpr uint32_t kMode_Deflected = 0xA0885876u;

constexpr uint32_t kEvent_Alerted = 0x59C04B13u;

// Horizontal unit direction from `from` to `to`.
ape::Vec3 FlatDirection(const ape::Vec3& from, const ape::Vec3& to)
{
    const float dx = to.x - from.x;
    const float dz = to.z - from.z;
    const float inv = 1.0f / std::sqrt(dx * dx + dz * dz);
    return { dx * inv, 0.0f, dz * inv, 0.0f };
}

float RandomActionDelay()
{
    return ape::RandomRange(g_bismarqueIceCaveMinTimeBetweenActions, g_bismarqueIceCaveMaxTimeBetweenActions,
                            ape::g_gameRandom);
}

} // namespace

// @func 0x00061DC8 SentryAIVariant::Attach
void SentryAIVariant::Attach(Entity* newEntity)
{
    diggingMachine = nullptr;
    buttHits = 0;
    for (Entity*& dart : darts)
        dart = nullptr;
    actionTimer = RandomActionDelay();
    SentryAI::Attach(newEntity);
}

// @func 0x00061E98 SentryAIVariant::Detach
void SentryAIVariant::Detach()
{
    for (Entity*& dart : darts) {
        if (!dart)
            continue;
        entity->DetachChild(dart);
        dart->Release(true);
        dart = nullptr;
    }
    // NOTE: the reference is dropped but the pointer is only cleared by the
    // next Attach.
    if (diggingMachine)
        diggingMachine->Release(false);
    AIController::Detach();
}

// @func 0x00061FA0 SentryAIVariant::Update
void SentryAIVariant::Update(float dt)
{
    if (!diggingMachine) {
        diggingMachine = entity::FindEntity(g_bismarqueIceCaveDiggingMachineInst.Get());
        if (diggingMachine)
            diggingMachine->AddRef();
        else
            (void)ape::TempHexString(g_bismarqueIceCaveDiggingMachineInst.Get());   // console warning text
    }

    if (state == kState_Idle) {
        actionTimer -= dt;
        if (actionTimer <= 0.0f) {
            const float impatient = g_bismarqueIceCaveImpatientAnimationWeight;
            const float look = g_bismarqueIceCaveLookForPlayerWeight;
            const float total = impatient + look + g_bismarqueIceCaveApproachPlayerWeight;
            const float pick = total * ape::RandomUnit(ape::g_gameRandom);
            const ape::Vec3& position = transform.translation;

            if (pick < impatient || g_bismarqueIceCaveDebugButt) {
                // Impatient: turn towards the digging machine.
                ape::Vec3 direction = transform.zAxis;
                field_1C = 1;
                SetState(0);
                if (CharacterPlayClip(characterComp, kClip_Impatient))
                    currentClip = kClip_Impatient;
                if (diggingMachine)
                    direction = FlatDirection(position, diggingMachine->Position());
                facing = direction;
            } else if (pick < impatient + look) {
                // Look around: away from the machine, or towards the player.
                ape::Vec3 direction = transform.zAxis;
                field_1C = 1;
                SetState(0);
                if (CharacterPlayClip(characterComp, kClip_Look))
                    currentClip = kClip_Look;
                if (!g_bismarqueIceCaveSearchTowardsPlayer) {
                    if (diggingMachine)
                        direction = FlatDirection(diggingMachine->Position(), position);
                } else if (Entity* player = PlayerCharacter()) {
                    direction = FlatDirection(position, player->Position());
                }
                facing = direction;
            }
            // Otherwise (approach) the sentry simply carries on.
            actionTimer = RandomActionDelay();
        }
    }

    // NOTE: with "DebugButt" the console also computes the butt bone
    // transform here for a debug draw; nothing is drawn in this build.

    if (!(entity->flags & 0x40u) && !(entity->flags & 0x20u))
        AIController::Update(dt);
}

// @func 0x000624E0 SentryAIVariant::OnDamaged
// Positional hits only count when they land on the butt: a capsule of
// "ButtRadius" around a segment of "ButtLength" along the z axis of the
// "ButtBone" bone.
void SentryAIVariant::OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info)
{
    (void)kind;
    if ((info->flags >> 13) == 4 && (info->field_0C & 0x100)) {
        const ape::Matrix34 bone = EntityBoneTransform(entity, g_bismarqueIceCaveButtBone.Get());
        const float length = g_bismarqueIceCaveButtLength;
        const float half = length * 0.5f;
        const ape::Vec3 start = { bone.translation.x - bone.zAxis.x * half, bone.translation.y - bone.zAxis.y * half,
                                  bone.translation.z - bone.zAxis.z * half, 0.0f };
        const ape::Vec3 axis = { bone.zAxis.x * length, bone.zAxis.y * length, bone.zAxis.z * length, 0.0f };
        const ape::Vec3 rel = { info->position.x - start.x, info->position.y - start.y,
                                info->position.z - start.z, 0.0f };
        const float t = (rel.x * axis.x + rel.y * axis.y + rel.z * axis.z) / (length * length);
        bool onButt = false;
        if (t >= 0.0f && t <= 1.0f) {
            const float dx = rel.x - t * axis.x;
            const float dy = rel.y - t * axis.y;
            const float dz = rel.z - t * axis.z;
            const float radius = g_bismarqueIceCaveButtRadius;
            onButt = dx * dx + dy * dy + dz * dz < radius * radius;
        }
        // NOTE: slot 32 broadcasts the hash to the entity's components (the
        // port names it SetTimeScale).
        if (onButt) {
            ++buttHits;
            entity->SetTimeScale(kMode_ButtHit, true);
            GameEvent event;
            InitEvent1B(event, AsWorldObject(entity), 0, 0, 0);
            GameWorldPostEvent(0, event);
        } else {
            entity->SetTimeScale(kMode_Deflected, true);
        }
        flags1 &= ~kF1_Hit;
    } else if (amount > 0.0f && (info->flags & 0x1000)) {
        SetState(kState_Alerted);
        if (alertReceiver)
            SendGameEvent(entity, alertReceiver, kEvent_Alerted, 0, true);
    }
}

// @func 0x000627BC SentryAIVariant::IsStateInterruptible
bool SentryAIVariant::IsStateInterruptible(int s)
{
    switch (s) {
    case 1: case 2: case 3: case 4:
    case 7: case 8:
    case 12:
    case 28:
        return false;
    default:
        return true;
    }
}

} // namespace ai
} // namespace levels
} // namespace sly
