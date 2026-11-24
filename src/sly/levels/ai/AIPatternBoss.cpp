// openTIT - Pattern boss behaviour (vtable 0x00B0E2F0).
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "sly/levels/ai/AIHooks.h"

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr uint32_t kMsg_Kill = 0x03DE9A54u;
constexpr uint32_t kBone_Effect = 0x001F0BAFu;          // NOTE: bone name unknown
constexpr uint32_t kScript_Loop = 0x5135BEBCu;          // NOTE: script name unknown
constexpr uint32_t kScriptState_Stop = 0x879F2BDAu;     // NOTE: state name unknown
constexpr uint32_t kScriptVar_This = 0x0027469Eu;       // "this"
constexpr int kState_Intro = 0;
constexpr int kState_Dead = 0x0C;

} // namespace

// @func 0x00027C4C PatternBossAI::Update
void PatternBossAI::Update(float dt)
{
    if (entity->flags & (0x40u | 0x20u))
        return;
    AIController::Update(dt);
    PreUpdate(dt);
    UpdateState(dt);
}

// @func 0x00027CFC PatternBossAI::PreUpdate
void PatternBossAI::PreUpdate(float dt)
{
    (void)dt;
    // NOTE: the console looks the bone up but does nothing with it.
    const int bone = entity->FindBone(kBone_Effect);
    if (bone >= 0)
        (void)entity->GetBone(bone);
}

// @func 0x00027D50 PatternBossAI::OnMessage
void PatternBossAI::OnMessage(const entity::EntityMessage& msg)
{
    if (msg.id == kMsg_Kill && state != kState_Dead)
        SetState(kState_Dead);
    AIController::OnMessage(msg);
}

// @func 0x00027DF0 PatternBossAI::OnDamaged
void PatternBossAI::OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info)
{
    (void)kind;
    if (amount <= 0.0f)
        return;
    // The damage goes to the entity itself when it is a player type entity.
    // NOTE: the console does not check for the null case.
    Entity* victim = (entity->typeBits[0] & 0x2u) ? entity : nullptr;
    if (victim)
        EntityDamage(victim, amount, info->source, 0);
}

// @func 0x00027E80 PatternBossAI::ChooseState
void PatternBossAI::ChooseState() {}

// @func 0x00027E88 PatternBossAI::Think
bool PatternBossAI::Think(float dt)
{
    (void)dt;
    return false;
}

// @func 0x00027E98 PatternBossAI::SetLoopScript
void PatternBossAI::SetLoopScript(bool on)
{
    if (!on) {
        if (loopScript) {
            loopScript->SetRemoveWhenFinished(true);
            loopScript->SetKeepAlive(false);
            loopScript->SetStateAndUpdate(kScriptState_Stop);
            loopScript = nullptr;
        }
        return;
    }
    if (loopScript)
        return;
    loopScript = ape::ApeAnimScriptInstance::Create(kScript_Loop);
    if (!loopScript)
        return;
    ape::ApeAnimScriptVar* var = loopScript->FindVar(kScriptVar_This);
    if (var && var->GetType() == ape::AnimScriptVarType::Object)
        static_cast<ape::ApeAnimScriptObjectVar*>(var)->SetObject(
            EntityScriptObject(entity));
    loopScript->SetRemoveWhenFinished(false);
    loopScript->SetKeepAlive(true);
    loopScript->Update(1.0f / 60.0f);
}

// @func 0x0002B3BC PatternBossAI::ExitState
void PatternBossAI::ExitState()
{
    if (state == kState_Intro)
        HudSetContext(entity, "CPU");
}

} // namespace ai
} // namespace levels
} // namespace sly
