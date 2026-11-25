// openTIT - Knowledge type tables (0x00F6503C / 0x00F65100) and the type
// factories that describe each cause/result's attributes.
//
// Every type has a factory (allocates its KTypeDef, declares its attributes
// and fills the slot map that tells the callbacks which column holds which
// field) and a runtime allocator (takes the next list entry as the rule's
// runtime state and constructs the type's runtime data, if it has any). The
// attributes are described by the spec tables below; the per-type functions
// only pick their row.
#include "sly/knowledge/Knowledge.h"
#include "sly/knowledge/KnowledgeTypes.h"

#include "ape/level/ApeDTD.h"

#include <cstring>
#include <new>

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
    const char* defaultText = nullptr;   // string default (only "Entity" has one)
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
        { "AnimName", kDTD_String, 0, 0, 0, {} }, { "Entity", kDTD_String, 1, 0, 1, {}, "" },
        { "Child", kDTD_Bool, 1, 0, 2, {} } } },
    { "RSetAttackInvincibility", 0, { { "IsEnable", kDTD_Bool, 0, 0, 0, {} } } },
    { "RSetMaxRotationRate", 0, { { "Value", kDTD_Float, 0, 0, 0, {} } } },
    { "RSetAdditiveAlpha", 0, { { "Value", kDTD_Float, 0, 0, 0, {} } } },
    { "RMotionExTuning", 0, {
        { "X", kDTD_Float, 0, 0, 0, {} }, { "Y", kDTD_Float, 0, 0, 1, {} },
        { "Z", kDTD_Float, 0, 0, 2, {} }, { "Rot", kDTD_Float, 0, 0, 3, {} } } },
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
        if (a.defaultText)
            attr->defaultValue.str = a.defaultText;
        else
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


KTypeDef* CreateCauseType(int16_t index, const TypeSpec& spec)
{
    KTypeDef* def = new KTypeDef();
    InitCauseDef(def, index, spec.name, spec.causeFlags);
    AddAttributes(def, spec);
    return def;
}

KTypeDef* CreateResultType(int16_t index, const TypeSpec& spec)
{
    KTypeDef* def = new KTypeDef();
    InitResultDef(def, index, spec.name);
    AddAttributes(def, spec);
    return def;
}

// ---------------------------------------------------------------------------
// Runtime data of the types that keep per-rule state. They are constructed in
// KRuntime::payload and laid out like the matching members of that union.

struct CEveryXSecondRuntime { float elapsed = 0.0f; };
struct CIsVarRuntime { uint16_t varIndex = 0xFFFF; KValue value{}; };
struct CIsTimerRuntime { uint16_t timerIndex = 0xFFFF; };

struct RChangeStateRuntime { uint32_t targetHash = 0; };
struct RSetVarDataIntRuntime { uint16_t varIndex = 0; };
struct RSetVarDataFloatRuntime { uint16_t varIndex = 0; };
struct RSetVarDataBoolRuntime { uint16_t varIndex = 0; };
struct RSetVarDataHashRuntime { uint16_t varIndex = 0; };
struct RIncrementVarDataRuntime { uint16_t varIndex = 0; };
struct RPlaySndSFXRuntime { void* soundName = nullptr; };
struct RPlayDirScriptRuntime { void* script = nullptr; };
struct RStartMeleeAttackRuntime { uint32_t boneHash = 0; };
struct REndMeleeAttackRuntime { uint32_t boneHash = 0; };
struct RFireProjectileRuntime { uint32_t field_00 = 0; };   // NOTE: set up by Link (not ported)
struct RFireWeaponRuntime { bool firing = false; };          // NOTE: name guessed

// Takes the next entry of the list as a runtime entry. Only the low state
// bits survive (`keepFlags`: 6 for causes, 14 for results); the rule-broken
// bit is always cleared.
KRuntime* TakeRuntimeEntry(KnowledgeData* list, int32_t entryType, uint16_t keepFlags, int16_t typeIndex,
                           int16_t* entryIndex)
{
    *entryIndex = (int16_t)list->entryCount;
    // NOTE: the console has no capacity check (lists are sized when loaded).
    if ((size_t)list->entryCount >= list->entries.size()) {
        *entryIndex = -1;
        return nullptr;
    }
    ++list->entryCount;
    KEntry& entry = list->entries[(size_t)*entryIndex];
    entry.type = entryType;
    entry.runtime.flags &= keepFlags;
    entry.runtime.type = typeIndex;
    return &entry.runtime;
}

constexpr uint16_t kCauseKeepFlags = 0x003F;
constexpr uint16_t kResultKeepFlags = 0x3FFF;

int16_t NewCauseEntry(int16_t typeIndex, KnowledgeData* list)
{
    int16_t index;
    TakeRuntimeEntry(list, kEntry_CauseRuntime, kCauseKeepFlags, typeIndex, &index);
    return index;
}

int16_t NewResultEntry(int16_t typeIndex, KnowledgeData* list)
{
    int16_t index;
    TakeRuntimeEntry(list, kEntry_ResultRuntime, kResultKeepFlags, typeIndex, &index);
    return index;
}

template <class T>
T* ConstructRuntimeData(KRuntime* runtime)
{
    static_assert(sizeof(T) <= sizeof(KRuntime::Payload), "runtime data too large");
    return runtime ? new (&runtime->payload) T() : nullptr;
}

// @func 0x007737A4 AllocCauseRuntime<CEveryXSecondRuntime>
// @func 0x00773898 AllocCauseRuntime<CIsVarRuntime>
// @func 0x00773998 AllocCauseRuntime<CIsTimerRuntime>
template <class T>
int16_t AllocCauseRuntime(int16_t typeIndex, KnowledgeData* list, T** data)
{
    int16_t index;
    KRuntime* runtime = TakeRuntimeEntry(list, kEntry_CauseRuntime, kCauseKeepFlags, typeIndex, &index);
    if (data)
        *data = ConstructRuntimeData<T>(runtime);
    return index;
}

// @func 0x007742DC AllocResultRuntime<RChangeStateRuntime>
// @func 0x007743CC AllocResultRuntime<RSetVarDataIntRuntime>
// @func 0x007744BC AllocResultRuntime<RSetVarDataFloatRuntime>
// @func 0x007745AC AllocResultRuntime<RSetVarDataBoolRuntime>
// @func 0x0077469C AllocResultRuntime<RSetVarDataHashRuntime>
// @func 0x0077478C AllocResultRuntime<RIncrementVarDataRuntime>
// @func 0x0077487C AllocResultRuntime<RPlaySndSFXRuntime>
// @func 0x0077496C AllocResultRuntime<RPlayDirScriptRuntime>
// @func 0x00774A5C AllocResultRuntime<RStartMeleeAttackRuntime>
// @func 0x00774B4C AllocResultRuntime<REndMeleeAttackRuntime>
// @func 0x00774C3C AllocResultRuntime<RFireProjectileRuntime>
// @func 0x00774D2C AllocResultRuntime<RFireWeaponRuntime>
template <class T>
int16_t AllocResultRuntime(int16_t typeIndex, KnowledgeData* list, T** data)
{
    int16_t index;
    KRuntime* runtime = TakeRuntimeEntry(list, kEntry_ResultRuntime, kResultKeepFlags, typeIndex, &index);
    if (data)
        *data = ConstructRuntimeData<T>(runtime);
    return index;
}

// ---------------------------------------------------------------------------
// Per-type factories and runtime allocators (the table rows point at these).

// @func 0x007755E4 CAlways::Create
KTypeDef* CAlwaysCreate(int16_t index) { return CreateCauseType(index, kCauseSpecs[0]); }
// @func 0x0077565C CAlways::AllocRuntime
int16_t CAlwaysAllocRuntime(int16_t index, KnowledgeData* list) { return NewCauseEntry(index, list); }

// @func 0x0077571C CNever::Create
KTypeDef* CNeverCreate(int16_t index) { return CreateCauseType(index, kCauseSpecs[1]); }
// @func 0x00775794 CNever::AllocRuntime
int16_t CNeverAllocRuntime(int16_t index, KnowledgeData* list) { return NewCauseEntry(index, list); }

// @func 0x00775854 COnEnterState::Create
KTypeDef* COnEnterStateCreate(int16_t index) { return CreateCauseType(index, kCauseSpecs[2]); }
// @func 0x007758CC COnEnterState::AllocRuntime
int16_t COnEnterStateAllocRuntime(int16_t index, KnowledgeData* list) { return NewCauseEntry(index, list); }

// @func 0x0077598C COnExitState::Create
KTypeDef* COnExitStateCreate(int16_t index) { return CreateCauseType(index, kCauseSpecs[3]); }
// @func 0x00775A04 COnExitState::AllocRuntime
int16_t COnExitStateAllocRuntime(int16_t index, KnowledgeData* list) { return NewCauseEntry(index, list); }

// @func 0x00775AC4 CEveryXSecond::Create
KTypeDef* CEveryXSecondCreate(int16_t index) { return CreateCauseType(index, kCauseSpecs[4]); }
// @func 0x00775BC4 CEveryXSecond::AllocRuntime
int16_t CEveryXSecondAllocRuntime(int16_t index, KnowledgeData* list)
{
    CEveryXSecondRuntime* data = nullptr;
    return AllocCauseRuntime(index, list, &data);
}

// @func 0x00775BF0 CIsVar::Create
KTypeDef* CIsVarCreate(int16_t index) { return CreateCauseType(index, kCauseSpecs[5]); }
// @func 0x00776068 CIsVar::AllocRuntime
int16_t CIsVarAllocRuntime(int16_t index, KnowledgeData* list)
{
    CIsVarRuntime* data = nullptr;
    return AllocCauseRuntime(index, list, &data);
}

// @func 0x00776094 CIsTimer::Create
KTypeDef* CIsTimerCreate(int16_t index) { return CreateCauseType(index, kCauseSpecs[6]); }
// @func 0x007762F4 CIsTimer::AllocRuntime
int16_t CIsTimerAllocRuntime(int16_t index, KnowledgeData* list)
{
    CIsTimerRuntime* data = nullptr;
    return AllocCauseRuntime(index, list, &data);
}

// @func 0x00776320 RDoNothing::Create
KTypeDef* RDoNothingCreate(int16_t index) { return CreateResultType(index, kResultSpecs[0]); }
// @func 0x00776390 RDoNothing::AllocRuntime
int16_t RDoNothingAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x0077644C RNachiDebug::Create
KTypeDef* RNachiDebugCreate(int16_t index) { return CreateResultType(index, kResultSpecs[1]); }
// @func 0x007764BC RNachiDebug::AllocRuntime
int16_t RNachiDebugAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00776570 RBreakPoint::Create
KTypeDef* RBreakPointCreate(int16_t index) { return CreateResultType(index, kResultSpecs[2]); }
// @func 0x00776668 RBreakPoint::AllocRuntime
int16_t RBreakPointAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x0077671C RChangeState::Create
KTypeDef* RChangeStateCreate(int16_t index) { return CreateResultType(index, kResultSpecs[3]); }
// @func 0x00776814 RChangeState::AllocRuntime
int16_t RChangeStateAllocRuntime(int16_t index, KnowledgeData* list)
{
    RChangeStateRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00776840 RSetVarDataInt::Create
KTypeDef* RSetVarDataIntCreate(int16_t index) { return CreateResultType(index, kResultSpecs[4]); }
// @func 0x00776968 RSetVarDataInt::AllocRuntime
int16_t RSetVarDataIntAllocRuntime(int16_t index, KnowledgeData* list)
{
    RSetVarDataIntRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00776994 RSetVarDataFloat::Create
KTypeDef* RSetVarDataFloatCreate(int16_t index) { return CreateResultType(index, kResultSpecs[5]); }
// @func 0x00776ABC RSetVarDataFloat::AllocRuntime
int16_t RSetVarDataFloatAllocRuntime(int16_t index, KnowledgeData* list)
{
    RSetVarDataFloatRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00776AE8 RSetVarDataBool::Create
KTypeDef* RSetVarDataBoolCreate(int16_t index) { return CreateResultType(index, kResultSpecs[6]); }
// @func 0x00776C10 RSetVarDataBool::AllocRuntime
int16_t RSetVarDataBoolAllocRuntime(int16_t index, KnowledgeData* list)
{
    RSetVarDataBoolRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00776C3C RSetVarDataHash::Create
KTypeDef* RSetVarDataHashCreate(int16_t index) { return CreateResultType(index, kResultSpecs[7]); }
// @func 0x00776D64 RSetVarDataHash::AllocRuntime
int16_t RSetVarDataHashAllocRuntime(int16_t index, KnowledgeData* list)
{
    RSetVarDataHashRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00776D90 RIncrementVarData::Create
KTypeDef* RIncrementVarDataCreate(int16_t index) { return CreateResultType(index, kResultSpecs[8]); }
// @func 0x00776E88 RIncrementVarData::AllocRuntime
int16_t RIncrementVarDataAllocRuntime(int16_t index, KnowledgeData* list)
{
    RIncrementVarDataRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00776EB4 RDisplayDebugText::Create
KTypeDef* RDisplayDebugTextCreate(int16_t index) { return CreateResultType(index, kResultSpecs[9]); }
// @func 0x00777070 RDisplayDebugText::AllocRuntime
int16_t RDisplayDebugTextAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x007778CC RPlaySndSFX::Create
KTypeDef* RPlaySndSFXCreate(int16_t index) { return CreateResultType(index, kResultSpecs[10]); }
// @func 0x00777B80 RPlaySndSFX::AllocRuntime
int16_t RPlaySndSFXAllocRuntime(int16_t index, KnowledgeData* list)
{
    RPlaySndSFXRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00777BAC RStopSndSFX::Create
KTypeDef* RStopSndSFXCreate(int16_t index) { return CreateResultType(index, kResultSpecs[11]); }
// @func 0x00777CA4 RStopSndSFX::AllocRuntime
int16_t RStopSndSFXAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00777D58 RPlayDirScript::Create
KTypeDef* RPlayDirScriptCreate(int16_t index) { return CreateResultType(index, kResultSpecs[12]); }
// @func 0x00777F7C RPlayDirScript::AllocRuntime
int16_t RPlayDirScriptAllocRuntime(int16_t index, KnowledgeData* list)
{
    RPlayDirScriptRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00777FA8 RTurnOnHighlight::Create
KTypeDef* RTurnOnHighlightCreate(int16_t index) { return CreateResultType(index, kResultSpecs[13]); }
// @func 0x007780A0 RTurnOnHighlight::AllocRuntime
int16_t RTurnOnHighlightAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00778154 RResetTimer::Create
KTypeDef* RResetTimerCreate(int16_t index) { return CreateResultType(index, kResultSpecs[14]); }
// @func 0x0077824C RResetTimer::AllocRuntime
int16_t RResetTimerAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00777124 RSendEvent::Create
KTypeDef* RSendEventCreate(int16_t index) { return CreateResultType(index, kResultSpecs[15]); }
// @func 0x0077724C RSendEvent::AllocRuntime
int16_t RSendEventAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00777300 RBroadcastEvent::Create
KTypeDef* RBroadcastEventCreate(int16_t index) { return CreateResultType(index, kResultSpecs[16]); }
// @func 0x00777458 RBroadcastEvent::AllocRuntime
int16_t RBroadcastEventAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x0077750C RChangeDirectorState::Create
KTypeDef* RChangeDirectorStateCreate(int16_t index) { return CreateResultType(index, kResultSpecs[17]); }
// @func 0x00777604 RChangeDirectorState::AllocRuntime
int16_t RChangeDirectorStateAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x0077A03C RStartMeleeAttack::Create
KTypeDef* RStartMeleeAttackCreate(int16_t index) { return CreateResultType(index, kResultSpecs[18]); }
// @func 0x0077A2B4 RStartMeleeAttack::AllocRuntime
int16_t RStartMeleeAttackAllocRuntime(int16_t index, KnowledgeData* list)
{
    RStartMeleeAttackRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x0077A2E0 REndMeleeAttack::Create
KTypeDef* REndMeleeAttackCreate(int16_t index) { return CreateResultType(index, kResultSpecs[19]); }
// @func 0x0077A3D8 REndMeleeAttack::AllocRuntime
int16_t REndMeleeAttackAllocRuntime(int16_t index, KnowledgeData* list)
{
    REndMeleeAttackRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x0077A404 RFireProjectile::Create
KTypeDef* RFireProjectileCreate(int16_t index) { return CreateResultType(index, kResultSpecs[20]); }
// @func 0x0077A82C RFireProjectile::AllocRuntime
int16_t RFireProjectileAllocRuntime(int16_t index, KnowledgeData* list)
{
    RFireProjectileRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x0077A858 RFireWeapon::Create
KTypeDef* RFireWeaponCreate(int16_t index) { return CreateResultType(index, kResultSpecs[21]); }
// @func 0x0077A950 RFireWeapon::AllocRuntime
int16_t RFireWeaponAllocRuntime(int16_t index, KnowledgeData* list)
{
    RFireWeaponRuntime* data = nullptr;
    return AllocResultRuntime(index, list, &data);
}

// @func 0x00779CB0 RSetAnimSpeed::Create
KTypeDef* RSetAnimSpeedCreate(int16_t index) { return CreateResultType(index, kResultSpecs[22]); }
// @func 0x00779DDC RSetAnimSpeed::AllocRuntime
int16_t RSetAnimSpeedAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00779E90 RCreateSoundAlert::Create
KTypeDef* RCreateSoundAlertCreate(int16_t index) { return CreateResultType(index, kResultSpecs[23]); }
// @func 0x00779F88 RCreateSoundAlert::AllocRuntime
int16_t RCreateSoundAlertAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x007776B8 RPlayAnimDirect::Create
KTypeDef* RPlayAnimDirectCreate(int16_t index) { return CreateResultType(index, kResultSpecs[24]); }
// @func 0x00777818 RPlayAnimDirect::AllocRuntime
int16_t RPlayAnimDirectAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00778300 RSetAttackInvincibility::Create
KTypeDef* RSetAttackInvincibilityCreate(int16_t index) { return CreateResultType(index, kResultSpecs[25]); }
// @func 0x007783F8 RSetAttackInvincibility::AllocRuntime
int16_t RSetAttackInvincibilityAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00778658 RSetMaxRotationRate::Create
KTypeDef* RSetMaxRotationRateCreate(int16_t index) { return CreateResultType(index, kResultSpecs[26]); }
// @func 0x00778750 RSetMaxRotationRate::AllocRuntime
int16_t RSetMaxRotationRateAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x007784AC RSetAdditiveAlpha::Create
KTypeDef* RSetAdditiveAlphaCreate(int16_t index) { return CreateResultType(index, kResultSpecs[27]); }
// @func 0x007785A4 RSetAdditiveAlpha::AllocRuntime
int16_t RSetAdditiveAlphaAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00778804 RMotionExTuning::Create
KTypeDef* RMotionExTuningCreate(int16_t index) { return CreateResultType(index, kResultSpecs[28]); }
// @func 0x0077898C RMotionExTuning::AllocRuntime
int16_t RMotionExTuningAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00778A40 RSetGravity::Create
KTypeDef* RSetGravityCreate(int16_t index) { return CreateResultType(index, kResultSpecs[29]); }
// @func 0x00778B38 RSetGravity::AllocRuntime
int16_t RSetGravityAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00778BEC RCameraShake::Create
KTypeDef* RCameraShakeCreate(int16_t index) { return CreateResultType(index, kResultSpecs[30]); }
// @func 0x00778F18 RCameraShake::AllocRuntime
int16_t RCameraShakeAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00778FCC RControllerRumble::Create
KTypeDef* RControllerRumbleCreate(int16_t index) { return CreateResultType(index, kResultSpecs[31]); }
// @func 0x00779124 RControllerRumble::AllocRuntime
int16_t RControllerRumbleAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x007791D8 RPlayFootstep::Create
KTypeDef* RPlayFootstepCreate(int16_t index) { return CreateResultType(index, kResultSpecs[32]); }
// @func 0x00779330 RPlayFootstep::AllocRuntime
int16_t RPlayFootstepAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x007793E4 RFootIK::Create
KTypeDef* RFootIKCreate(int16_t index) { return CreateResultType(index, kResultSpecs[33]); }
// @func 0x0077950C RFootIK::AllocRuntime
int16_t RFootIKAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x007795C0 REnableComponent::Create
KTypeDef* REnableComponentCreate(int16_t index) { return CreateResultType(index, kResultSpecs[34]); }
// @func 0x007796E8 REnableComponent::AllocRuntime
int16_t REnableComponentAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x0077979C RSetProjectileParam::Create
KTypeDef* RSetProjectileParamCreate(int16_t index) { return CreateResultType(index, kResultSpecs[35]); }
// @func 0x007798F8 RSetProjectileParam::AllocRuntime
int16_t RSetProjectileParamAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x007799AC RStopDirScript::Create
KTypeDef* RStopDirScriptCreate(int16_t index) { return CreateResultType(index, kResultSpecs[36]); }
// @func 0x00779A50 RStopDirScript::AllocRuntime
int16_t RStopDirScriptAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

// @func 0x00779B04 RSetPosition::Create
KTypeDef* RSetPositionCreate(int16_t index) { return CreateResultType(index, kResultSpecs[37]); }
// @func 0x00779BFC RSetPosition::AllocRuntime
int16_t RSetPositionAllocRuntime(int16_t index, KnowledgeData* list) { return NewResultEntry(index, list); }

} // namespace

// Generic runtime entry without type data (used by the loader for types
// that have none).
int16_t AllocRuntimeEntry(KnowledgeData* list, int32_t entryType, int16_t typeIndex)
{
    return entryType == kEntry_CauseRuntime ? NewCauseEntry(typeIndex, list) : NewResultEntry(typeIndex, list);
}

#define CAUSE(n, link, clone, eval) { n##Create, n##AllocRuntime, nullptr, link, clone, eval, nullptr }
#define RESULT(n, release, link, clone, exec) \
    { n##Create, n##AllocRuntime, release, nullptr, link, clone, nullptr, exec, nullptr, nullptr, nullptr }

KCauseType g_causeTypes[kCauseTypeCount] = {
    CAUSE(CAlways, nullptr, nullptr, CAlwaysEvaluate),
    CAUSE(CNever, nullptr, nullptr, CNeverEvaluate),
    CAUSE(COnEnterState, nullptr, nullptr, COnEnterStateEvaluate),
    CAUSE(COnExitState, nullptr, nullptr, COnExitStateEvaluate),
    CAUSE(CEveryXSecond, nullptr, nullptr, CEveryXSecondEvaluate),
    CAUSE(CIsVar, CIsVarLink, nullptr, CIsVarEvaluate),
    CAUSE(CIsTimer, CIsTimerLink, nullptr, CIsTimerEvaluate),
};

KResultType g_resultTypes[kResultTypeCount] = {
    RESULT(RDoNothing, nullptr, nullptr, nullptr, RDoNothingExecute),
    RESULT(RNachiDebug, nullptr, nullptr, nullptr, RNachiDebugExecute),
    RESULT(RBreakPoint, nullptr, nullptr, nullptr, RBreakPointExecute),
    RESULT(RChangeState, nullptr, RChangeStateLink, nullptr, RChangeStateExecute),
    RESULT(RSetVarDataInt, nullptr, RSetVarDataIntLink, nullptr, RSetVarDataIntExecute),
    RESULT(RSetVarDataFloat, nullptr, RSetVarDataFloatLink, nullptr, RSetVarDataFloatExecute),
    RESULT(RSetVarDataBool, nullptr, RSetVarDataBoolLink, nullptr, RSetVarDataBoolExecute),
    RESULT(RSetVarDataHash, nullptr, RSetVarDataHashLink, nullptr, RSetVarDataHashExecute),
    RESULT(RIncrementVarData, nullptr, RIncrementVarDataLink, nullptr, RIncrementVarDataExecute),
    RESULT(RDisplayDebugText, nullptr, nullptr, nullptr, RDisplayDebugTextExecute),
    RESULT(RPlaySndSFX, RPlaySndSFXRelease, RPlaySndSFXLink, RPlaySndSFXClone, RPlaySndSFXExecute),
    RESULT(RStopSndSFX, nullptr, nullptr, nullptr, RStopSndSFXExecute),
    RESULT(RPlayDirScript, nullptr, RPlayDirScriptLink, nullptr, RPlayDirScriptExecute),
    RESULT(RTurnOnHighlight, nullptr, nullptr, nullptr, RTurnOnHighlightExecute),
    RESULT(RResetTimer, nullptr, nullptr, RResetTimerClone, RResetTimerExecute),
    RESULT(RSendEvent, nullptr, nullptr, nullptr, RSendEventExecute),
    RESULT(RBroadcastEvent, nullptr, nullptr, nullptr, RBroadcastEventExecute),
    RESULT(RChangeDirectorState, nullptr, nullptr, nullptr, RChangeDirectorStateExecute),
    RESULT(RStartMeleeAttack, nullptr, RStartMeleeAttackLink, RStartMeleeAttackClone, RStartMeleeAttackExecute),
    RESULT(REndMeleeAttack, nullptr, REndMeleeAttackLink, nullptr, REndMeleeAttackExecute),
    RESULT(RFireProjectile, nullptr, RFireProjectileLink, RFireProjectileClone, RFireProjectileExecute),
    RESULT(RFireWeapon, nullptr, RFireWeaponLink, nullptr, RFireWeaponExecute),
    RESULT(RSetAnimSpeed, nullptr, nullptr, nullptr, RSetAnimSpeedExecute),
    RESULT(RCreateSoundAlert, nullptr, nullptr, nullptr, RCreateSoundAlertExecute),
    RESULT(RPlayAnimDirect, nullptr, nullptr, nullptr, RPlayAnimDirectExecute),
    RESULT(RSetAttackInvincibility, nullptr, nullptr, nullptr, RSetAttackInvincibilityExecute),
    RESULT(RSetMaxRotationRate, nullptr, nullptr, nullptr, RSetMaxRotationRateExecute),
    RESULT(RSetAdditiveAlpha, nullptr, nullptr, nullptr, RSetAdditiveAlphaExecute),
    RESULT(RMotionExTuning, nullptr, nullptr, nullptr, RMotionExTuningExecute),
    RESULT(RSetGravity, nullptr, nullptr, nullptr, RSetGravityExecute),
    RESULT(RCameraShake, nullptr, nullptr, nullptr, RCameraShakeExecute),
    RESULT(RControllerRumble, nullptr, nullptr, nullptr, RControllerRumbleExecute),
    RESULT(RPlayFootstep, nullptr, nullptr, nullptr, RPlayFootstepExecute),
    RESULT(RFootIK, nullptr, nullptr, nullptr, RFootIKExecute),
    RESULT(REnableComponent, nullptr, nullptr, nullptr, REnableComponentExecute),
    RESULT(RSetProjectileParam, nullptr, nullptr, nullptr, RSetProjectileParamExecute),
    RESULT(RStopDirScript, nullptr, nullptr, nullptr, RStopDirScriptExecute),
    RESULT(RSetPosition, nullptr, nullptr, nullptr, RSetPositionExecute),
};

#undef CAUSE
#undef RESULT

} // namespace knowledge
} // namespace sly
