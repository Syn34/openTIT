// openTIT - <DamageSequence> component (0x002506A8, 0x002D31B8-0x002D4190).
#include "sly/components/DamageSequence.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"

namespace sly {
namespace components {

namespace {
constexpr uint32_t kVar_This = 0x0027469Eu;      // "this"
constexpr uint32_t kMsg_Damaged = 0x69988172u;   // posted by ScriptedEntity::Damage
constexpr float kFirstUpdate = 0.0177f;          // NOTE: 0x3F9119CE as float
} // namespace

// @func 0x002506A8 DamageSequenceComponent::DamageSequenceComponent
// The script's "this" variable refers to the owner's world object (with the
// animation controller in its extra word).
DamageSequenceComponent::DamageSequenceComponent(const DamageSequenceComponentDesc* desc)
    : EntityComponent(desc)
    , stepCount(desc->int18)
    , initTime(desc->initTime)
    , enabledTime(desc->enabledTime)
    , disabledTime(desc->disabledTime)
    , flag29(desc->flag29)
{
    listener.outer = this;
    ape::ApeAnimScriptLibrary* library = desc->script ? ape::ApeAnimScriptLibrary::Get(0) : nullptr;
    ape::ApeAnimScript* source = library ? library->FindScript(desc->script) : nullptr;
    if (source) {
        if (!owner) {
            script = new ape::ApeAnimScriptInstance(source);
            ape::ApeAnimScriptInstance::Register(script);
        } else {
            script = EntityStartScript(owner, source);
        }
        if (script) {
            ape::ApeAnimScriptVar* self = script->FindVar(kVar_This);
            if (self && self->GetType() == ape::AnimScriptVarType::Object && owner) {
                auto* objectVar = static_cast<ape::ApeAnimScriptObjectVar*>(self);
                // NOTE: the world object is reached as an anim script object
                // reference through the entity hook.
                objectVar->SetObject(EntityScriptObject(owner));
                objectVar->SetField24(0);   // NOTE: the console stores the animation controller pointer
            }
            script->SetRemoveWhenFinished(false);
            script->Update(kFirstUpdate);
        }
    }
    enabled = desc->startEnabled;
}

// @func 0x002D31B8 DamageSequenceComponent::~DamageSequenceComponent
// @func 0x002D333C DamageSequenceComponent::~DamageSequenceComponent (deleting)
// @func 0x002D33D0 DamageSequenceComponent::Listener::~Listener (thunk)
// @func 0x002D3384 DamageSequenceComponent::Listener::~Listener (deleting thunk)
DamageSequenceComponent::~DamageSequenceComponent()
{
    if (script) {
        script->SetRemoveWhenFinished(true);
        script = nullptr;
    }
}

// @func 0x0025069C DamageSequenceComponent::SetOption
void DamageSequenceComponent::SetOption(bool on)
{
    enabled = on;
}

// @func 0x002D4030 DamageSequenceComponent::OnListenerMessage
// @func 0x002D4190 DamageSequenceComponent::Listener::OnMessage (thunk)
void DamageSequenceComponent::OnListenerMessage(const ListenerMessage& msg)
{
    if (msg.message != kMsg_Damaged || failed)
        return;
    if (nextStep < stepCount && nextStep < (int32_t)steps.Count() && msg.sender == steps[(size_t)nextStep].part) {
        ++nextStep;
        if (nextStep == stepCount)
            completed = true;
        return;
    }
    if (phase == 2 || !owner || !owner->children)
        return;
    for (const entity::EntityChildLink& link : *owner->children) {
        if (link.child == msg.sender) {
            completed = false;
            failed = true;
            return;
        }
    }
}

} // namespace components
} // namespace sly
