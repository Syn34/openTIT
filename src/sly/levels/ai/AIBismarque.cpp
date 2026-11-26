// openTIT - Bismarque boss behaviour (vtable 0x00B0E888), partial.
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/world/ApeObj.h"
#include "sly/levels/ai/AIHooks.h"
#include "ape/anim/ApeAnimScript.h"
#include "sly/game/SlyGame.h"
#include "sly/entity/Character.h"
#include "sly/levels/ai/AITuning.h"

#include <cmath>

namespace sly {
namespace levels {
namespace ai {

const BismarqueStepList& BismarqueAI::CurrentSteps() const
{
    const BismarquePhase& p = pattern->phases[phase];
    if (modeFlags & 0x40000000u)
        return p.modeC;
    if (modeFlags & 0x80000000u)
        return p.modeB;
    return p.normal;
}

// @func 0x000175FC BismarqueAI::BismarqueAI
BismarqueAI::BismarqueAI()
{
    pool1AC.reserve(10);
    modeFlags &= 0x3FFFFFFFu;
    currentAttack = -1;
    qteIntroDelay = g_bismarqueQTEGameIntroDelayTimeMax;
    counterCooldown = g_bismarqueCounterAttackDelayTimeMax;
    field_308 &= 0x1FFFFFFFu;
}

// @func 0x007392D8 BismarqueAI::~BismarqueAI
// @func 0x00739464 BismarqueAI::~BismarqueAI (deleting)
BismarqueAI::~BismarqueAI() = default;

// @func 0x0003E040 BismarqueAI::CurrentStep
const BismarqueStep& BismarqueAI::CurrentStep() const
{
    return CurrentSteps().steps[stepIndex];
}

// @func 0x00042D5C BismarqueAI::AdvanceStep
void BismarqueAI::AdvanceStep()
{
    if (!pattern)
        return;
    ++stepIndex;
    if (stepIndex >= int32_t(CurrentSteps().count)) {
        currentAttack = -1;
        stepIndex = 0;
    }
    EnterStepState();
}

// @func 0x0003E0CC BismarqueAI::EnterStepState
void BismarqueAI::EnterStepState()
{
    // AI state of each step kind. NOTE: the states' names are not known.
    static constexpr int kStepStates[13] = { 2, 5, 8, 0xC, 0xF, 0, 0x10, 0x11, 0x20, 0x21, 0x22, 0x23, 0x24 };
    if (!pattern)
        return;
    const int kind = CurrentStep().kind;
    SetState(kind >= 0 && kind < 13 ? kStepStates[kind] : 0);
}

// @func 0x00042D3C BismarqueAI::GetTargetPosition
ape::Vec3 BismarqueAI::GetTargetPosition()
{
    return AIController::GetTargetPosition();
}

// @func 0x0004377C BismarqueAI::Think
bool BismarqueAI::Think(float dt)
{
    (void)dt;
    return false;
}

// @func 0x0004399C BismarqueAI::UpdateMarkers
void BismarqueAI::UpdateMarkers()
{
    for (int i = 0; i < 2; ++i) {
        if (!markers[i] || !anchors[i])
            continue;
        ape::Matrix34 local = markers[i]->GetLocal();
        const ape::Vec3& at = anchors[i]->GetWorld().translation;
        local.translation.x = at.x;
        local.translation.y = at.y;
        local.translation.z = at.z;
        markers[i]->SetLocal(local);
        markers[i]->MarkDirty();
        markers[i]->UpdateWorld(false, false);
    }
}

// @func 0x00043A80 BismarqueAI::SetMarkersVisible
void BismarqueAI::SetMarkersVisible(bool first, bool second)
{
    if (markers[0])
        markers[0]->SetVisible(first, true);
    if (markers[1])
        markers[1]->SetVisible(second, true);
}

// @func 0x00044318 BismarqueAI::ScoreTargetPoint
float BismarqueAI::ScoreTargetPoint(const ape::Vec3& point, const ape::Vec3& direction) const
{
    const ape::Vec3& pos = transform.translation;
    const ape::Vec3& forward = transform.zAxis;
    const float dx = point.x - pos.x, dy = point.y - pos.y, dz = point.z - pos.z;
    const float dist = std::sqrt(dz * dz + dx * dx + dy * dy);
    const float inv = 1.0f / dist;
    const float turn = 1.0f - (direction.z * forward.z + direction.x * forward.x + direction.y * forward.y);
    const float aim = 1.0f - (forward.z * inv * dz + forward.x * inv * dx + forward.y * inv * dy);
    return turn * g_bismarqueTargetPointDeltaOrientationWeight + aim * g_bismarqueTargetPointAngleWeight +
           dist * g_bismarqueTargetPointDistanceWeight + 0.0f;
}

// @func 0x00046638 BismarqueAI::CounterAttackReady
bool BismarqueAI::CounterAttackReady(float dt)
{
    Entity* player = PlayerCharacter();
    if (!player)
        return false;
    counterCooldown -= dt;
    if (counterCooldown > 0.0f)
        return false;
    counterCooldown = 0.0f;
    const float dx = player->Position().x - transform.translation.x;
    const float dz = player->Position().z - transform.translation.z;
    const float range = g_bismarqueCounterAttackDistMin;
    return dz * dz + dx * dx + 0.0f < range * range;
}

// @func 0x00048CE0 BismarqueAI::GravitySpeed
float BismarqueAI::GravitySpeed()
{
    if (!(character->classBits & 0x4000u) && BodyMotionType(body) != 1)
        return -10.0f;
    return 0.0f;
}

// @func 0x000447E8 BismarqueAI::PickRetreatPoint
BismarqueAI::RetreatPoint* BismarqueAI::PickRetreatPoint()
{
    const uint32_t count = retreatPoints.Count();
    if (count == 0)
        return nullptr;
    static const ape::Vec3 kOrigin = {0.0f, 0.0f, 0.0f, 0.0f};
    Entity* player = PlayerCharacter();
    const ape::Vec3& playerPos = player ? player->transform.translation : kOrigin;
    const ape::Vec3& self = Position();
    const ape::Vec3& forward = transform.zAxis;
    float bestScore = -3.4028235e38f;
    int best = -1;
    for (uint32_t i = 0; i < count; ++i) {
        const ape::world::ApeObj* object = retreatPoints[i].object;
        const ape::Vec3& p = object ? object->GetWorld().translation : kOrigin;
        const float px = p.x - playerPos.x, pz = p.z - playerPos.z;
        const float sx = p.x - self.x, sz = p.z - self.z;
        const float score = sz * sz + sx * sx + pz * pz + px * px;
        if (score > bestScore && forward.z * sz + forward.x * sx >= 0.0f) {
            bestScore = score;
            best = int(i);
        }
    }
    if (best < 0)
        best = ape::RandomInt(ape::g_gameRandom, 0, int(count) - 1);
    return &retreatPoints[size_t(best)];
}

// @func 0x00041A08 BismarqueAI::SetFightVisualsHidden
void BismarqueAI::SetFightVisualsHidden(bool hidden)
{
    if (g_game->cutsceneQueue.Count() != 0 || state != 0x19)
        return;
    for (ape::world::ApeObj* marker : markers) {
        if (marker)
            marker->SetVisible(!hidden, true);
    }
    for (QteButton& button : qteButtons) {
        if (button.script)
            button.script->SetStateAndUpdate(hidden ? 0x8C3F8D2Eu : 0x28298107u);
    }
}

namespace {
constexpr uint32_t kBarrierState_Destroy = 0x879F2BDAu;

void KillBarrier(Entity* barrier)
{
    if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(barrier->GetAnimScript()))
        script->SetStateAndUpdate(kBarrierState_Destroy);
    barrier->Release(true);
}
} // namespace

// @func 0x0003F260 BismarqueAI::ClearAttack
void BismarqueAI::ClearAttack()
{
    stepIndex = 0;
    modeFlags &= 0x3FFFFFFFu;
    if (field_200) {
        field_200->Release(true);
        field_200 = nullptr;
    }
    for (Entity* barrier : pendingBarriers) {
        if (barrier)
            KillBarrier(barrier);
    }
    for (ThrownBarrier& thrown : thrownBarriers) {
        if (thrown.entity)
            KillBarrier(thrown.entity);
    }
    pendingBarriers.Clear();
    list1D8.Clear();
    thrownBarriers.Clear();
}

// @func 0x00040208 BismarqueAI::ThrowBarriers
void BismarqueAI::ThrowBarriers()
{
    for (size_t i = 0; i < pendingBarriers.Count(); ++i) {
        const float delay = (i & 1) ? float(g_bismarqueIceBarrierThrowOffsetTimemax) : 0.0f;
        thrownBarriers.PushBack({pendingBarriers[i], delay});
    }
    pendingBarriers.Clear();
    Entity* player = PlayerCharacter();
    if (player && player->HasTypeBit(3))
        static_cast<entity::Character*>(player)->SetReference1AC(nullptr);
}

// @func 0x00043784 BismarqueAI::RestartQteButtons
void BismarqueAI::RestartQteButtons(int skip)
{
    static const ape::Vec3 kOrigin = {0.0f, 0.0f, 0.0f, 0.0f};
    const int count = int(qteButtons.Count());
    int placed = 0;
    for (QteButton& button : qteButtons) {
        if (!button.object || button.done)
            continue;
        button.interpTime = g_bismarqueQTEButtonToGrizzIntroInterpTimeMax;
        button.delay = g_bismarqueQTEButtonToGrizzIntroDelayTimeMax * float(count - skip - placed - 1);
        const ape::Vec3& anchor = anchors[0] ? anchors[0]->GetWorld().translation : kOrigin;
        button.target = {anchor.x, anchor.y, anchor.z, button.target.pad};
        button.position = {anchor.x + g_bismarqueQTEButtonToGrizzStartXOffset, anchor.y, anchor.z,
                           button.position.pad};
        button.target.x += (g_bismarqueQTEGameGrizzTimeDelay + button.key->time) * g_bismarqueQTEGameDetectBarMoveSpeed;
        ape::Matrix34 local = button.object->GetLocal();
        local.translation = {button.position.x, button.position.y, button.position.z, local.translation.pad};
        button.object->SetLocalTransform(local);
        button.object->UpdateWorld(false, false);
        if (button.script)
            button.script->SetStateAndUpdate(0x9020A4D6u);
        ++placed;
    }
}

} // namespace ai
} // namespace levels
} // namespace sly
