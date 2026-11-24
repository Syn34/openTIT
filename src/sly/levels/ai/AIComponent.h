// openTIT - AI component (component type 0x0D): owns the AIController of an
// entity and forwards the component callbacks to it.
//
// The XML component description ("type" picks the behaviour, a second
// attribute names the AI settings object) builds the controller class:
// "Critter", "Player" and nine more behaviours known only by their type
// hashes; anything else gets the standard enemy behaviour.
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/entity/Entity.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace levels {
namespace ai {

class AIController;
class PropertyBlock;

// Behaviour type hashes ("type" attribute of the component description).
enum AIType : uint32_t {
    kAIType_Critter        = 0x685B0BE7u,   // "Critter"
    kAIType_Player         = 0x8CD18EE1u,   // "Player"
    kAIType_PatternBoss    = 0x49CC0E00u,   // reads the "Pattern" lists below
    kAIType_Cpu            = 0xCCFB1F40u,
    kAIType_C83BCA3D       = 0xC83BCA3Du,
    kAIType_BC6EBE7B       = 0xBC6EBE7Bu,
    kAIType_ElJefe         = 0xA1519173u,
    kAIType_MsDecibel      = 0x0EEBEFAAu,
    kAIType_4908BDCF       = 0x4908BDCFu,
    kAIType_7B70E717       = 0x7B70E717u,
};

// Description of an AI component (vtable 0x00B0DDAC, 0x1C bytes).
struct AIComponentDesc : entity::ComponentDesc {
    explicit AIComponentDesc(uint32_t nameHash) : entity::ComponentDesc(nameHash, entity::kComp_0D) {}
    uint32_t settingsName = 0;   // +0x14 AI settings object (attribute 0x3A034393)
    uint32_t aiType = 0;         // +0x18 AIType ("type")
};

// "<component type=...>" parser (0x00017420).
entity::ComponentDesc* ParseAIComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                            uint8_t scope);

// State patterns of the pattern boss: one per "Pattern" element (keyed by
// its "level" attribute), each a run of state names in `states`.
struct AIStatePatterns {
    // NOTE: initial capacities are not known.
    AIStatePatterns() : levels(8), first(8), count(8), states(32) {}
    ape::ApeTArray<uint32_t> levels;   // 0x00F9BC78
    ape::ApeTArray<uint32_t> first;    // 0x00F9BC90 index of the pattern's first state
    ape::ApeTArray<uint32_t> count;    // 0x00F9BCA8 number of states
    ape::ApeTArray<uint32_t> states;   // 0x00F9BCC0 state name hashes
};
AIStatePatterns& PatternBossPatterns();
// Replaces the patterns with the "Pattern" children of `elem` (0x00017124).
void ParseAIStatePatterns(ape::xml::XmlElement* elem);

class AIComponent : public entity::EntityComponent {
public:
    // Builds the behaviour named by the description (0x000179C0).
    explicit AIComponent(const AIComponentDesc* desc);
    ~AIComponent() override;

    void OnOwnerSpawned(entity::Entity* owner) override;
    void Update(float dt) override;
    void PostUpdate(float dt) override;
    // Turns the AI on or off.
    void SetOption(bool value) override;

    AIController* GetController() const { return controller; }
    const PropertyBlock* GetSettings() const { return settings; }

    AIController* controller = nullptr;      // +0x18
    const PropertyBlock* settings = nullptr; // +0x1C AI settings object
};

// Builds the component (0x00017F64, the factory entry for type 0x0D).
AIComponent* CreateAIComponent(AIComponent* storage, const AIComponentDesc* desc);

} // namespace ai
} // namespace levels
} // namespace sly
