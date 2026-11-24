// openTIT - Sentry behaviour (vtable 0x00B0E750).
#include "sly/levels/ai/AIBehaviours.h"

#include "sly/levels/ai/AIComponent.h"
#include "sly/levels/ai/AIHooks.h"

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr uint32_t kGroup_Enemies = 0xCB186A06u;
constexpr uint32_t kGroup_Sentries = 0x4908BDCFu;   // the behaviour's own type hash
constexpr uint32_t kEvent_SentryHit = 0x59C04B13u;  // NOTE: event name unknown
constexpr uint32_t kSentryDefault144 = 0x002889C9u; // NOTE: meaning unknown
constexpr int kState_Hit = 4;

} // namespace

// @func 0x0003C090 SentryAI::Attach
void SentryAI::Attach(Entity* newEntity)
{
    auto* component = static_cast<AIComponent*>(newEntity->GetComponent0D());
    settings = component ? component->GetSettings() : nullptr;
    AIController::Attach(newEntity);
    field_13D = 0;
    flags0 &= static_cast<uint8_t>(~kF0_PatrolReverse);
    field_13E = 1;
    AddToListenerGroup(entity, kGroup_Enemies, true);
    AddToListenerGroup(entity, kGroup_Sentries, true);
    CreateNavAgent();
    alertReceiver = nullptr;
    settled = 0;
    idleClip = PropHash(settings, AIProp::Sentry_IdleAnimClip);
    field_144 = kSentryDefault144;
    field_1C = 0;
    SetState(0);
}

// @func 0x0003C1BC SentryAI::Update
void SentryAI::Update(float dt)
{
    if (!(entity->flags & (0x40u | 0x20u)))
        AIController::Update(dt);
}

// @func 0x0003CF00 SentryAI::OnDamaged
void SentryAI::OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info)
{
    (void)kind;
    if (amount > 0.0f && (info->flags & 0x1000u)) {
        SetState(kState_Hit);
        if (alertReceiver)
            SendGameEvent(entity, alertReceiver, kEvent_SentryHit, 0, true);
    }
}

// @func 0x0003CFB0 SentryAI::GravitySpeed
float SentryAI::GravitySpeed()
{
    const bool supported = character && (character->classBits & 0x4000u);
    if (!supported && BodyMotionType(body) != 1)
        return -10.0f;
    return 0.0f;
}

// @func 0x0003D000 SentryAI::CanBeSneakedUpOn
bool SentryAI::CanBeSneakedUpOn()
{
    if (!PropBool(settings, AIProp::Sentry_SneakAllowed))
        return false;
    return !(flags0 & kF0_Bit0);
}

// @func 0x0003D044 SentryAI::IsSettled
bool SentryAI::IsSettled()
{
    return state != 0 || settled != 0;
}

// @func 0x0003D068 SentryAI::ShouldApplyGravity
bool SentryAI::ShouldApplyGravity(const ape::Vec3& velocity)
{
    if (PropBool(settings, AIProp::Sentry_AlwaysApplyGravity))
        return true;
    return velocity.y < 1e-05f;
}

// @func 0x0003DE48 SentryAI::ExitState
void SentryAI::ExitState()
{
    field_13E = 1;
    settled = 0;
    if (state == 1 || state == 2)
        flags0 &= static_cast<uint8_t>(~(kF0_Leashed | kF0_NavMoving));
    else if (state == 3)
        field_13D = 0;
    else if (state == 4)
        GameStateNotify();
}

} // namespace ai
} // namespace levels
} // namespace sly
