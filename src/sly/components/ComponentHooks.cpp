// openTIT - Component hooks (stubs).
#include "sly/components/ComponentHooks.h"

namespace sly {
namespace components {

// TODO(0x00F65CA0 game world +0x1281D4): the level physics world is not ported.
void* LevelPhysicsWorld() { return nullptr; }

// TODO(0x00017420 ParseAIComponentDesc): the AI component description is not ported.
entity::ComponentDesc* ParseAIComponentDescHook(ape::xml::XmlElement*, ape::xml::XmlElement*, uint8_t) { return nullptr; }

// TODO(0x00063D54 ParseBossGrizzComponentDesc): not ported yet.
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement*, ape::xml::XmlElement*, uint8_t)
{
    return nullptr;
}

} // namespace components
} // namespace sly
