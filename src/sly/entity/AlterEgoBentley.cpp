// openTIT - States of the Bentley alter ego entity (see AlterEgoBentley.h).
#include "sly/entity/AlterEgoBentley.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

// @func 0x0016CD1C AlterEgoBentleyState::~AlterEgoBentleyState
// @func 0x0016CD2C AlterEgoBentleyState::~AlterEgoBentleyState (deleting)
AlterEgoBentleyState::~AlterEgoBentleyState() = default;

// @func 0x0016CD70 AlterEgoBentleyState::Enter
void AlterEgoBentleyState::Enter(void* /*owner*/) {}

// @func 0x0016CD74 AlterEgoBentleyState::Exit
void AlterEgoBentleyState::Exit(void* /*owner*/) {}

namespace {

constexpr uint32_t kFx_Death = 0x03DE9A54u;        // the "killed" event's effect
constexpr uint32_t kFx_PowerUp = 0xD467C476u;      // NOTE: names unknown
constexpr uint32_t kFx_Speed = 0xB94AF9E7u;
constexpr uint32_t kFx_Script = 0x663F99F0u;
constexpr uint32_t kVar_SpeedLevel = 0x04B70827u;  // NOTE: name unknown
constexpr uint32_t kVar_This = 0x0027469Eu;        // "this"
constexpr uint32_t kScript_PowerUpA = 0x480402DDu; // NOTE: names unknown
constexpr uint32_t kScript_PowerUpB = 0x24CC86BDu;
constexpr uint32_t kState_Destroy = 0x879F2BDAu;   // "Destroy"
constexpr uint32_t kState_Refresh = 0x00011773u;   // NOTE: name unknown
constexpr float kScriptStep = 1.0f / 60.0f;

AlterEgoBentleyEntity* OwnerOf(void* owner) { return static_cast<AlterEgoBentleyEntity*>(owner); }

// Integer variable of the owner's anim script, or null.
ape::ApeAnimScriptIntVar* SpeedLevelVar(AlterEgoBentleyEntity* owner)
{
    auto* script = static_cast<ape::ApeAnimScriptInstance*>(owner->GetAnimScript());
    if (!script)
        return nullptr;
    ape::ApeAnimScriptVar* var = script->FindVar(kVar_SpeedLevel);
    return (var && var->GetType() == ape::AnimScriptVarType::Int) ? static_cast<ape::ApeAnimScriptIntVar*>(var)
                                                                   : nullptr;
}

// Starts a power-up script with the owner's world object as "this".
ape::ApeAnimScriptInstance* StartPowerUpScript(uint32_t name, AlterEgoBentleyEntity* owner)
{
    ape::ApeAnimScriptInstance* script = ape::ApeAnimScriptInstance::Create(name);
    if (!script)
        return nullptr;
    if (ape::ApeAnimScriptVar* self = script->FindVar(kVar_This)) {
        if (self->GetType() == ape::AnimScriptVarType::Object)
            static_cast<ape::ApeAnimScriptObjectVar*>(self)->SetObject(WorldObjectRef(owner->object));
    }
    return script;
}

} // namespace

void AlterEgoBentleyEntity::PlayPowerUpFx(uint32_t fx)
{
    if (health > 0.0f || lastFx != kFx_Death) {
        PlayCharacterFx(GetCharacterComponent(), fx, 1.0f, 0.25f);
        lastFx = fx;
    }
}

// @func 0x0016CED8 AlterEgoBentleyStateA::Enter
void AlterEgoBentleyStateA::Enter(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    if (EntityComponent* skin = EntitySkin(bentley)) {
        savedRate = SkinRate(skin);
        savedCount = bentley->skinCount;
        const float period = savedRate > 0.0f ? 1.0f / savedRate : savedRate;
        SkinSetScale(skin, 1.0f / (period + 5.0f));
        SkinSetRate(skin, SkinComputeRate(skin));
        SkinSetLevel(skin, ++bentley->skinCount);
    }
    bentley->PlayPowerUpFx(kFx_PowerUp);
}

// @func 0x0016D068 AlterEgoBentleyStateA::Exit
void AlterEgoBentleyStateA::Exit(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    if (EntityComponent* skin = EntitySkin(bentley)) {
        SkinSetScale(skin, savedRate);
        SkinSetRate(skin, SkinComputeRate(skin));
        bentley->skinCount = savedCount;
        SkinSetLevel(skin, savedCount);
    }
}

// @func 0x0016D160 AlterEgoBentleyStateB::Enter
void AlterEgoBentleyStateB::Enter(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    savedSpeed = bentley->speedScale;
    bentley->speedScale = savedSpeed * 1.25f;
    bentley->PlayPowerUpFx(kFx_Speed);
    if (ape::ApeAnimScriptIntVar* level = SpeedLevelVar(bentley)) {
        savedLevel = level->GetInt();
        level->SetInt(savedLevel + 1);
    }
}

// @func 0x0016D314 AlterEgoBentleyStateB::Exit
void AlterEgoBentleyStateB::Exit(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    bentley->speedScale = savedSpeed;
    // NOTE: the console reads the current value back before writing it.
    if (ape::ApeAnimScriptIntVar* level = SpeedLevelVar(bentley)) {
        savedLevel = level->GetInt();
        level->SetInt(savedLevel);
    }
}

// @func 0x0016D460 AlterEgoBentleyStateC::Enter
void AlterEgoBentleyStateC::Enter(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    bentley->PlayPowerUpFx(kFx_PowerUp);
    EntityComponent* skin = EntitySkin(bentley);
    if (skin)
        skin = bentley->GetComponent(kComp_Skin, skin);   // the second skin
    if (skin) {
        savedLevel = bentley->skinLevel;
        SkinSetScale(skin, 1.0f / (float(savedLevel) + 1.0f));
        SkinSetRate(skin, SkinComputeRate(skin));
    }
    bentley->skinLevel = savedLevel + 1;
}

// @func 0x0016D5F0 AlterEgoBentleyStateC::Exit
void AlterEgoBentleyStateC::Exit(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    EntityComponent* skin = EntitySkin(bentley);
    if (skin)
        skin = bentley->GetComponent(kComp_Skin, skin);
    if (skin) {
        SkinSetScale(skin, 1.0f / float(savedLevel));
        SkinSetRate(skin, SkinComputeRate(skin));
    }
    bentley->skinLevel = savedLevel;
}

// @func 0x0016D720 AlterEgoBentleyStateD::Enter
void AlterEgoBentleyStateD::Enter(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    savedLevel = SkinVariant(EntitySkin(bentley));
    SkinSetVariant(EntitySkin(bentley), savedLevel + 3);
    bentley->PlayPowerUpFx(kFx_PowerUp);
}

// @func 0x0016D850 AlterEgoBentleyStateD::Exit
void AlterEgoBentleyStateD::Exit(void* owner)
{
    SkinSetVariant(EntitySkin(OwnerOf(owner)), savedLevel);
}

// @func 0x0016D9E8 AlterEgoBentleyScriptStateA::Enter
void AlterEgoBentleyScriptStateA::Enter(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    const uint32_t level = bentley->scriptLevel;
    if (level & 1)
        field_18 = -1.0f;
    savedLevel = level;
    bentley->scriptLevel = level + 1;
    strength *= float(double(level) * 0.4 + 1.0);
    bentley->PlayPowerUpFx(kFx_Script);
    if (!script) {
        auto* instance = StartPowerUpScript(kScript_PowerUpA, bentley);
        script = instance;
        if (instance) {
            AttachAnimScript(instance);
            SetAnimScriptKeepAlive(instance, true);
            // NOTE: slot 4 of the state runs once before the first script step.
            Slot4();
            instance->Update(kScriptStep);
        }
    }
}

// @func 0x0016DC78 AlterEgoBentleyScriptStateA::Exit
void AlterEgoBentleyScriptStateA::Exit(void* owner)
{
    OwnerOf(owner)->scriptLevel = savedLevel;
    auto* instance = static_cast<ape::ApeAnimScriptInstance*>(script);
    DetachAnimScript(instance);
    SetAnimScriptKeepAlive(instance, false);
    instance->SetStateAndUpdate(kState_Destroy);
    script = nullptr;
}

// @func 0x0016E908 AlterEgoBentleyScriptStateB::Enter
void AlterEgoBentleyScriptStateB::Enter(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    bentley->PlayPowerUpFx(kFx_Script);
    count = 1;
    bentley->buffFlags |= 0x2000000;
    if (!script) {
        auto* instance = StartPowerUpScript(kScript_PowerUpB, bentley);
        script = instance;
        if (instance) {
            AttachAnimScript(instance);
            SetAnimScriptKeepAlive(instance, true);
            instance->Update(kScriptStep);
        }
    }
}

// @func 0x0016EB20 AlterEgoBentleyScriptStateB::Exit
// Each exit uses up one charge; the last one ends the buff and its script.
void AlterEgoBentleyScriptStateB::Exit(void* owner)
{
    AlterEgoBentleyEntity* bentley = OwnerOf(owner);
    const uint32_t left = count - 1;
    auto* instance = static_cast<ape::ApeAnimScriptInstance*>(script);
    if (left == 0) {
        count = 0;
        bentley->buffFlags &= ~0x2000000u;
        if (instance) {
            DetachAnimScript(instance);
            SetAnimScriptKeepAlive(instance, false);
            instance->SetStateAndUpdate(kState_Destroy);
            script = nullptr;
        }
        return;
    }
    count = left;
    ++bentley->buffTicks;
    bentley->buffFlags |= 0x2000000;
    bentley->buffRate = 1.0f / float(left);
    if (instance) {
        AttachAnimScript(instance);
        SetAnimScriptKeepAlive(instance, true);
        instance->SetStateAndUpdate(kState_Refresh);
    }
}

// @func 0x0016CD84 AlterEgoBentleyStateA::~AlterEgoBentleyStateA
// @func 0x0016CD94 AlterEgoBentleyStateA::~AlterEgoBentleyStateA (deleting)
AlterEgoBentleyStateA::~AlterEgoBentleyStateA() = default;

// @func 0x0016D0F8 AlterEgoBentleyStateB::~AlterEgoBentleyStateB
// @func 0x0016D108 AlterEgoBentleyStateB::~AlterEgoBentleyStateB (deleting)
AlterEgoBentleyStateB::~AlterEgoBentleyStateB() = default;

// @func 0x0016D3FC AlterEgoBentleyStateC::~AlterEgoBentleyStateC
// @func 0x0016D40C AlterEgoBentleyStateC::~AlterEgoBentleyStateC (deleting)
AlterEgoBentleyStateC::~AlterEgoBentleyStateC() = default;

// @func 0x0016D6B0 AlterEgoBentleyStateC::Slot4
void AlterEgoBentleyStateC::Slot4() {}

// @func 0x0016D6BC AlterEgoBentleyStateD::~AlterEgoBentleyStateD
// @func 0x0016D6CC AlterEgoBentleyStateD::~AlterEgoBentleyStateD (deleting)
AlterEgoBentleyStateD::~AlterEgoBentleyStateD() = default;

void AlterEgoBentleyScriptState::ReleaseScript()
{
    if (!script)
        return;
    OrphanAndReleaseAnimScript(script);
    script = nullptr;
}

// @func 0x0016D8A0 AlterEgoBentleyScriptStateA::~AlterEgoBentleyScriptStateA
// @func 0x0016D928 AlterEgoBentleyScriptStateA::~AlterEgoBentleyScriptStateA (deleting)
AlterEgoBentleyScriptStateA::~AlterEgoBentleyScriptStateA()
{
    ReleaseScript();
}

// @func 0x0016E7C8 AlterEgoBentleyScriptStateB::~AlterEgoBentleyScriptStateB
// @func 0x0016E850 AlterEgoBentleyScriptStateB::~AlterEgoBentleyScriptStateB (deleting)
AlterEgoBentleyScriptStateB::~AlterEgoBentleyScriptStateB()
{
    ReleaseScript();
}

// @func 0x0016EC48 AlterEgoBentleyScriptStateB::Slot4
void AlterEgoBentleyScriptStateB::Slot4() {}

} // namespace entity
} // namespace sly
