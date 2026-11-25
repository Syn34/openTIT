// openTIT - Ape engine anim scripts: running script instances.
#include "ape/anim/ApeAnimScript.h"

#include "ape/anim/ApeAnimScriptVars.h"

namespace ape {

namespace {
constexpr uint32_t kStartState = AnimHash("Start");
// Step used when a state change is applied immediately.
constexpr float kImmediateUpdateStep = 0.0167f;
} // namespace

bool ApeAnimScriptInstance::s_paused = false;   // 0x010C2958

// @func 0x004C7A68 ApeAnimScriptInstance::ApeAnimScriptInstance
ApeAnimScriptInstance::ApeAnimScriptInstance(ApeAnimScript* script) : m_script(script)
{
    if (script)
        script->AddRef();
    Init();
}

// @func 0x004C761C ApeAnimScriptInstance::Init
void ApeAnimScriptInstance::Init()
{
    if (!m_script)
        return;

    // Own copies of the script's variables, with object references bound to
    // the world objects of the same name.
    if (AnimScriptVarMap* vars = m_script->GetVars()) {
        for (ApeAnimScriptVar* var = vars->First(); var; var = vars->Next()) {
            ApeAnimScriptVar* copy = var->Clone();
            if (!copy)
                continue;
            m_vars.Add(copy->GetNameHash(), copy);
            if (copy->GetType() == AnimScriptVarType::Object) {
                auto* objectVar = static_cast<ApeAnimScriptObjectVar*>(copy);
                if (objectVar->GetObjectName() != 0)
                    objectVar->SetObject(ResolveObject(objectVar->GetObjectName(), true));
            }
        }
    }

    if (ApeAnimScriptState* start = m_script->FindState(kStartState))
        m_state = start->CloneForInstance(this);
    m_finished = false;
}

// @func 0x004C8E80 ApeAnimScriptInstance::~ApeAnimScriptInstance
ApeAnimScriptInstance::~ApeAnimScriptInstance()
{
    for (ApeAnimScriptInstance* child : m_children)
        delete child;
    m_children.clear();

    m_vars.ForEach([](ApeAnimScriptVar* var) { var->OnOwnerDestroyed(); });
    m_vars.ForEach([](ApeAnimScriptVar* var) { var->Release(); });
    m_vars.Clear();

    if (m_script)
        m_script->Release();
    if (m_state)
        m_state->Release();
}

// @func 0x004BB4B4 ApeAnimScriptInstance::FindVar
ApeAnimScriptVar* ApeAnimScriptInstance::FindVar(uint32_t nameHash)
{
    // Without a state only the instance's own variables are visible.
    if (m_state) {
        if (AnimScriptVarMap* stateVars = m_state->GetVars())
            if (ApeAnimScriptVar* var = stateVars->Find(nameHash))
                return var;
        if (ApeAnimScriptVar* var = GlobalVars().Find(nameHash))
            return var;
    }
    return m_vars.Find(nameHash);
}

ApeRefCounted* ApeAnimScriptInstance::FindWorldObject(uint32_t nameHash)
{
    AnimScriptHooks& hooks = GetAnimScriptHooks();
    return hooks.findWorldObject ? hooks.findWorldObject(nameHash) : nullptr;
}

// @func 0x004BB7E0 ApeAnimScriptInstance::ResolveObject
ApeRefCounted* ApeAnimScriptInstance::ResolveObject(uint32_t nameHash, bool fromWorld)
{
    if (fromWorld)
        return FindWorldObject(nameHash);

    ApeAnimScriptVar* var = FindVar(nameHash);
    ApeAnimScriptObjectVar* local = nullptr;
    if (var && var->GetType() == AnimScriptVarType::Object) {
        local = static_cast<ApeAnimScriptObjectVar*>(var);
        if (local->GetObject())
            return local->GetObject();
    }

    // Fall back to a global object variable of the same name and remember
    // the object in the local variable.
    ApeRefCounted* object = nullptr;
    ApeAnimScriptVar* global = GlobalVars().Find(nameHash);
    if (global && global->GetType() == AnimScriptVarType::Object)
        object = static_cast<ApeAnimScriptObjectVar*>(global)->GetObject();
    if (object && local)
        local->SetObject(object);
    return object;
}

// @func 0x004CF2BC ApeAnimScriptInstance::GetObjectVarField24
uint32_t ApeAnimScriptInstance::GetObjectVarField24(uint32_t nameHash)
{
    ApeAnimScriptVar* var = FindVar(nameHash);
    if (!var || var->GetType() != AnimScriptVarType::Object)
        return 0;
    return static_cast<ApeAnimScriptObjectVar*>(var)->GetField24();
}

// @func 0x004CF330 ApeAnimScriptInstance::SetState
bool ApeAnimScriptInstance::SetState(ApeAnimScriptState* state)
{
    m_finished = false;
    if (!state)
        return false;
    if (m_state)
        m_state->Release();
    m_state = state->CloneForInstance(this);
    return true;
}

// @func 0x004CF3C4 ApeAnimScriptInstance::SetState(hash)
bool ApeAnimScriptInstance::SetState(uint32_t stateHash)
{
    ApeAnimScriptState* state = m_script ? m_script->FindState(stateHash) : nullptr;
    m_finished = false;
    if (!state)
        return false;
    if (m_state)
        m_state->Release();
    m_state = state->CloneForInstance(this);
    return true;
}

// @func 0x004CF8D8 ApeAnimScriptInstance::SetStateAndUpdate
bool ApeAnimScriptInstance::SetStateAndUpdate(ApeAnimScriptState* state)
{
    if (!SetState(state))
        return false;
    Update(kImmediateUpdateStep);
    return true;
}

// @func 0x004CF988 ApeAnimScriptInstance::SetStateAndUpdate(hash)
bool ApeAnimScriptInstance::SetStateAndUpdate(uint32_t stateHash)
{
    if (!SetState(stateHash))
        return false;
    Update(kImmediateUpdateStep);
    return true;
}

// @func 0x004CF710 ApeAnimScriptInstance::Update
void ApeAnimScriptInstance::Update(float dt)
{
    if (!m_active)
        return;

    if (m_state) {
        m_state->Update(dt, this);
        // A state without conditional events ends when its clock runs out.
        const AnimScriptEventMap* events = m_state->GetEvents();
        if (m_state->GetDuration() <= m_state->GetTime() && (!events || events->Count() < 1) &&
            !m_state->IsLooping())
            m_finished = true;
    }

    for (size_t i = 0; i < m_children.size(); ++i) {
        ApeAnimScriptInstance* child = m_children[i];
        if (!child)
            continue;
        child->Update(dt);
        if (child->IsFinished(false)) {
            delete child;
            m_children[i] = m_children.back();
            m_children.pop_back();
            --i;
        }
    }
    if (!m_children.empty())
        m_finished = false;

    if (m_pendingState) {
        SetState(m_pendingState);
        m_pendingState = 0;
        m_finished = false;
    }
}

// @func 0x004CF548 ApeAnimScriptInstance::SendEvent
void ApeAnimScriptInstance::SendEvent(uint32_t eventHash)
{
    if (!m_active)
        return;
    if (m_state)
        m_state->FireEvent(this, eventHash);
    for (size_t i = 0; i < m_children.size(); ++i)
        if (m_children[i])
            m_children[i]->SendEvent(eventHash);
    if (m_pendingState) {
        SetState(m_pendingState);
        m_pendingState = 0;
    }
}

// @func 0x004CF620 ApeAnimScriptInstance::IsFinished
bool ApeAnimScriptInstance::IsFinished(bool ignoreChildren) const
{
    if (m_keepAlive)
        return false;
    if (!ignoreChildren && (!m_script || m_script->WaitsForChildren())) {
        for (ApeAnimScriptInstance* child : m_children)
            if (!child->IsFinished(false))
                return false;
    }
    if (!m_state)
        return true;
    return !m_state->IsHolding() && m_state->IsDone();
}

void ApeAnimScriptInstance::StopChildren(uint32_t scriptHash)
{
    for (size_t i = 0; i < m_children.size(); ++i) {
        ApeAnimScriptInstance* child = m_children[i];
        if (!child)
            continue;
        const uint32_t childScript = child->m_script ? child->m_script->GetNameHash() : 0;
        if (childScript == scriptHash) {
            delete child;
            m_children[i] = m_children.back();
            m_children.pop_back();
            --i;
        }
    }
}

void ApeAnimScriptInstance::StopAllChildren()
{
    for (ApeAnimScriptInstance* child : m_children)
        delete child;
    m_children.clear();
}

ApeAnimScriptInstance* ApeAnimScriptInstance::FindChild(uint32_t scriptHash) const
{
    for (ApeAnimScriptInstance* child : m_children) {
        if (!child)
            continue;
        const uint32_t childScript = child->m_script ? child->m_script->GetNameHash() : 0;
        if (childScript == scriptHash)
            return child;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Global instance list (0x010C2968)

std::vector<ApeAnimScriptInstance*>& ApeAnimScriptInstance::All()
{
    static std::vector<ApeAnimScriptInstance*>* s_instances = new std::vector<ApeAnimScriptInstance*>();
    return *s_instances;
}

// @func 0x004C753C ApeAnimScriptInstance::Register
void ApeAnimScriptInstance::Register(ApeAnimScriptInstance* instance)
{
    All().push_back(instance);
}

// @func 0x004CFB24 ApeAnimScriptInstance::Create
ApeAnimScriptInstance* ApeAnimScriptInstance::Create(uint32_t scriptHash)
{
    ApeAnimScript* script = ApeAnimScriptLibrary::Get(0)->FindScript(scriptHash);
    if (!script)
        return nullptr;
    auto* instance = new ApeAnimScriptInstance(script);
    Register(instance);
    return instance;
}

// @func 0x004CFD1C ApeAnimScriptInstance::UpdateAll
void ApeAnimScriptInstance::UpdateAll(float dt)
{
    if (s_paused)
        return;
    std::vector<ApeAnimScriptInstance*>& instances = All();
    for (size_t i = 0; i < instances.size(); ++i) {
        ApeAnimScriptInstance* instance = instances[i];
        if (instance->m_active && !instance->m_finished)
            instance->Update(dt);
        // NOTE: finished instances are only dropped from the list; their
        // owner deletes them.
        if (instance->m_removeWhenFinished && instance->IsFinished(false)) {
            instances[i] = instances.back();
            instances.pop_back();
            --i;
        }
    }
}

// @func 0x004D00CC ApeAnimScriptInstance::DestroyAllInstances
void ApeAnimScriptInstance::DestroyAllInstances()
{
    if (s_paused)
        return;
    std::vector<ApeAnimScriptInstance*>& instances = All();
    while (!instances.empty()) {
        delete instances.back();
        instances.pop_back();
    }
}

// @func 0x004D0280 ApeAnimScriptInstance::DestroyInstance
void ApeAnimScriptInstance::DestroyInstance(ApeAnimScriptInstance* instance)
{
    std::vector<ApeAnimScriptInstance*>& instances = All();
    for (size_t i = 0; i < instances.size(); ++i) {
        if (instances[i] != instance)
            continue;
        // Swap with the last entry, delete, drop the last entry.
        instances[i] = instances.back();
        instances.back() = instance;
        delete instance;
        instances.pop_back();
        return;
    }
}

} // namespace ape
