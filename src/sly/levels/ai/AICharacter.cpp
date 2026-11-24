// openTIT - AI driven character and definition.
#include "sly/levels/ai/AICharacter.h"

#include "sly/levels/ai/AIController.h"

namespace sly {
namespace levels {
namespace ai {

// @func 0x0001CD40 AICharacter::GetComponent0D
entity::EntityComponent* AICharacter::GetComponent0D()
{
    if (entity::EntityComponent* comp = GetComponent(entity::kComp_0D))
        return comp;
    return GetComponent(0x30);
}

// @func 0x0001CB78 AICharacter::SetActive
void AICharacter::SetActive(bool active)
{
    if (entity::EntityComponent* ai = GetComponent0D())
        ai->SetOption(!active);
    entity::Character::SetActive(active);
}

// @func 0x0001BF4C AICharacterDef::~AICharacterDef
// @func 0x0001BF7C AICharacterDef::~AICharacterDef (deleting)
AICharacterDef::~AICharacterDef() = default;

// @func 0x0001CCA8 SignalAIControllerOn
void SignalAIControllerOn(entity::Entity* entity)
{
    if (auto* controller = dynamic_cast<AIController*>(entity->listener))
        controller->OnEvent(1);
}

// @func 0x0001CCF4 SignalAIControllerOff
void SignalAIControllerOff(entity::Entity* entity)
{
    if (auto* controller = dynamic_cast<AIController*>(entity->listener))
        controller->OnEvent(0);
}

} // namespace ai
} // namespace levels
} // namespace sly
