// openTIT - Component hooks (stubs).
#include "sly/components/ComponentHooks.h"

#include "sly/levels/ai/AIBossGrizz.h"

namespace sly {
namespace components {

// TODO(0x00F65CA0 game world +0x1281D4): the level physics world is not ported.
void* LevelPhysicsWorld() { return nullptr; }


// TODO(0x000250A0): the AI brain lives in sly/levels/ai.
void AIBrainFillInput(void* brain, uint8_t* input) { (void)brain; (void)input; }
// TODO(0x003482B0 / 0x00348650): anim tree and blend map registries.
void* FindAnimTree(uint32_t nameHash) { (void)nameHash; return nullptr; }
void* FindAnimBlendMap(uint32_t nameHash) { (void)nameHash; return nullptr; }

// The parser lives with the AI component (sly/levels/ai/AIBossGrizz.cpp).
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                       uint8_t scope)
{
    return levels::ai::ParseBossGrizzComponentDesc(root, elem, scope);
}

} // namespace components
} // namespace sly
