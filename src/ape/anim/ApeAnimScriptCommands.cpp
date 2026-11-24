// openTIT - Built-in anim script commands that only touch the script system.
#include "ape/anim/ApeAnimScriptCommands.h"

#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

namespace ape {

namespace {
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrValue = AnimHash("Value");
constexpr uint32_t kAttrVariable = AnimHash("variable");
constexpr uint32_t kAttrReverse = AnimHash("Reverse");
constexpr uint32_t kAttrMin = AnimHash("min");
constexpr uint32_t kAttrMax = AnimHash("max");
constexpr uint32_t kAttrScript = AnimHash("script");
constexpr uint32_t kAttrScriptVar = AnimHash("ScriptVar");
constexpr uint32_t kAttrScriptLib = AnimHash("ScriptLib");
constexpr uint32_t kAttrDetach = AnimHash("detach");
constexpr uint32_t kElemParam = AnimHash("Param");
constexpr uint32_t kAttrChild = AnimHash("child");
constexpr uint32_t kAttrStopAll = 0x2A9C853Eu;  // NOTE: name unknown
constexpr uint32_t kAttrState = AnimHash("state");
constexpr uint32_t kAttrChildScript = AnimHash("ChildScript");
constexpr uint32_t kAttrDelayTimeVar = AnimHash("DelayTimeVar");
constexpr uint32_t kAttrDelayTime = AnimHash("DelayTime");
constexpr uint32_t kAttrRandom = AnimHash("Random");
constexpr uint32_t kAttrValueVar = AnimHash("ValueVar");
constexpr uint32_t kAttrStr = AnimHash("str");
constexpr uint32_t kAttrOperation = AnimHash("operation");

void WarnName(uint32_t nameHash)
{
    (void)TempHexString(nameHash);
}

void LinkCommandList(const AnimScriptCommandList& list)
{
    for (ApeAnimScriptCommand* command : list)
        command->Link();
}

// Gives the callee instance the caller's variable `var` under `name`.
// An existing object variable takes over the reference instead.
void PassParameter(ApeAnimScriptInstance* callee, uint32_t name, ApeAnimScriptVar* var)
{
    AnimScriptVarMap& vars = callee->Vars();
    if (!vars.Contains(name)) {
        vars.Add(name, var);
        var->AddRef();
        return;
    }
    ApeAnimScriptVar* existing = vars.Find(name);
    if (existing->GetType() == AnimScriptVarType::Object && var->GetType() == AnimScriptVarType::Object) {
        static_cast<ApeAnimScriptObjectVar*>(existing)->CopyReference(*static_cast<ApeAnimScriptObjectVar*>(var));
        return;
    }
    existing->OnOwnerDestroyed();
    existing->Release();
    vars.Remove(name);
    vars.Add(name, var);
    var->AddRef();
}
} // namespace

AnimScriptOperation ReadOperation(xml::XmlElement* element)
{
    uint32_t operation = 0;
    element->GetHash(kAttrOperation, &operation, 0);
    switch (operation) {
    case AnimHash("Divide"): return AnimScriptOperation::Divide;
    case AnimHash("Multiply"): return AnimScriptOperation::Multiply;
    case AnimHash("Add"): return AnimScriptOperation::Add;
    case AnimHash("Subtract"): return AnimScriptOperation::Subtract;
    default: return AnimScriptOperation::Set;
    }
}

// ---------------------------------------------------------------------------
// IfEquals

// @func 0x004C5DF0 ApeAnimScriptIfEqualsCommand::ApeAnimScriptIfEqualsCommand
ApeAnimScriptIfEqualsCommand::ApeAnimScriptIfEqualsCommand(xml::XmlElement* element)
{
    element->GetHash(kAttrVariable, &m_variable, 0);
    // The comparison value takes the type the data gives it.
    xml::AttributeType type = xml::kAttr_None;
    if (element->GetRawAttribute(kAttrValue, &type)) {
        if (type == xml::kAttr_Int || type == xml::kAttr_Float) {
            element->GetFloat(kAttrValue, &m_floatValue, 0.0f);
        } else if (type == xml::kAttr_Vec3) {
            float value[3] = {0.0f, 0.0f, 0.0f};
            element->GetVec3(kAttrValue, value, value);
            m_vecValue = {value[0], value[1], value[2], 0.0f};
        } else {
            element->GetHash(kAttrValue, &m_hashValue, 0);
        }
    }
    element->GetBool(kAttrReverse, &m_reverse, false);
    LoadCommandList(m_commands, element);
}

// @func 0x004C6210 ApeAnimScriptIfEqualsCommand::~ApeAnimScriptIfEqualsCommand
// @func 0x004C62F0 ApeAnimScriptIfEqualsCommand::~ApeAnimScriptIfEqualsCommand (deleting)
ApeAnimScriptIfEqualsCommand::~ApeAnimScriptIfEqualsCommand()
{
    ReleaseCommandList(m_commands);
}

// @func 0x004C63EC ApeAnimScriptIfEqualsCommand::Link
void ApeAnimScriptIfEqualsCommand::Link()
{
    LinkCommandList(m_commands);
}

// @func 0x004C6474 ApeAnimScriptIfEqualsCommand::Create
ApeAnimScriptCommand* ApeAnimScriptIfEqualsCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptIfEqualsCommand(element);
}

// @func 0x004C64E4 ApeAnimScriptIfEqualsCommand::Execute
void ApeAnimScriptIfEqualsCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    ApeAnimScriptVar* var = instance->FindVar(m_variable);
    if (!var)
        return;
    bool equal;
    if (var->IsNumber()) {
        equal = var->GetFloat() == m_floatValue;
    } else if (var->GetType() == AnimScriptVarType::Vec3) {
        const Vec3& v = static_cast<ApeAnimScriptVec3Var*>(var)->GetVec3();
        equal = v.x == m_vecValue.x && v.y == m_vecValue.y && v.z == m_vecValue.z;
    } else if (var->GetType() == AnimScriptVarType::Hash) {
        equal = var->GetHash() == m_hashValue;
    } else {
        return;
    }
    if (equal != m_reverse)
        ExecuteCommandList(m_commands, instance, state);
}

// TODO(0x004C6708 ApeAnimScriptIfEqualsCommand::CreateDebugElement) "IfEquals" debug element.

// ---------------------------------------------------------------------------
// IfRange

// @func 0x004C6778 ApeAnimScriptIfRangeCommand::ApeAnimScriptIfRangeCommand
ApeAnimScriptIfRangeCommand::ApeAnimScriptIfRangeCommand(xml::XmlElement* element)
{
    if (const char* name = element->GetString("variable"))
        m_variable = HashNameNoCase(name);
    element->GetFloat(kAttrMin, &m_min, 0.0f);
    element->GetFloat(kAttrMax, &m_max, 0.0f);
    element->GetBool(kAttrReverse, &m_reverse, false);
    LoadCommandList(m_commands, element);
}

// @func 0x004C6B2C ApeAnimScriptIfRangeCommand::~ApeAnimScriptIfRangeCommand
// @func 0x004C6C0C ApeAnimScriptIfRangeCommand::~ApeAnimScriptIfRangeCommand (deleting)
ApeAnimScriptIfRangeCommand::~ApeAnimScriptIfRangeCommand()
{
    ReleaseCommandList(m_commands);
}

// @func 0x004C6D08 ApeAnimScriptIfRangeCommand::Link
void ApeAnimScriptIfRangeCommand::Link()
{
    LinkCommandList(m_commands);
}

// @func 0x004C6D90 ApeAnimScriptIfRangeCommand::Create
ApeAnimScriptCommand* ApeAnimScriptIfRangeCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptIfRangeCommand(element);
}

// @func 0x004C6E00 ApeAnimScriptIfRangeCommand::Execute
void ApeAnimScriptIfRangeCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    ApeAnimScriptVar* var = instance->FindVar(m_variable);
    if (!var || !var->IsNumber())
        return;
    const float value = var->GetFloat();
    const bool inside = m_min <= value && value <= m_max;
    if (inside != m_reverse)
        ExecuteCommandList(m_commands, instance, state);
}

// TODO(0x004C6F4C ApeAnimScriptIfRangeCommand::CreateDebugElement) "IfRange" debug element.

// ---------------------------------------------------------------------------
// Call

// @func 0x004C6FB8 ApeAnimScriptCallCommand::ApeAnimScriptCallCommand
ApeAnimScriptCallCommand::ApeAnimScriptCallCommand(xml::XmlElement* element)
{
    for (xml::XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (ElementHash(child) != kElemParam)
            continue;
        uint32_t name = 0;
        uint32_t value = 0;
        child->GetHash(kAttrName, &name, 0);
        child->GetHash(kAttrValue, &value, 0);
        m_params.Add(name, value);
    }
    element->GetHash(kAttrScript, &m_script, 0);
    element->GetHash(kAttrScriptVar, &m_scriptVar, 0);
    element->GetHash(kAttrScriptLib, &m_scriptLib, 0);
    element->GetBool(kAttrDetach, &m_detach, false);
}

// @func 0x004C71F8 ApeAnimScriptCallCommand::~ApeAnimScriptCallCommand
// @func 0x004C7350 ApeAnimScriptCallCommand::~ApeAnimScriptCallCommand (deleting)
ApeAnimScriptCallCommand::~ApeAnimScriptCallCommand() = default;

// @func 0x004C74CC ApeAnimScriptCallCommand::Create
ApeAnimScriptCommand* ApeAnimScriptCallCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptCallCommand(element);
}

// @func 0x004C7BBC ApeAnimScriptCallCommand::Execute
void ApeAnimScriptCallCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    if (!m_script && !m_scriptVar)
        return;
    uint32_t scriptName = m_script;
    if (m_scriptVar) {
        ApeAnimScriptVar* var = instance->FindVar(m_scriptVar);
        if (var && var->GetType() == AnimScriptVarType::Int)
            scriptName = (uint32_t)var->GetInt();
    }
    ApeAnimScript* script = ApeAnimScriptLibrary::FindScriptAnywhere(scriptName, m_scriptLib, true);
    if (!script)
        return;
    auto* callee = new ApeAnimScriptInstance(script);
    // NOTE: without a calling instance the new instance is never attached.
    if (!instance)
        return;

    for (uint32_t callerVar = m_params.First(); !m_params.AtEnd(); callerVar = m_params.Next()) {
        const uint32_t name = m_params.CurrentKey();
        if (!name || !callerVar)
            continue;
        if (ApeAnimScriptVar* var = instance->FindVar(callerVar))
            PassParameter(callee, name, var);
    }

    if (m_detach)
        ApeAnimScriptInstance::Register(callee);
    else
        instance->AddChild(callee);
}

// TODO(0x004C82F8 ApeAnimScriptCallCommand::CreateDebugElement) "Call" debug element.

// ---------------------------------------------------------------------------
// StopChildScript

ApeAnimScriptStopChildScriptCommand::~ApeAnimScriptStopChildScriptCommand() = default;

// @func 0x004C8DB4 ApeAnimScriptStopChildScriptCommand::Create
ApeAnimScriptCommand* ApeAnimScriptStopChildScriptCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptStopChildScriptCommand();
    element->GetHash(kAttrChild, &command->m_child, 0);
    element->GetBool(kAttrStopAll, &command->m_all, false);
    return command;
}

// @func 0x004C94C8 ApeAnimScriptStopChildScriptCommand::Execute
void ApeAnimScriptStopChildScriptCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    if (!instance)
        return;
    if (m_all)
        instance->StopAllChildren();
    else if (m_child)
        instance->StopChildren(m_child);
}

// TODO(0x004C9674 ApeAnimScriptStopChildScriptCommand::CreateDebugElement) "StopChildScript" debug element.

// ---------------------------------------------------------------------------
// ChangeState

// @func 0x004CBB40 ApeAnimScriptChangeStateCommand::ApeAnimScriptChangeStateCommand
ApeAnimScriptChangeStateCommand::ApeAnimScriptChangeStateCommand(xml::XmlElement* element)
{
    m_randomStates.reserve(4);
    element->GetHash(kAttrState, &m_state, 0);
    element->GetHash(kAttrChildScript, &m_childScript, m_childScript);
    element->GetHash(kAttrDelayTimeVar, &m_delayTimeVar, m_delayTimeVar);
    element->GetFloat(kAttrDelayTime, &m_delayTime, m_delayTime);
    element->GetBool(kAttrRandom, &m_random, m_random);
    if (!m_random)
        return;
    // Random mode picks one of the "state" children.
    for (xml::XmlElement* child = element->FirstChildElement(kAttrState); child;
         child = child->NextSiblingElement(kAttrState)) {
        uint32_t name = 0;
        child->GetHash(kAttrName, &name, 0);
        m_randomStates.push_back(name);
    }
}

// @func 0x004CBD50 ApeAnimScriptChangeStateCommand::~ApeAnimScriptChangeStateCommand
// @func 0x004CBDB8 ApeAnimScriptChangeStateCommand::~ApeAnimScriptChangeStateCommand (deleting)
ApeAnimScriptChangeStateCommand::~ApeAnimScriptChangeStateCommand() = default;

// @func 0x004CBE40 ApeAnimScriptChangeStateCommand::Create
ApeAnimScriptCommand* ApeAnimScriptChangeStateCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptChangeStateCommand(element);
}

// @func 0x004CBEBC ApeAnimScriptChangeStateCommand::Execute
void ApeAnimScriptChangeStateCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    if (!instance)
        return;
    float delay = m_delayTime;
    if (m_delayTimeVar) {
        ApeAnimScriptVar* var = instance->FindVar(m_delayTimeVar);
        if (var && var->GetType() == AnimScriptVarType::Float)
            delay = var->GetFloat();
    }
    if (state->GetTime() < delay)
        return;

    uint32_t target = m_state;
    if (m_random && !m_randomStates.empty()) {
        target = m_randomStates[g_gameRandom.seed % m_randomStates.size()];
        RandomUnit(g_gameRandom);   // advance the generator
    }
    ApeAnimScriptInstance* owner = m_childScript ? instance->FindChild(m_childScript) : instance;
    if (owner)
        owner->RequestState(target);
}

// ---------------------------------------------------------------------------
// SetFloat

// @func 0x004CA888 ApeAnimScriptSetFloatCommand::ApeAnimScriptSetFloatCommand
ApeAnimScriptSetFloatCommand::ApeAnimScriptSetFloatCommand(xml::XmlElement* element)
{
    element->GetHash(kAttrVariable, &m_variable, 0);
    element->GetHash(kAttrValueVar, &m_valueVar, m_valueVar);
    element->GetFloat(kAttrValue, &m_value, m_value);
    m_operation = ReadOperation(element);
}

// @func 0x004CA9E8 ApeAnimScriptSetFloatCommand::~ApeAnimScriptSetFloatCommand
// @func 0x004CA9F8 ApeAnimScriptSetFloatCommand::~ApeAnimScriptSetFloatCommand (deleting)
ApeAnimScriptSetFloatCommand::~ApeAnimScriptSetFloatCommand() = default;

// @func 0x004CAA3C ApeAnimScriptSetFloatCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetFloatCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptSetFloatCommand(element);
}

// @func 0x004CAAB0 ApeAnimScriptSetFloatCommand::Execute
void ApeAnimScriptSetFloatCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    ApeAnimScriptVar* var = instance->FindVar(m_variable);
    if (!var || var->GetType() != AnimScriptVarType::Float) {
        WarnName(m_variable);
        return;
    }
    auto* target = static_cast<ApeAnimScriptFloatVar*>(var);
    const float current = target->GetFloat();
    float operand = m_value;
    if (m_valueVar) {
        ApeAnimScriptVar* valueVar = instance->FindVar(m_valueVar);
        if (valueVar && valueVar->GetType() == AnimScriptVarType::Float)
            operand = valueVar->GetFloat();
    }
    switch (m_operation) {
    case AnimScriptOperation::Set: target->SetFloat(operand); break;
    case AnimScriptOperation::Add: target->SetFloat(current + operand); break;
    case AnimScriptOperation::Subtract: target->SetFloat(current - operand); break;
    case AnimScriptOperation::Multiply: target->SetFloat(current * operand); break;
    case AnimScriptOperation::Divide:
        // NOTE: only the literal operand is checked against zero.
        if (m_value != 0.0f)
            target->SetFloat(current / operand);
        break;
    }
}

// @func 0x004CAC6C ApeAnimScriptSetFloatCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetFloatCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetInt

// @func 0x004CAC7C ApeAnimScriptSetIntCommand::ApeAnimScriptSetIntCommand
ApeAnimScriptSetIntCommand::ApeAnimScriptSetIntCommand(xml::XmlElement* element)
{
    element->GetHash(kAttrVariable, &m_variable, 0);
    uint32_t hashValue = (uint32_t)m_value;
    const int found = element->GetHash(kAttrStr, &hashValue, hashValue);
    m_value = (int32_t)hashValue;
    if (found != xml::kQuery_Ok || m_value == 0)
        element->GetInt(kAttrValue, &m_value, m_value);
    m_operation = ReadOperation(element);
}

// @func 0x004CADF4 ApeAnimScriptSetIntCommand::~ApeAnimScriptSetIntCommand
// @func 0x004CAE04 ApeAnimScriptSetIntCommand::~ApeAnimScriptSetIntCommand (deleting)
ApeAnimScriptSetIntCommand::~ApeAnimScriptSetIntCommand() = default;

// @func 0x004CAE48 ApeAnimScriptSetIntCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetIntCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptSetIntCommand(element);
}

// @func 0x004CAEB8 ApeAnimScriptSetIntCommand::Execute
void ApeAnimScriptSetIntCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    ApeAnimScriptVar* var = instance->FindVar(m_variable);
    if (!var) {
        WarnName(m_variable);
        return;
    }
    if (var->GetType() == AnimScriptVarType::Int) {
        auto* target = static_cast<ApeAnimScriptIntVar*>(var);
        const int32_t current = target->GetInt();
        switch (m_operation) {
        case AnimScriptOperation::Set: target->SetInt(m_value); break;
        case AnimScriptOperation::Add: target->SetInt(current + m_value); break;
        case AnimScriptOperation::Subtract: target->SetInt(current - m_value); break;
        case AnimScriptOperation::Multiply: target->SetInt(current * m_value); break;
        case AnimScriptOperation::Divide:
            if (m_value != 0)
                target->SetInt(current / m_value);
            break;
        }
        return;
    }
    if (var->GetType() == AnimScriptVarType::Hash) {
        // Names can only be assigned.
        if (m_operation == AnimScriptOperation::Set)
            static_cast<ApeAnimScriptHashVar*>(var)->SetHash((uint32_t)m_value);
        else
            WarnName(m_variable);
        return;
    }
    WarnName(m_variable);
}

// @func 0x004CB080 ApeAnimScriptSetIntCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetIntCommand::CreateDebugElement() const
{
    return nullptr;
}

} // namespace ape
