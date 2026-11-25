// openTIT - Knowledge results (0x00391868-0x00398ED0).
#include "sly/knowledge/KnowledgeTypes.h"
#include "sly/knowledge/KnowledgeHooks.h"

#include "ape/core/ApeString.h"
#include "platform/Platform.h"
#include "sly/entity/Entity.h"
#include "sly/game/AudioSystem.h"
#include "sly/levels/ai/AIController.h"

namespace sly {
namespace knowledge {

namespace {

constexpr uint32_t kTypeBit_Character = 0x08;   // Entity::typeBits[0]
constexpr uint32_t kTypeBit_10 = 0x10;          // NOTE: characters the AI can highlight
constexpr uint64_t kClass_Player = 0x01;        // Entity::classBits bit 0
constexpr uint64_t kClass_NoGravity = 0x4000;   // NOTE: name from RSetGravity

bool IsCharacter(const Entity* e) { return e && (e->typeBits[0] & kTypeBit_Character); }

// Finds the variable named by field 0 of a RSetVarData* result and checks
// its type (`wantType` < 0: any type).
void LinkVariable(KRef* ref, KnowledgeData* list, int32_t wantType, const char* typeName)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const char* name = ResultValue(ref, 0).str;
    if (!name) {
        ReportResultError(ref, list, "Null var data name pass in!");
        runtime->payload.var.index = 0xFFFF;
        return;
    }
    const int index = list->vars.Find(ape::HashNameNoCase(name));
    runtime->payload.var.index = (uint16_t)index;
    if (index < 0) {
        ReportResultError(ref, list, ape::TempFormat("Cannot find KVarData with name (%s) to set!", name));
        return;
    }
    if (wantType >= 0 && list->vars.types[(size_t)index] != wantType)
        ReportResultError(ref, list, ape::TempFormat("The KVarData (%s) is not type %s!", name, typeName));
}

// Copies field 1 into the variable. Bool variables only take the first byte.
void StoreVariable(KRef* ref, KnowledgeData* list, bool asBool)
{
    const int16_t index = (int16_t)RuntimeOf(ref, list)->payload.var.index;
    if (index == -1 || (size_t)index >= list->vars.values.size())
        return;
    KValue& var = list->vars.values[(size_t)index];
    if (asBool)
        var.b = ResultValue(ref, 1).b;
    else
        var.u = ResultValue(ref, 1).u;
}

} // namespace

// @func 0x00776444 RDoNothing::Execute
bool RDoNothingExecute(KRef*, KnowledgeData*) { return false; }

// @func 0x00391868 RNachiDebug::Execute
bool RNachiDebugExecute(KRef*, KnowledgeData*) { return true; }

// @func 0x00391870 RBreakPoint::Execute
bool RBreakPointExecute(KRef*, KnowledgeData*) { return true; }

// @func 0x00391878 RChangeState::Link
void RChangeStateLink(KRef* ref, KnowledgeData* list)
{
    if (const char* name = ResultValue(ref, 0).str)
        RuntimeOf(ref, list)->payload.hash = ape::HashNameNoCase(name);
}

// Asks for the state change (the AI applies it after the rules ran).
// @func 0x003919A4 RChangeState::Execute
bool RChangeStateExecute(KRef* ref, KnowledgeData* list)
{
    const uint32_t target = RuntimeOf(ref, list)->payload.hash;
    if (!target || list->states[list->currentState].hash == target)
        return false;
    for (int i = 0; i < list->stateSlotCount; ++i) {
        if (list->states[i].hash == target) {
            list->flags |= kList_StateRequested;
            list->requestedState = (uint8_t)i;
            return false;
        }
    }
    plat::Log(plat::LogLevel::Warning, "Fail to change state since new state (%s) cannot be found!\n",
              ape::TempHexString(target));
    list->flags |= kList_Broken;
    plat::Log(plat::LogLevel::Warning, "Fail to change to new state (%s)\n",
              ResultValue(ref, 0).str ? ResultValue(ref, 0).str : "");
    return false;
}

// @func 0x00391B80 RSetVarDataInt::Link
void RSetVarDataIntLink(KRef* ref, KnowledgeData* list) { LinkVariable(ref, list, kVar_Int, "int"); }
// @func 0x00391FF4 RSetVarDataInt::Execute
bool RSetVarDataIntExecute(KRef* ref, KnowledgeData* list) { StoreVariable(ref, list, false); return false; }
// @func 0x00392130 RSetVarDataFloat::Link
void RSetVarDataFloatLink(KRef* ref, KnowledgeData* list) { LinkVariable(ref, list, kVar_Float, "float"); }
// @func 0x003925A4 RSetVarDataFloat::Execute
bool RSetVarDataFloatExecute(KRef* ref, KnowledgeData* list) { StoreVariable(ref, list, false); return false; }
// @func 0x003926E0 RSetVarDataBool::Link
void RSetVarDataBoolLink(KRef* ref, KnowledgeData* list) { LinkVariable(ref, list, kVar_Bool, "Bool"); }
// @func 0x00392B54 RSetVarDataBool::Execute
bool RSetVarDataBoolExecute(KRef* ref, KnowledgeData* list) { StoreVariable(ref, list, true); return false; }
// @func 0x00392C90 RSetVarDataHash::Link
void RSetVarDataHashLink(KRef* ref, KnowledgeData* list) { LinkVariable(ref, list, kVar_Hash, "hash"); }
// @func 0x00393104 RSetVarDataHash::Execute
bool RSetVarDataHashExecute(KRef* ref, KnowledgeData* list) { StoreVariable(ref, list, false); return false; }

// @func 0x00393240 RIncrementVarData::Link
void RIncrementVarDataLink(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const char* name = ResultValue(ref, 0).str;
    if (!name) {
        runtime->payload.var.index = 0xFFFF;
        return;
    }
    const int index = list->vars.Find(ape::HashNameNoCase(name));
    runtime->payload.var.index = (uint16_t)index;
    if (index < 0)
        plat::Log(plat::LogLevel::Warning,
                  "RIncrementKVarDataDef: Cannot find KVarData with name (%s) to set!\n", name);
}

// Ints and floats count up by one, bools toggle.
// @func 0x00393418 RIncrementVarData::Execute
bool RIncrementVarDataExecute(KRef* ref, KnowledgeData* list)
{
    const uint16_t index = RuntimeOf(ref, list)->payload.var.index;
    if (index == 0xFFFF || index >= list->vars.values.size())
        return false;
    KValue& var = list->vars.values[index];
    const int32_t type = list->vars.types[index];
    switch (type) {
    case kVar_Int: ++var.i; return false;
    case kVar_Float: var.f += 1.0f; return false;
    case kVar_Bool: var.b = !var.b; return false;
    default: break;
    }
    const char* typeName = type == kVar_Hash ? "hash" : type == kVar_String ? "bool" : "int";
    plat::Log(plat::LogLevel::Warning,
              "RIncrementKVarDataDef: VarData (%s) data type (%s) do not support increment!\n",
              ResultValue(ref, 0).str ? ResultValue(ref, 0).str : "", typeName);
    return false;
}

// Debug text is compiled out of the release build.
// @func 0x00393664 RDisplayDebugText::Execute
bool RDisplayDebugTextExecute(KRef*, KnowledgeData*) { return false; }

// @func 0x0039366C RSendEvent::Execute
bool RSendEventExecute(KRef* ref, KnowledgeData* list)
{
    if (Entity* owner = list->owner)
        SendGameEvent(owner, owner, ResultValue(ref, 0).u, ResultValue(ref, 1).u, true);
    return false;
}

// @func 0x0039379C RBroadcastEvent::Execute
bool RBroadcastEventExecute(KRef* ref, KnowledgeData* list)
{
    const uint32_t group = ResultValue(ref, 1).u;
    if (group)
        BroadcastGameEvent(list->owner, group, ResultValue(ref, 0).u, ResultValue(ref, 2).u);
    return false;
}

// @func 0x00393944 RChangeDirectorState::Execute
bool RChangeDirectorStateExecute(KRef* ref, KnowledgeData* list)
{
    if (Entity* owner = list->owner)
        EntitySetDirectorState(owner, ResultValue(ref, 0).u);
    return false;
}

// Plays an animation on the owner, a named entity or a named child of the
// owner; characters play it on their character component.
// @func 0x00393A38 RPlayAnimDirect::Execute
bool RPlayAnimDirectExecute(KRef* ref, KnowledgeData* list)
{
    const char* anim = ResultValue(ref, 0).str;
    if (!anim)
        return false;
    Entity* target = list->owner;
    if (const char* name = ResultValue(ref, 1).str) {
        const uint32_t hash = ape::HashNameNoCase(name);
        Entity* found = !ResultValue(ref, 2).b ? entity::FindEntity(hash)
                                               : (target ? target->FindDescendant(hash) : nullptr);
        if (found)
            target = found;
    }
    if (!target)
        return false;
    if (EntityComponent* character = target->GetCharacterComponent())
        CharacterPlayAnim(character, anim);
    else
        EntityPlayAnim(target, anim);
    return false;
}

// @func 0x00393ED4 RPlaySndSFX::Release
void RPlaySndSFXRelease(KRef* ref, KnowledgeData* list)
{
    if (KRuntime* runtime = RuntimeOf(ref, list)) {
        audio::SoundNameRelease(static_cast<audio::SoundName*>(runtime->payload.object));
        runtime->payload.object = nullptr;
    }
}

// Resolves the sound.
// @func 0x00393F6C RPlaySndSFX::Link
void RPlaySndSFXLink(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const uint32_t sound = ResultValue(ref, 0).u;
    if (!sound) {
        ReportResultError(ref, list, "ResultPlaySoundDef: Null snd name pass in, no snd will be played!");
        return;
    }
    audio::SoundNameRelease(static_cast<audio::SoundName*>(runtime->payload.object));
    runtime->payload.object = audio::CreateSoundName(sound, 0);
}

// A clone shares the sound.
// @func 0x00394174 RPlaySndSFX::Clone
void RPlaySndSFXClone(KRef* ref, KnowledgeData* list)
{
    audio::SoundNameAddRef(static_cast<audio::SoundName*>(RuntimeOf(ref, list)->payload.object));
}

// @func 0x003941F4 RPlaySndSFX::Execute
bool RPlaySndSFXExecute(KRef* ref, KnowledgeData* list)
{
    enum { kSndName, kPart, kUseDefaultSndLib, kIs3DSnd, kUseSurfaceData, kAttachType, kAttachTypePlayer };
    auto* sound = static_cast<audio::SoundName*>(RuntimeOf(ref, list)->payload.object);
    uint32_t attachType = ResultValue(ref, kAttachType).u;
    const uint32_t part = ResultValue(ref, kPart).u;
    const bool useSurface = ResultValue(ref, kUseSurfaceData).b;
    if (!audio::SoundNameHasDef(sound))
        return false;
    Entity* target = list->owner;
    if (!target) {
        PlayRuleSound(nullptr, sound, 0, 0, false);
        return false;
    }
    if (part)
        if (Entity* child = target->FindDescendant(part))
            target = child;
    const uint32_t playerAttach = ResultValue(ref, kAttachTypePlayer).u;
    if (playerAttach && (target->classBits & kClass_Player))
        attachType = playerAttach;
    PlayRuleSound(target, sound, attachType, part, useSurface);
    return false;
}

// @func 0x0039474C RStopSndSFX::Execute
bool RStopSndSFXExecute(KRef* ref, KnowledgeData* list)
{
    if (Entity* owner = list->owner)
        audio::StopVoice(owner->FindSoundVoice(ResultValue(ref, 0).u), true);
    return false;
}

// @func 0x00394840 RPlayDirScript::Link
void RPlayDirScriptLink(KRef* ref, KnowledgeData* list)
{
    if (!ResultValue(ref, 0).u) {
        ReportResultError(ref, list, "RPlayDirScriptDef: No dir script name pass in!");
        RuntimeOf(ref, list)->payload.object = nullptr;
    }
}

// Plays a director script on the owner (or attaches it to a character);
// with InheritSurface the script learns the surface the owner stands on.
// @func 0x00394A24 RPlayDirScript::Execute
bool RPlayDirScriptExecute(KRef* ref, KnowledgeData* list)
{
    enum { kScript, kMode, kAttach, kInheritSurface };
    constexpr uint32_t kVar_Surface = 0xBC55788Du;
    Entity* owner = list->owner;
    if (!owner)
        return false;
    const uint32_t script = ResultValue(ref, kScript).u;
    if (ResultValue(ref, kAttach).b && IsCharacter(owner)) {
        EntityAttachDirScript(owner, script);
        return false;
    }
    void* instance = EntityPlayDirScript(owner, script);
    uint32_t surface = 0;
    if (ResultValue(ref, kInheritSurface).b && instance && EntitySurfaceType(owner, &surface))
        ScriptSetVar(instance, kVar_Surface, surface);
    return false;
}

// @func 0x00394D28 RTurnOnHighlight::Execute
bool RTurnOnHighlightExecute(KRef* ref, KnowledgeData* list)
{
    Entity* owner = list->owner;
    if (owner && (owner->typeBits[0] & kTypeBit_10))
        AIHighlight(owner, ResultValue(ref, 0).b);
    return true;
}

// Timers only exist in clones, so the timer is looked up there.
// @func 0x00394E14 RResetTimer::Clone
void RResetTimerClone(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const char* name = ResultValue(ref, 0).str;
    if (!name) {
        ReportResultError(ref, list, "RResetTimer: No timer is specified!");
        return;
    }
    const int index = list->timers.Find(ape::HashNameNoCase(name));
    runtime->payload.var.index = (uint16_t)index;
    if (index < 0)
        ReportResultError(ref, list,
                          ape::TempFormat("RResetTimer: Cannot find the KTimer variable with name (%s)", name));
}

// @func 0x00395280 RResetTimer::Execute
bool RResetTimerExecute(KRef* ref, KnowledgeData* list)
{
    const uint16_t index = RuntimeOf(ref, list)->payload.var.index;
    if (index < list->timers.values.size())   // NOTE: the console writes without a check
        list->timers.values[index] = 0.0f;
    return false;
}

// @func 0x00395348 RSetAttackInvincibility::Execute
bool RSetAttackInvincibilityExecute(KRef* ref, KnowledgeData* list)
{
    if (list->owner && list->owner->listener)
        AISetAttackInvincible(list->owner, ResultValue(ref, 0).b);
    return false;
}

// @func 0x003953FC RSetAdditiveAlpha::Execute
bool RSetAdditiveAlphaExecute(KRef* ref, KnowledgeData* list)
{
    if (list->owner && list->owner->GetCharacterComponent())
        CharacterSetAdditiveAlpha(list->owner, ResultValue(ref, 0).f);
    return false;
}

// @func 0x003954FC RSetMaxRotationRate::Execute
bool RSetMaxRotationRateExecute(KRef* ref, KnowledgeData* list)
{
    if (list->owner->GetComponent0D())
        AISetMaxRotationRate(list->owner, ResultValue(ref, 0).f);
    return false;
}

// @func 0x003955EC RMotionExTuning::Execute
bool RMotionExTuningExecute(KRef* ref, KnowledgeData* list)
{
    if (list->owner)
        EntitySetMotionTuning(list->owner, ResultValue(ref, 0).u, ResultValue(ref, 1).u,
                              ResultValue(ref, 2).u, ResultValue(ref, 3).u);
    return false;
}

// @func 0x003957DC RSetGravity::Execute
bool RSetGravityExecute(KRef* ref, KnowledgeData* list)
{
    Entity* owner = list->owner;
    if (IsCharacter(owner))
        owner->SetClassBit(kClass_NoGravity, !ResultValue(ref, 0).b);
    return false;
}

// TODO(0x003958C8 RCameraShake::Execute)
bool RCameraShakeExecute(KRef*, KnowledgeData*) { return false; }

// @func 0x00395EB4 RControllerRumble::Execute
bool RControllerRumbleExecute(KRef* ref, KnowledgeData*)
{
    PadRumble(ResultValue(ref, 0).i, ResultValue(ref, 1).f, ResultValue(ref, 2).f);
    return false;
}

// @func 0x00396068 RPlayFootstep::Execute
bool RPlayFootstepExecute(KRef* ref, KnowledgeData* list)
{
    constexpr uint32_t kHash_Right = 0x04A5C9FCu;
    Entity* owner = list->owner;
    if (!IsCharacter(owner))
        return false;
    uint32_t attach = ResultValue(ref, 1).u;
    const uint32_t playerAttach = ResultValue(ref, 2).u;
    if (playerAttach && (owner->classBits & kClass_Player))
        attach = playerAttach;
    CharacterPlayFootstep(owner, ResultValue(ref, 0).u == kHash_Right, attach);
    return true;
}

// @func 0x00396250 RFootIK::Execute
bool RFootIKExecute(KRef* ref, KnowledgeData* list)
{
    Entity* owner = list->owner;
    if (!IsCharacter(owner) || !owner->GetCharacterComponent())
        return false;
    CharacterFootIK(owner, ResultValue(ref, 0).u, ResultValue(ref, 1).b);
    return true;
}

// @func 0x003963E0 REnableComponent::Execute
bool REnableComponentExecute(KRef* ref, KnowledgeData* list)
{
    Entity* owner = list->owner;
    if (!IsCharacter(owner))
        return false;
    EntityComponent* component = owner->FindComponentById(ResultValue(ref, 0).u);
    if (!component)
        return false;
    component->SetOption(ResultValue(ref, 1).b);
    return true;
}

// @func 0x003965A0 RSetProjectileParam::Execute
bool RSetProjectileParamExecute(KRef* ref, KnowledgeData* list)
{
    Entity* owner = list->owner;
    if (!IsCharacter(owner))
        return false;
    CharacterSetProjectileParams(owner, ResultValue(ref, 0).f, ResultValue(ref, 1).b, ResultValue(ref, 2).b);
    return true;
}

// @func 0x00396750 RStopDirScript::Execute
bool RStopDirScriptExecute(KRef*, KnowledgeData* list)
{
    if (!IsCharacter(list->owner))
        return false;
    EntityStopDirScript(list->owner);
    return true;
}

// TODO(0x003967B0 RSetPosition::Execute)
bool RSetPositionExecute(KRef*, KnowledgeData*) { return false; }

// Speed, or with Time > 0 the speed that plays the clip in Time seconds.
// @func 0x00396930 RSetAnimSpeed::Execute
bool RSetAnimSpeedExecute(KRef* ref, KnowledgeData* list)
{
    if (!list->owner)
        return false;
    EntityComponent* character = list->owner->GetCharacterComponent();
    if (!character)
        return false;
    float speed = ResultValue(ref, 0).f;
    const float time = ResultValue(ref, 1).f;
    if (time > 0.0f)
        speed = CharacterAnimRate(character, speed) / time;
    CharacterSetAnimSpeed(character, speed);
    return false;
}

// Makes noise the enemies may hear.
// @func 0x00396B6C RCreateSoundAlert::Execute
bool RCreateSoundAlertExecute(KRef* ref, KnowledgeData* list)
{
    int noise = 0;
    const uint32_t type = ResultValue(ref, 0).u;
    if (type == 0x804F6B58u) {
        noise = 1;
        if (IsCharacter(list->owner) && SoundAlertSuppressed(list->owner))
            return false;
    } else if (type == 0x75208DC6u) {
        noise = 2;
    }
    levels::ai::AlertEnemies(list->owner, noise, false);
    return false;
}

// @func 0x00396D80 RStartMeleeAttack::Link
void RStartMeleeAttackLink(KRef* ref, KnowledgeData* list)
{
    const char* bone = ResultValue(ref, 0).str;
    if (!bone) {
        ReportResultError(ref, list, "ResultSetMeleeAttackRaidus: No attach bone name associate with!");
        return;
    }
    RuntimeOf(ref, list)->payload.hash = ape::HashNameNoCase(bone);
}

// Checks the clone's owner has the attach bone.
// @func 0x00396FC0 RStartMeleeAttack::Clone
void RStartMeleeAttackClone(KRef* ref, KnowledgeData* list)
{
    Entity* owner = list->owner;
    const uint32_t bone = RuntimeOf(ref, list)->payload.hash;
    if (!owner || !bone || EntityHasAttachPoint(owner, bone))
        return;
    ReportResultError(ref, list,
                      ape::TempFormat("RStartMeleeAttack: Cannot find bone (%s) for character %s!",
                                      ResultValue(ref, 0).str, ""));
}

// TODO(0x0039724C RStartMeleeAttack::Execute)
bool RStartMeleeAttackExecute(KRef*, KnowledgeData*) { return false; }

// @func 0x003976C8 REndMeleeAttack::Link
void REndMeleeAttackLink(KRef* ref, KnowledgeData* list)
{
    const char* bone = ResultValue(ref, 0).str;
    RuntimeOf(ref, list)->payload.hash = bone ? ape::HashNameNoCase(bone) : 0;
}

// Drops the owner's attack sighting (of that bone) once its target is gone.
// @func 0x00397858 REndMeleeAttack::Execute
bool REndMeleeAttackExecute(KRef* ref, KnowledgeData* list)
{
    Entity* owner = list->owner;
    if (!owner || !owner->listener)
        return false;
    // The listener of an AI character is its controller.
    auto* controller = static_cast<levels::ai::AIController*>(owner->listener);
    Entity* target = controller->target;
    if (!target || !(target->typeBits[0] & 0x2u) || !target->IsAlive())
        return false;
    levels::ai::AIManager* manager = levels::ai::GetAIManager();
    if (!manager)
        return false;
    const uint32_t bone = RuntimeOf(ref, list)->payload.hash;
    if (!bone)
        manager->ForgetSighting(owner);
    else
        manager->ForgetSighting(owner, bone);
    return false;
}

// TODO(0x00397970 RFireProjectile::Link)
void RFireProjectileLink(KRef*, KnowledgeData*) {}
// TODO(0x00397D8C RFireProjectile::Clone)
void RFireProjectileClone(KRef*, KnowledgeData*) {}
// TODO(0x00398040 RFireProjectile::Execute)
bool RFireProjectileExecute(KRef*, KnowledgeData*) { return false; }

// @func 0x00398D54 RFireWeapon::Link
void RFireWeaponLink(KRef*, KnowledgeData*) {}

// @func 0x00398D58 RFireWeapon::Execute
bool RFireWeaponExecute(KRef*, KnowledgeData* list)
{
    if (!list->owner)
        return false;
    EntityComponent* skin = entity::EntitySkin(list->owner);
    if (!skin)
        return false;
    SkinRequestFire(skin);
    return true;
}

} // namespace knowledge
} // namespace sly
