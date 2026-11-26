// openTIT - <SystemCracker> component (0x002562C8, 0x002B17EC-0x002B1CE0).
#include "sly/components/SystemCracker.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "sly/components/ComponentHooks.h"

namespace sly {
namespace components {

namespace {
constexpr uint32_t kGroup_SystemCracker = 0x0C8B8AB8u;   // "SystemCracker"
constexpr uint32_t kVar_Transform = 0x001E8ECCu;         // NOTE: variable name unknown (matrix)
constexpr uint32_t kRenderLayer_OnTop = 9;               // NOTE: layer meaning assumed

bool HasState(ape::ApeAnimScriptInstance* instance, uint32_t state)
{
    ape::ApeAnimScript* source = instance ? instance->GetScript() : nullptr;
    return source && source->FindState(state) != nullptr;
}
} // namespace

// @func 0x002562C8 SystemCrackerComponent::SystemCrackerComponent
SystemCrackerComponent::SystemCrackerComponent(const SystemCrackerComponentDesc* desc)
    : EntityComponent(desc)
    , scriptName(desc->script)
    , renderOnTop(desc->flag18)
{
    listener.outer = this;
    ListenerJoinGroup(&listener, kGroup_SystemCracker);

    ape::ApeAnimScriptLibrary* library = scriptName ? ape::ApeAnimScriptLibrary::Get(0) : nullptr;
    ape::ApeAnimScript* source = library ? library->FindScript(scriptName) : nullptr;
    if (source) {
        script = EntityStartScript(owner, source);
        if (script) {
            script->SetRemoveWhenFinished(false);
            ape::ApeAnimScriptVar* var = script->FindVar(kVar_Transform);
            if (var && var->GetType() == ape::AnimScriptVarType::Matrix && owner)
                static_cast<ape::ApeAnimScriptMatrixVar*>(var)->SetMatrix(owner->transform);
        }
    }
    if (renderOnTop && owner)
        EntitySetRenderLayer(owner, kRenderLayer_OnTop);
}

// @func 0x002B17EC SystemCrackerComponent::~SystemCrackerComponent
// @func 0x002B1954 SystemCrackerComponent::~SystemCrackerComponent (deleting)
// @func 0x002B19E8 SystemCrackerComponent::Listener::~Listener (thunk)
// @func 0x002B199C SystemCrackerComponent::Listener::~Listener (deleting thunk)
SystemCrackerComponent::~SystemCrackerComponent()
{
    ListenerLeaveGroups(&listener);
    if (script) {
        script->SetRemoveWhenFinished(true);
        script = nullptr;
    }
}

// @func 0x002B1A10 SystemCrackerComponent::Update
void SystemCrackerComponent::Update(float dt)
{
    (void)dt;
}

void SystemCrackerComponent::Listener::OnGameMessage(const entity::EntityMessage& msg)
{
    ListenerMessage message;
    message.sender = msg.sender;
    message.message = msg.id;
    message.argument = msg.argument;
    outer->OnListenerMessage(message);
}

// @func 0x002B1A14 SystemCrackerComponent::OnListenerMessage
// @func 0x002B1CE0 SystemCrackerComponent::Listener::OnMessage (thunk)
void SystemCrackerComponent::OnListenerMessage(const ListenerMessage& msg)
{
    const uint32_t message = msg.message;
    if (HasState(script, message)) {
        script->SetStateAndUpdate(message);
        return;
    }
    auto* ownerScript = owner ? static_cast<ape::ApeAnimScriptInstance*>(owner->GetAnimScript()) : nullptr;
    if (HasState(ownerScript, message))
        ownerScript->SetStateAndUpdate(message);
}

} // namespace components
} // namespace sly
