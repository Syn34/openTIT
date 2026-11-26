// openTIT - <Combat> component (type 3, vtable 0x00B1C050, 0xBC bytes): the
// owner's six attacks (looked up by name in the attack registry) and its hit
// reactions by type (reaction registry), with the combat state the
// character code drives.
#pragma once

#include "sly/components/CombatDefs.h"
#include "sly/components/ComponentDescs.h"
#include "sly/entity/Entity.h"

#include "ape/memory/ApeTArray.h"

namespace ape { class ApeAnimScriptInstance; }

namespace sly {
namespace components {

class CombatComponent : public entity::EntityComponent {
public:
    static constexpr int kAttackCount = 6;

    // 0x0025FE94
    explicit CombatComponent(const CombatComponentDesc* desc);
    ~CombatComponent() override;
    // Slot 2: the combat state block (+0x18).
    // @func 0x007630BC CombatComponent::DataBlockA
    void* DataBlockA() override { return &state; }
    // TODO(0x002766A8 CombatComponent slot 4), TODO(0x002794F0 CombatComponent::Update).

    // State word (+0x18): bits 25-29 the current mode, bits 20-24 the
    // requested mode, bit 19 a request is pending.
    uint32_t state = 0;                   // 0x18
    uint32_t modeMask = 0;                // 0x1C NOTE: modes that need a deferred switch
    uint32_t field_20 = 0;                // 0x20
    uint32_t field_24 = 0;                // 0x24
    uint32_t field_28 = 0x10000;          // 0x28
    uint32_t field_2C[5] = {};            // 0x2C
    float value44 = 1.0f;                 // 0x44 CombatComponentDesc::value2C
    void* attacks[kAttackCount] = {};     // 0x48 attack records (registry 0x0105752C)
    uint32_t field_60[4] = {};            // 0x60
    ape::ApeTSortedMap<CombatReaction*> reactions{0x10};   // 0x70 reaction type -> reaction
    uint32_t field_88[6] = {};            // 0x88
    ape::ApeTArray<uint32_t> field_A0{4}; // 0xA0 NOTE: element type unknown (4 bytes)
    ape::ApeAnimScriptInstance* script = nullptr;   // 0xB4 let go with the component
    uint32_t voice = 0;                   // 0xB8
};

} // namespace components
} // namespace sly
