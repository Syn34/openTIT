// openTIT - Ms Decibel boss behaviour (vtable 0x00B0EAF8), partial.
#include "sly/levels/ai/AIBehaviours.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeInstances.h"

#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/ai/AITuning.h"
#include "sly/entity/Entity.h"

#include <cmath>

namespace sly {
namespace levels {
namespace ai {

namespace {
constexpr uint32_t kCmd_Phase3 = 0x0464F605u;   // NOTE: command names unknown
constexpr uint32_t kCmd_Phase8 = 0x8839CACDu;
constexpr int kPhase_Ground = 9;               // NOTE: the phase in which gravity applies
} // namespace

// @func 0x000575C4 MsDecibelAI::GetTargetPosition
ape::Vec3 MsDecibelAI::GetTargetPosition()
{
    return AIController::GetTargetPosition();
}

// @func 0x00057778 MsDecibelAI::GravitySpeed
float MsDecibelAI::GravitySpeed()
{
    if (phase == kPhase_Ground && state != 0x0B && state != 0x0A && state != 0x0C)
        return -10.0f;
    return 0.0f;
}

// @func 0x000577C0 MsDecibelAI::OnTauntChance
void MsDecibelAI::OnTauntChance()
{
    if (state == 0x13)
        SetState(0x15);
    else if (state == 0x0E)
        SetState(0x0F);
}

// @func 0x0005B964 MsDecibelAI::OnScriptCommand
void MsDecibelAI::OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value)
{
    (void)arg1; (void)arg2; (void)value;
    entity->SetActive(false);
    if (command == kCmd_Phase3)
        SetPhase(3);
    else if (command == kCmd_Phase8)
        SetPhase(8);
}

// @func 0x0005BB08 MsDecibelAI::Start
void MsDecibelAI::Start()
{
    started = true;
}

// @func 0x0005BA08 MsDecibelAI::SetSplineStretch
void MsDecibelAI::SetSplineStretch(uint32_t from, uint32_t to)
{
    stretchFrom = ape::world::FindObject(from, ape::world::kObjType_PointInstance);
    stretchTo = ape::world::FindObject(to, ape::world::kObjType_PointInstance);
    if (!spline)
        return;
    stretchStart = stretchFrom ? spline->FindClosestParam(stretchFrom->GetLocal().translation, nullptr, nullptr) : 0.0f;
    stretchEnd = stretchTo ? spline->FindClosestParam(stretchTo->GetLocal().translation, nullptr, nullptr) : 1.0f;
}

// TODO(0x00053A80 MsDecibelAI::SetPhase)
void MsDecibelAI::SetPhase(int newPhase)
{
    phase = newPhase;
}

// @func 0x000544B8 MsDecibelAI::CheckCounterAttack
void MsDecibelAI::CheckCounterAttack()
{
    if (state == 9 || state == 5 || state == 11 || state == 12)
        return;
    Entity* player = PlayerCharacter();
    if (!player)
        return;
    const float dx = player->transform.translation.x - Position().x;
    const float dz = player->transform.translation.z - Position().z;
    if (std::sqrt(dz * dz + dx * dx) >= g_msDecibelPlatformCounterAttackDist)
        return;
    if (counterAttacks > 0)
        --counterAttacks;
    SetState(9);
}

} // namespace ai
} // namespace levels
} // namespace sly
