// openTIT - <Combat> component (0x0025FE94, 0x00274AC0-0x00274C94).
#include "sly/components/CombatComponent.h"

#include "ape/anim/ApeAnimScript.h"
#include "sly/entity/CharacterHooks.h"
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

// @func 0x00276978 CombatComponent::AttackSlot
uint8_t CombatComponent::AttackSlot(uint32_t kind) const
{
    static const uint8_t kSlots[8] = {0xFF, 0, 1, 0, 2, 0, 1, 0};
    static const uint8_t kMode2Slots[8] = {0xFF, 3, 4, 3, 5, 3, 4, 3};
    constexpr uint32_t kCharacterTypeBit = 0x8u;
    constexpr int kMovement_Mode2 = 2;
    if (owner && (owner->typeBits[0] & kCharacterTypeBit)) {
        entity::EntityComponent* physics = owner->GetComponent00();
        if (physics && entity::PhysicsCompMovementMode(physics) == kMovement_Mode2)
            return kMode2Slots[kind];
    }
    return kSlots[kind];
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
