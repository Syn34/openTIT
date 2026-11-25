// openTIT - Knowledge causes: CEveryXSecond, CIsVar, CIsTimer
// (0x0038E8F0-0x003904FF) plus the trivial ones of the table.
#include "sly/knowledge/KnowledgeTypes.h"
#include "sly/knowledge/KnowledgeHooks.h"

#include "ape/core/ApeString.h"

namespace sly {
namespace knowledge {

namespace {
enum CIsVarField { kVarName, kOperator, kFValue, kIValue, kBValue, kHValue };
enum CIsTimerField { kTimerName, kTimerValue, kTimerOperator };

// Compares a variable with the rule's value: `var <op> value`.
template <class T>
bool Compare(uint32_t op, T var, T value)
{
    switch (op) {
    case kOp_LargerEqual: return value <= var;
    case kOp_Equal: return var == value;
    case kOp_Less: return var < value;
    case kOp_LessEqual: return var <= value;
    case kOp_NotEqual: return var != value;
    case kOp_Larger: return value < var;
    default: return false;
    }
}
} // namespace

// The four trivial causes (0x00775714, 0x0077584C, 0x00775984, 0x00775ABC;
// outside this module's range): CNever is never true, the others always
// (the state causes are only asked when their state is entered/left).
bool CAlwaysEvaluate(KRef*, KnowledgeData*) { return true; }
bool CNeverEvaluate(KRef*, KnowledgeData*) { return false; }
bool COnEnterStateEvaluate(KRef*, KnowledgeData*) { return true; }
bool COnExitStateEvaluate(KRef*, KnowledgeData*) { return true; }

// True every "Frequency" seconds.
// @func 0x0038E8F0 CEveryXSecond::Evaluate
bool CEveryXSecondEvaluate(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    runtime->payload.elapsed += FrameTime();
    const bool due = CauseValue(ref, 0).f <= runtime->payload.elapsed;
    if (due)
        runtime->payload.elapsed = 0.0f;
    return due;
}

// Finds the variable and keeps the value to compare with in the type the
// variable has.
// @func 0x0038EAAC CIsVar::Link
void CIsVarLink(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const char* name = CauseValue(ref, kVarName).str;
    if (!name) {
        ReportCauseError(ref, list, "CIsVarDef: Null var name pass in!");
        return;
    }
    const int index = list->vars.Find(ape::HashNameNoCase(name));
    runtime->payload.var.index = (uint16_t)index;
    if (index < 0) {
        ReportCauseError(ref, list,
                         ape::TempFormat("CIsVarDef: Cannot find the var data (%s) from the script!", name));
        return;
    }
    const int32_t type = list->vars.types[(size_t)index];
    if (type == kVar_String) {
        ReportCauseError(ref, list, "CIsVarDef: Don't support string comparison!");
        return;
    }
    KValue& value = runtime->payload.var.value;
    value.u = 0;
    switch (type) {
    case kVar_Hash: value.u = CauseValue(ref, kHValue).u; break;
    case kVar_Bool: value.b = CauseValue(ref, kBValue).b; break;
    case kVar_Float: value.f = CauseValue(ref, kFValue).f; break;
    default:
        // NOTE: int and uint variables compare with the low 16 bits of
        // IValue, sign extended (as on the console).
        value.i = (int16_t)CauseValue(ref, kIValue).i;
        break;
    }
}

// @func 0x0038F3F0 CIsVar::Evaluate
bool CIsVarEvaluate(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const uint32_t op = CauseValue(ref, kOperator).u;
    const uint16_t index = runtime->payload.var.index;
    if (index >= list->vars.values.size())
        return false;
    const KValue& var = list->vars.values[index];
    const KValue& value = runtime->payload.var.value;
    switch (list->vars.types[index]) {
    case kVar_Int: return Compare(op, var.i, value.i);
    case kVar_UInt:
    case kVar_Hash: return Compare(op, var.u, value.u);
    case kVar_Float: return Compare(op, var.f, value.f);
    case kVar_Bool: return Compare(op, (uint8_t)var.b, (uint8_t)value.b);
    default: return false;
    }
}

// @func 0x0038FDBC CIsTimer::Link
void CIsTimerLink(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const char* name = CauseValue(ref, kTimerName).str;
    if (!name) {
        ReportCauseError(ref, list, "CIsTimerLargerEqualThan: No timer is specified!");
        return;
    }
    const int index = list->timers.Find(ape::HashNameNoCase(name));
    runtime->payload.var.index = (uint16_t)index;
    if (index < 0)
        ReportCauseError(ref, list,
                         ape::TempFormat("CIsTimerLargerEqualThan: Cannot find the KTimer variable with name (%s)", name));
}

// Timer >= Value (or <= with Operator="LessEqual").
// @func 0x00390220 CIsTimer::Evaluate
bool CIsTimerEvaluate(KRef* ref, KnowledgeData* list)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const uint16_t index = runtime->payload.var.index;
    if (index >= list->timers.values.size())
        return false;
    const float timer = list->timers.values[index];
    const float value = CauseValue(ref, kTimerValue).f;
    if (CauseValue(ref, kTimerOperator).u == kOp_LessEqual)
        return timer <= value;
    return value <= timer;
}

} // namespace knowledge
} // namespace sly
