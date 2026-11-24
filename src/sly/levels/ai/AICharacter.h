// openTIT - AI driven characters: the character class and definition the
// level AI spawns (vtables 0x00B0DE40 and 0x00B0DE10).
#pragma once

#include "sly/entity/Character.h"

namespace sly {
namespace levels {
namespace ai {

// Character whose AI component may also be component type 0x30.
// NOTE: class name unknown.
class AICharacter : public entity::Character {
public:
    entity::EntityComponent* GetComponent0D() override;
    // Switching the character off turns its AI option on and vice versa.
    void SetActive(bool active) override;
};

// Definition that spawns AICharacter (0x550 bytes).
class AICharacterDef : public entity::CharacterDef {
public:
    ~AICharacterDef() override;
    // TODO(0x0001C984 AICharacterDef::Spawn): fills the spawn parameters
    // (0x000DFDF8) and builds an AICharacter through the character
    // constructor 0x00121BB8.
};

// Event 1 / 0 to the AI controller listening to `entity` (its slot 11).
void SignalAIControllerOn(entity::Entity* entity);
void SignalAIControllerOff(entity::Entity* entity);

} // namespace ai
} // namespace levels
} // namespace sly
