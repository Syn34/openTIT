// openTIT - <Combat> component (0x0025FE94, 0x00274AC0-0x00274C94).
#include "sly/components/CombatComponent.h"

#include "ape/anim/ApeAnimScript.h"
#include "sly/game/AudioSystem.h"

namespace sly {
namespace components {

// @func 0x0025FE94 CombatComponent::CombatComponent
// Names that are given but not registered only print a warning ("Combat
// Attack def '%s' not found." / "Combat Reaction def '%s' not found.").
CombatComponent::CombatComponent(const CombatComponentDesc* desc)
    : EntityComponent(desc)
    , value44(desc->value2C)
{
    for (int i = 0; i < kAttackCount; ++i)
        attacks[i] = *CombatAttacks().Find(desc->hashes[i]);
    auto& descReactions = const_cast<ape::ApeTSortedMap<uint32_t>&>(desc->reactions);
    for (uint16_t i = 0; i < descReactions.Count(); ++i) {
        const auto& entry = descReactions.Data()[i];
        reactions.Add(entry.key, *CombatReactions().Find(entry.value));
    }
}

// @func 0x00274AC0 CombatComponent::~CombatComponent
// @func 0x00274C94 CombatComponent::~CombatComponent (deleting)
CombatComponent::~CombatComponent()
{
    if (script) {
        script->SetRemoveWhenFinished(true);
        script = nullptr;
    }
    audio::StopVoice(voice, true);
}

} // namespace components
} // namespace sly
