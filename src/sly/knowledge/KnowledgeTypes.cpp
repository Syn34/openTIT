// openTIT - Knowledge type tables (0x00F6503C / 0x00F65100) and the type
// factories that describe each cause/result's attributes.
//
// NOTE: the factories (0x007755E4-0x0077A950) and the runtime entry
// allocators (0x007737A4 causes, 0x007743CC results) lie outside the range
// this module was ported from; they are written here (untagged) because the
// callbacks need the slot layout they define. The per-type allocators the
// tables point at are thin wrappers around the two generic ones.
#include "sly/knowledge/Knowledge.h"
#include "sly/knowledge/KnowledgeTypes.h"

#include "ape/level/ApeDTD.h"

#include <cstring>

namespace sly {
namespace knowledge {

namespace {

struct AttrSpec {
    const char* name;
    int32_t type;          // ape::DTDAttributeType
    int32_t optional;      // DTD field_3C: 1 = may be missing
    uint32_t defaultBits;  // raw default value
    int slot;              // field index in the type's slot map (-1: none)
    const char* enums[6];
};

struct TypeSpec {
    const char* name;
    uint8_t causeFlags;    // causes only
    AttrSpec attrs[18];
};

using ape::kDTD_Int;
using ape::kDTD_Int1;
using ape::kDTD_Hash;
using ape::kDTD_Float;
using ape::kDTD_Bool;
using ape::kDTD_String;

constexpr uint32_t kOne = 0x3F800000u;   // 1.0f

const TypeSpec kCauseSpecs[kCauseTypeCount] = {
    { "CAlways", kCauseDef_Polled, {} },
    { "CNever", kCauseDef_Polled, {} },
    { "COnEnterState", kCauseDef_OnEnterState, {} },
    { "COnExitState", kCauseDef_OnExitState, {} },
    { "CEveryXSecond", kCauseDef_Polled, { { "Frequency", kDTD_Float, 0, 0, 0, {} } } },
    { "CIsVar", kCauseDef_Polled, {
        { "VarName", kDTD_String, 0, 0, 0, {} },
        { "Operator", kDTD_Hash, 0, kOp_Equal, 1,
          { "Equal", "Larger", "LargerEqual", "Less", "LessEqual", "NotEqual" } },
        { "FValue", kDTD_Float, 1, 0, 2, {} },
        { "IValue", kDTD_Int, 1, 0, 3, {} },
        { "BValue", kDTD_Bool, 1, 0, 4, {} },
        { "HValue", kDTD_Hash, 1, 0, 5, {} } } },
    { "CIsTimer", kCauseDef_Polled, {
        { "TimerName", kDTD_String, 0, 0, 0, {} },
        { "Value", kDTD_Float, 0, 0, 1, {} },
        { "Operator", kDTD_Hash, 1, kOp_LargerEqual, 2, { "LargerEqual", "LessEqual" } } } },
};

const TypeSpec kResultSpecs[kResultTypeCount] = {
    { "RDoNothing", 0, {} },
    { "RNachiDebug", 0, {} },
    { "RBreakPoint", 0, { { "Name", kDTD_String, 0, 0, 0, {} } } },
    { "RChangeState", 0, { { "StateName", kDTD_String, 0, 0, 0, {} } } },
    { "RSetVarDataInt", 0, { { "varDataName", kDTD_String, 0, 0, 0, {} }, { "value", kDTD_Int, 0, 0, 1, {} } } },
    { "RSetVarDataFloat", 0, { { "varDataName", kDTD_String, 0, 0, 0, {} }, { "value", kDTD_Float, 0, 0, 1, {} } } },
    { "RSetVarDataBool", 0, { { "varDataName", kDTD_String, 0, 0, 0, {} }, { "value", kDTD_Float, 0, 0, 1, {} } } },
    { "RSetVarDataHash", 0, { { "varDataName", kDTD_String, 0, 0, 0, {} }, { "value", kDTD_Float, 0, 0, 1, {} } } },
    { "RIncrementVarData", 0, { { "varDataName", kDTD_String, 0, 0, 0, {} } } },
    { "RDisplayDebugText", 0, {
        { "PosX", kDTD_Float, 1, 0, 0, {} }, { "PosY", kDTD_Float, 1, 0, 1, {} },
        { "Duration", kDTD_Float, 1, 0, 2, {} }, { "Text", kDTD_String, 0, 0, 3, {} },
        { "PrintOnAI", kDTD_Bool, 1, 1, 4, {} } } },
    { "RPlaySndSFX", 0, {
        { "SndName", kDTD_Hash, 0, 0, 0, {} }, { "Part", kDTD_Hash, 1, 0, 1, {} },
        { "UseDefaultSndLib", kDTD_Bool, 1, 0, 2, {} }, { "Is3DSnd", kDTD_Bool, 1, 1, 3, {} },
        { "AttachType", kDTD_Hash, 1, 0, 5, {} }, { "AttachTypePlayer", kDTD_Hash, 1, 0, 6, {} },
        { "UseSurfaceData", kDTD_Bool, 1, 1, 4, {} }, { "Layer", kDTD_Hash, 1, 0, 7, {} },
        { "ExclusivityGroup", kDTD_Hash, 1, 0, 8, {} },
        { "ExclusivityGroupPriority", kDTD_Int, 1, 0, 9, {} } } },
    { "RStopSndSFX", 0, { { "SndName", kDTD_Hash, 0, 0, 0, {} } } },
    { "RPlayDirScript", 0, {
        { "DirScriptName", kDTD_Hash, 0, 0, 0, {} },
        { "Mode", kDTD_Hash, 1, 0x36B9B6DEu, 1, { "PlayInObj" } },
        { "Attach", kDTD_Bool, 1, 0, 2, {} }, { "InheritSurface", kDTD_Bool, 1, 0, 3, {} } } },
    { "RTurnOnHighlight", 0, { { "TurnOn", kDTD_Bool, 1, 1, 0, {} } } },
    { "RResetTimer", 0, { { "TimerName", kDTD_String, 0, 0, 0, {} } } },
    { "RSendEvent", 0, { { "EventName", kDTD_Hash, 0, 0, 0, {} }, { "Data", kDTD_Hash, 1, 0, 1, {} } } },
    { "RBroadcastEvent", 0, {
        { "EventName", kDTD_Hash, 0, 0, 0, {} }, { "GroupName", kDTD_Hash, 0, 0, 1, {} },
        { "Data", kDTD_Hash, 1, 0, 2, {} } } },
    { "RChangeDirectorState", 0, { { "State", kDTD_Hash, 0, 0, 0, {} } } },
    { "RStartMeleeAttack", 0, {
        { "AttachBoneName", kDTD_String, 0, 0, 0, {} }, { "Radius", kDTD_Float, 0, 0, 1, {} },
        { "Damage", kDTD_Float, 0, 0, 2, {} }, { "Impulse", kDTD_Float, 0, 0, 3, {} },
        { "XOffset", kDTD_Float, 1, 0, 4, {} }, { "YOffset", kDTD_Float, 1, 0, 5, {} },
        { "ZOffset", kDTD_Float, 1, 0, 6, {} }, { "HitScript", kDTD_Hash, 1, 0, 7, {} },
        { "ReactType", kDTD_Hash, 1, 0, 8, {} } } },
    { "REndMeleeAttack", 0, { { "AttachBoneName", kDTD_String, 1, 0, 0, {} } } },
    { "RFireProjectile", 0, {
        { "AttachBoneName", kDTD_String, 1, 0, 0, {} }, { "ProjDefName", kDTD_String, 0, 0, 1, {} },
        { "XOffset", kDTD_Float, 1, 0, 2, {} }, { "YOffset", kDTD_Float, 1, 0, 3, {} },
        { "ZOffset", kDTD_Float, 1, 0, 4, {} }, { "PredictTargetPos", kDTD_Bool, 1, 0, 8, {} },
        { "UseHighArc", kDTD_Bool, 1, 0, 5, {} }, { "UseBoneOrient", kDTD_Bool, 1, 0, 6, {} },
        { "InvertBoneOrient", kDTD_Bool, 1, 0, 7, {} }, { "ForceLaunchAngle", kDTD_Float, 1, 0, 9, {} },
        { "AttackFailAnim", kDTD_String, 1, 0, 10, {} },
        { "AttackFailEffectName", kDTD_String, 1, 0, 11, {} },
        { "DirX", kDTD_Float, 1, 0, 12, {} }, { "DirY", kDTD_Float, 1, 0, 13, {} },
        { "DirZ", kDTD_Float, 1, 0, 14, {} }, { "TargetOffsetX", kDTD_Float, 1, 0, 15, {} },
        { "TargetOffsetY", kDTD_Float, 1, 0, 16, {} }, { "TargetOffsetZ", kDTD_Float, 1, 0, 17, {} } } },
    { "RFireWeapon", 0, { { "on", kDTD_Bool, 1, 1, 0, {} } } },
    { "RSetAnimSpeed", 0, { { "Speed", kDTD_Float, 1, kOne, 0, {} }, { "Time", kDTD_Float, 1, 0, 1, {} } } },
    { "RCreateSoundAlert", 0, { { "Type", kDTD_Hash, 1, 0, 0, {} } } },
    { "RPlayAnimDirect", 0, {
        { "AnimName", kDTD_String, 0, 0, 0, {} }, { "Entity", kDTD_String, 1, 0, 1, {} },
        { "Child", kDTD_Bool, 1, 0, 2, {} } } },
    { "RSetAttackInvincibility", 0, { { "IsEnable", kDTD_Bool, 0, 0, 0, {} } } },
    { "RSetMaxRotationRate", 0, { { "Value", kDTD_Float, 0, 0, 0, {} } } },
    { "RSetAdditiveAlpha", 0, { { "Value", kDTD_Float, 0, 0, 0, {} } } },
    { "RMotionExTuning", 0, { { "Rot", kDTD_Float, 0, 0, 0, {} } } },
    { "RSetGravity", 0, { { "Value", kDTD_Bool, 0, 0, 0, {} } } },
    { "RCameraShake", 0, {
        { "Radius", kDTD_Float, 1, 0, 0, {} }, { "Duration", kDTD_Float, 1, kOne, 1, {} },
        { "Magnitude", kDTD_Float, 1, kOne, 2, {} }, { "FalloffMode", kDTD_Hash, 1, 0x85D98325u, 3, {} },
        { "FalloffStartTime", kDTD_Float, 1, 0, 4, {} }, { "Pitch", kDTD_Float, 1, kOne, 5, {} },
        { "Yaw", kDTD_Float, 1, 0, 6, {} }, { "Roll", kDTD_Float, 1, kOne, 7, {} },
        { "PitchFrequency", kDTD_Float, 1, 0x40800000u, 8, {} },
        { "YawFrequency", kDTD_Float, 1, 0x40D00000u, 9, {} },
        { "RollFrequency", kDTD_Float, 1, 0x40200000u, 10, {} },
        { "FalloffStartDistance", kDTD_Float, 1, 0xBF800000u, 11, {} } } },
    { "RControllerRumble", 0, {
        { "ControllerIdx", kDTD_Int1, 1, 0, 0, {} }, { "Strength", kDTD_Float, 1, 0, 1, {} },
        { "Duration", kDTD_Float, 1, 0, 2, {} } } },
    { "RPlayFootstep", 0, {
        { "foot", kDTD_Hash, 1, 0, 0, {} }, { "attachType", kDTD_Hash, 1, 0, 1, {} },
        { "attachTypePlayer", kDTD_Hash, 1, 0, 2, {} } } },
    { "RFootIK", 0, { { "foot", kDTD_Hash, 0, 0, 0, {} }, { "enabled", kDTD_Bool, 0, 0, 1, {} } } },
    { "REnableComponent", 0, { { "name", kDTD_Hash, 0, 0, 0, {} }, { "value", kDTD_Bool, 0, 0, 1, {} } } },
    { "RSetProjectileParam", 0, {
        { "scalar", kDTD_Float, 0, 0, 0, {} }, { "reflectEnable", kDTD_Bool, 0, 0, 1, {} },
        { "reflectRandomEnable", kDTD_Bool, 1, 1, 2, {} } } },
    { "RStopDirScript", 0, {} },
    { "RSetPosition", 0, { { "useSpawnMatrix", kDTD_Bool, 1, 0, 0, {} } } },
};

void AddAttributes(KTypeDef* def, const TypeSpec& spec)
{
    for (const AttrSpec& a : spec.attrs) {
        if (!a.name)
            break;
        ape::DTDAttribute* attr = def->dtd->AddAttribute(a.name, a.type, a.optional);
        attr->defaultValue.u = a.defaultBits;
        int enumCount = 0;
        while (enumCount < 6 && a.enums[enumCount])
            ++enumCount;
        if (enumCount) {
            attr->AllocEnumNames((uint32_t)enumCount);
            for (int i = 0; i < enumCount; ++i)
                attr->AddEnumName(a.enums[i]);
        }
        if (def->defBlock && a.slot >= 0)
            def->defBlock->slots[a.slot] = (uint8_t)attr->column;
    }
    if (def->defBlock)
        def->defBlock->valueCount = 0x10;   // slot map offset
}

template <int N>
KTypeDef* CreateCause(int16_t index)
{
    KTypeDef* def = new KTypeDef();
    InitCauseDef(def, index, kCauseSpecs[N].name, kCauseSpecs[N].causeFlags);
    AddAttributes(def, kCauseSpecs[N]);
    return def;
}

template <int N>
KTypeDef* CreateResult(int16_t index)
{
    KTypeDef* def = new KTypeDef();
    InitResultDef(def, index, kResultSpecs[N].name);
    AddAttributes(def, kResultSpecs[N]);
    return def;
}

} // namespace

// Runtime entries (0x007737A4 for causes: type 6, flags & 0x3F; 0x007743CC
// for results: type 7, flags & 0x3FFF; payload cleared).
int16_t AllocRuntimeEntry(KnowledgeData* list, int32_t entryType, int16_t typeIndex)
{
    const int16_t index = (int16_t)list->entryCount;
    if ((size_t)list->entryCount >= list->entries.size())
        return -1;
    ++list->entryCount;
    KEntry& entry = list->entries[(size_t)index];
    entry.type = entryType;
    entry.runtime = KRuntime();
    entry.runtime.type = typeIndex;
    return index;
}

static int16_t AllocCauseRuntime(int16_t index, KnowledgeData* list)
{
    return AllocRuntimeEntry(list, kEntry_CauseRuntime, index);
}

static int16_t AllocResultRuntime(int16_t index, KnowledgeData* list)
{
    return AllocRuntimeEntry(list, kEntry_ResultRuntime, index);
}

#define CAUSE(n, link, clone, eval) { CreateCause<n>, AllocCauseRuntime, nullptr, link, clone, eval, nullptr }
#define RESULT(n, release, link, clone, exec) \
    { CreateResult<n>, AllocResultRuntime, release, nullptr, link, clone, nullptr, exec, nullptr, nullptr, nullptr }

KCauseType g_causeTypes[kCauseTypeCount] = {
    CAUSE(0, nullptr, nullptr, CAlwaysEvaluate),
    CAUSE(1, nullptr, nullptr, CNeverEvaluate),
    CAUSE(2, nullptr, nullptr, COnEnterStateEvaluate),
    CAUSE(3, nullptr, nullptr, COnExitStateEvaluate),
    CAUSE(4, nullptr, nullptr, CEveryXSecondEvaluate),
    CAUSE(5, CIsVarLink, nullptr, CIsVarEvaluate),
    CAUSE(6, CIsTimerLink, nullptr, CIsTimerEvaluate),
};

KResultType g_resultTypes[kResultTypeCount] = {
    RESULT(0, nullptr, nullptr, nullptr, RDoNothingExecute),
    RESULT(1, nullptr, nullptr, nullptr, RNachiDebugExecute),
    RESULT(2, nullptr, nullptr, nullptr, RBreakPointExecute),
    RESULT(3, nullptr, RChangeStateLink, nullptr, RChangeStateExecute),
    RESULT(4, nullptr, RSetVarDataIntLink, nullptr, RSetVarDataIntExecute),
    RESULT(5, nullptr, RSetVarDataFloatLink, nullptr, RSetVarDataFloatExecute),
    RESULT(6, nullptr, RSetVarDataBoolLink, nullptr, RSetVarDataBoolExecute),
    RESULT(7, nullptr, RSetVarDataHashLink, nullptr, RSetVarDataHashExecute),
    RESULT(8, nullptr, RIncrementVarDataLink, nullptr, RIncrementVarDataExecute),
    RESULT(9, nullptr, nullptr, nullptr, RDisplayDebugTextExecute),
    RESULT(10, RPlaySndSFXRelease, RPlaySndSFXLink, RPlaySndSFXClone, RPlaySndSFXExecute),
    RESULT(11, nullptr, nullptr, nullptr, RStopSndSFXExecute),
    RESULT(12, nullptr, RPlayDirScriptLink, nullptr, RPlayDirScriptExecute),
    RESULT(13, nullptr, nullptr, nullptr, RTurnOnHighlightExecute),
    RESULT(14, nullptr, nullptr, RResetTimerClone, RResetTimerExecute),
    RESULT(15, nullptr, nullptr, nullptr, RSendEventExecute),
    RESULT(16, nullptr, nullptr, nullptr, RBroadcastEventExecute),
    RESULT(17, nullptr, nullptr, nullptr, RChangeDirectorStateExecute),
    RESULT(18, nullptr, RStartMeleeAttackLink, RStartMeleeAttackClone, RStartMeleeAttackExecute),
    RESULT(19, nullptr, REndMeleeAttackLink, nullptr, REndMeleeAttackExecute),
    RESULT(20, nullptr, RFireProjectileLink, RFireProjectileClone, RFireProjectileExecute),
    RESULT(21, nullptr, RFireWeaponLink, nullptr, RFireWeaponExecute),
    RESULT(22, nullptr, nullptr, nullptr, RSetAnimSpeedExecute),
    RESULT(23, nullptr, nullptr, nullptr, RCreateSoundAlertExecute),
    RESULT(24, nullptr, nullptr, nullptr, RPlayAnimDirectExecute),
    RESULT(25, nullptr, nullptr, nullptr, RSetAttackInvincibilityExecute),
    RESULT(26, nullptr, nullptr, nullptr, RSetMaxRotationRateExecute),
    RESULT(27, nullptr, nullptr, nullptr, RSetAdditiveAlphaExecute),
    RESULT(28, nullptr, nullptr, nullptr, RMotionExTuningExecute),
    RESULT(29, nullptr, nullptr, nullptr, RSetGravityExecute),
    RESULT(30, nullptr, nullptr, nullptr, RCameraShakeExecute),
    RESULT(31, nullptr, nullptr, nullptr, RControllerRumbleExecute),
    RESULT(32, nullptr, nullptr, nullptr, RPlayFootstepExecute),
    RESULT(33, nullptr, nullptr, nullptr, RFootIKExecute),
    RESULT(34, nullptr, nullptr, nullptr, REnableComponentExecute),
    RESULT(35, nullptr, nullptr, nullptr, RSetProjectileParamExecute),
    RESULT(36, nullptr, nullptr, nullptr, RStopDirScriptExecute),
    RESULT(37, nullptr, nullptr, nullptr, RSetPositionExecute),
};

#undef CAUSE
#undef RESULT

} // namespace knowledge
} // namespace sly
