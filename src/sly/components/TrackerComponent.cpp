// openTIT - <tracker> component (0x00252DFC-0x00253390, 0x002CBD94-0x002CC018).
#include "sly/components/TrackerComponent.h"

#include "ape/anim/ApeAnimScript.h"
#include "sly/components/ComponentHooks.h"

namespace sly {
namespace components {

namespace {
constexpr uint32_t kVar_Value = 0x4DE86539u;          // NOTE: variable name unknown
constexpr float kDegToRad = 0.017453292f;
constexpr float kFirstUpdate = 0.0177f;               // NOTE: 0x3F9119CE as float
// Script states by mode: enabling / disabling. NOTE: names unknown.
constexpr uint32_t kEnableStates[3] = { 0xA6CB0022u, 0xC0FD2685u, 0x12252850u };
constexpr uint32_t kDisableStates[3] = { 0xF6239C09u, 0x1A67FE7Eu, 0xF96B3877u };
} // namespace

// @func 0x00252FF0 TrackerComponent::TrackerComponent
// The script comes from the default library; it runs under the owner's anim
// script when there is an owner, otherwise as a free instance.
TrackerComponent::TrackerComponent(const TrackerComponentDesc* desc)
    : EntityComponent(desc)
    , minRange(desc->minRange)
    , maxRange(desc->maxRange)
    , minAngle(desc->minAngle * kDegToRad)
    , maxAngle(desc->maxAngle * kDegToRad)
{
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
            ape::ApeAnimScriptVar* var = script->FindVar(kVar_Value);
            if (var && var->GetType() == ape::AnimScriptVarType::Float)
                valueVar = var;
            script->SetRemoveWhenFinished(false);
            script->Update(kFirstUpdate);
        }
    }
    SetOption(desc->startEnabled);
}

// @func 0x002CBD94 TrackerComponent::~TrackerComponent
// @func 0x002CBEAC TrackerComponent::~TrackerComponent (deleting)
TrackerComponent::~TrackerComponent()
{
    if (script) {
        script->SetRemoveWhenFinished(true);
        script = nullptr;
    }
}

// @func 0x00252DFC TrackerComponent::SetOption
// Enabling picks the mode from the player's gadget (binocucom modes 6-9,
// any other gadget mode, or none); the script enters the state of the mode
// and the owner is marked busy while enabled.
void TrackerComponent::SetOption(bool on)
{
    if (on) {
        mode = kMode_None;
        if (player) {
            EntityComponent* gadget = entity::EntityGadget(player);
            if (gadget && gadget->enabled) {
                const uint32_t gadgetMode = GadgetMode(gadget);
                if (gadgetMode >= 6 && gadgetMode <= 9)
                    mode = kMode_Binocucom;
                else if (gadgetMode != 0)
                    mode = kMode_Gadget;
            }
        }
    }
    if (script && mode >= 0 && mode <= 2)
        script->SetStateAndUpdate(on ? kEnableStates[mode] : kDisableStates[mode]);
    EntitySetBusy(owner, on);
    enabled = on;
}

} // namespace components
} // namespace sly
