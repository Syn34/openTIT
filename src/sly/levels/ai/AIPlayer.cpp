// openTIT - Player buddy behaviour (vtable 0x00B0E1B8).
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "sly/levels/ai/AIComponent.h"
#include "sly/levels/ai/AIHooks.h"

#include <cmath>

namespace sly {
namespace levels {
namespace ai {

namespace {

const ape::Vec3 kUp = {0.0f, 1.0f, 0.0f, 0.0f};   // 0x01164B60

constexpr uint32_t kClip_Sprint = 0x922FF61Au;           // "Sprint"
constexpr uint32_t kScriptState_Release = 0x0C544DCFu;   // NOTE: name unknown

// Buddy messages (EntityMessage::id). NOTE: names unknown.
constexpr uint32_t kMsg_SetFlag40 = 0x00764FA9u;
constexpr uint32_t kMsg_ClearFlag40 = 0x7D2BB408u;
constexpr uint32_t kMsg_SetFlag20 = 0x50A63D95u;
constexpr uint32_t kMsg_Mode0 = 0xFAE875CEu;
constexpr uint32_t kMsg_Mode1 = 0x632B318Bu;
constexpr uint32_t kMsg_Mode2 = 0xB32B479Bu;
constexpr uint32_t kMsg_Mode3 = 0x5A7E7E77u;
constexpr uint32_t kMsg_Warp = 0xDD92E72Bu;

} // namespace

// @func 0x00024048 PlayerAI::SetMode
void PlayerAI::SetMode(int newMode)
{
    if (newMode == 0 || newMode == 1)
        flags1 |= kF1_HitLocked;
    else if (newMode == 2 || newMode == 3)
        flags1 &= static_cast<uint8_t>(~kF1_HitLocked);
    mode = newMode;
}

// @func 0x000240C0 PlayerAI::Attach
void PlayerAI::Attach(Entity* newEntity)
{
    auto* component = static_cast<AIComponent*>(newEntity->GetComponent0D());
    settings = component ? component->GetSettings() : nullptr;
    AIController::Attach(newEntity);
    character = (newEntity && (newEntity->typeBits[0] & 0x8u)) ? newEntity : nullptr;
    EntitySetEngaged(entity, true);
    AnimControllerSetEngaged(entity, true);
    field_150 = 0;
    field_154 = 20.0f;
    field_170 = 0;
    field_178 = 0;
    buddyFlags &= static_cast<uint8_t>(~(kBuddy_Flag20 | kBuddy_Flag40));
    startPosition = transform.translation;
    SetMode(1);
    SetState(2);
}

void PlayerAI::PopState()
{
    if (stateDepth != 0)
        --stateDepth;
    SetState(stateStack[stateDepth]);
}

// @func 0x00024230 PlayerAI::SetActive
void PlayerAI::SetActive(bool active)
{
    if (active)
        return;
    if (state == 1) {
        PopState();
    } else if (state == 0x0C) {
        SetState(0x0D);
        PopState();
        EntitySetScriptState(entity, kScriptState_Release);
    } else if (state == 0x0D) {
        PopState();
        EntitySetScriptState(entity, kScriptState_Release);
    }
}

// @func 0x000243D0 PlayerAI::Update
void PlayerAI::Update(float dt)
{
    // The buddy follows its physics body.
    if (body)
        ape::Matrix34FromRotationTranslation(transform, BodyRotation(body), BodyPosition(body));
    if ((entity->flags & (0x40u | 0x20u)) || (entity->classBits & 1u))
        return;
    PreUpdate(dt);
    if (!Think(dt) && characterComp &&
        ((flags1 & kF1_ForceRethink) || CharacterClipFinished(characterComp) || IsWithinLeash()))
        ChooseState();
    UpdateState(dt);
    UpdateNavigation();
    lastPosition = transform.translation;
    field_158 = 1;
}

// @func 0x00024660 PlayerAI::PreUpdate
void PlayerAI::PreUpdate(float dt)
{
    (void)dt;
}

// @func 0x000248D0 PlayerAI::OnMessage
void PlayerAI::OnMessage(const entity::EntityMessage& msg)
{
    switch (msg.id) {
    case kMsg_SetFlag40:
        buddyFlags |= kBuddy_Flag40;
        if (state == 5)
            PopState();
        break;
    case kMsg_Mode0:
        mode = 0;
        break;
    case kMsg_Warp: {
        bool warped = false;
        for (int i = 0; i < stateDepth; ++i) {
            if (stateStack[i] == 0x0B) {
                WarpTo(warpPoint);
                warped = true;
                break;
            }
        }
        if (!warped)
            WarpTo(state == 7 ? navGoal : homePosition);
        break;
    }
    case kMsg_Mode2:
        mode = 2;
        break;
    case kMsg_ClearFlag40:
        buddyFlags &= static_cast<uint8_t>(~kBuddy_Flag40);
        break;
    case kMsg_Mode1:
        mode = 1;
        break;
    case kMsg_Mode3:
        mode = 3;
        break;
    case kMsg_SetFlag20:
        buddyFlags |= kBuddy_Flag20;
        break;
    default:
        break;
    }
    AIController::OnMessage(msg);
}

// @func 0x00024670 PlayerAI::WarpTo
void PlayerAI::WarpTo(const ape::Vec3& point)
{
    stateDepth = 0;
    SetState(stateStack[0]);
    SetFacing({point.x - transform.translation.x, point.y - transform.translation.y,
               point.z - transform.translation.z, 0.0f});
    const ape::Vec3 z = facing;
    transform.xAxis = {kUp.y * z.z - kUp.z * z.y, kUp.z * z.x - kUp.x * z.z, kUp.x * z.y - kUp.y * z.x, 0.0f};
    transform.yAxis = kUp;
    transform.zAxis = z;
    transform.translation = {kUp.x * 3.0f + point.x - z.x, kUp.y * 3.0f + point.y - z.y,
                             kUp.z * 3.0f + point.z - z.z, 0.0f};
    EntitySetTransform(entity, transform);
    BodySync(body);
    navGoal = point;
    OnNavGoalChanged();
}

// @func 0x00024AD8 PlayerAI::OnDamaged
void PlayerAI::OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info)
{
    (void)kind;
    if (amount > 0.0f)
        EntityDamage(character, amount, info->source, 0);
}

// @func 0x00024FCC PlayerAI::StateName
const char* PlayerAI::StateName(int s)
{
    return ape::TempFormat("%d", s);
}

// @func 0x00024FF8 PlayerAI::IsSettled
bool PlayerAI::IsSettled()
{
    if (stateStack[0] != 9)
        return true;
    const float dx = homePosition.x - transform.translation.x;
    const float dy = homePosition.y - transform.translation.y;
    const float dz = homePosition.z - transform.translation.z;
    // NOTE: the console compares the squared distance with the radius itself.
    return dz * dz + dx * dx + dy * dy < leashRadius;
}

// @func 0x0002550C PlayerAI::RefreshComponents
void PlayerAI::RefreshComponents()
{
    characterComp = entity->GetCharacterComponent();
    body = entity->GetComponent00();
}

// @func 0x00025598 PlayerAI::ChooseState
void PlayerAI::ChooseState()
{
    if (state == 1) {
        PlayClip(currentClip);
    } else if (state == 4 && CharacterClipFinished(characterComp)) {
        PlayClip(kClip_Sprint);
    }
}

// @func 0x000256D0 PlayerAI::Think
bool PlayerAI::Think(float dt)
{
    (void)dt;
    return false;
}

// @func 0x0002742C PlayerAI::ExitState
void PlayerAI::ExitState()
{
    switch (state) {
    case 1:
    case 0x0D:
        if (EntityComponent* comp = entity->GetComponent(0x24))
            Component24Reset(comp);
        if (EntityComponent* comp = entity->GetComponent(0x03))
            comp->SetOption(true);
        break;
    case 4:
        flags0 &= static_cast<uint8_t>(~(kF0_Leashed | kF0_NavMoving));
        BodyClearRequestBit31(body);
        break;
    case 2: case 3: case 5: case 8: case 9:
        flags0 &= static_cast<uint8_t>(~(kF0_Leashed | kF0_NavMoving));
        break;
    case 10:
        flags0 &= static_cast<uint8_t>(~kF0_Leashed);
        break;
    default:
        break;
    }
}

// @func 0x00027588 PlayerAI::OnNavGoalChanged
void PlayerAI::OnNavGoalChanged()
{
    SetFacing({navGoal.x - transform.translation.x, navGoal.y - transform.translation.y,
               navGoal.z - transform.translation.z, 0.0f});
    if (stateDepth + 1 < 4)
        ++stateDepth;
    SetState(6);
}

} // namespace ai
} // namespace levels
} // namespace sly
