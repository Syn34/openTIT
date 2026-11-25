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

// TODO(0x00367900 / 0x00362BEC / 0x00257910 / 0x0046B7A4): HUD element
// instancing and the marker material override need the HUD and GUI ports.
void* HudCloneObject(uint32_t, uint32_t) { return nullptr; }
uint32_t HudObjectAnimState(void*) { return 0; }
void HudObjectHideNow(void*) {}
void HudObjectShow(void*, bool, bool, bool) {}
void HudObjectSetActive(void*, bool) {}
void MarkerSetColorOverride(void* hudObject, MarkerColorOverride& record, bool enable)
{
    record.hudObject = enable ? hudObject : nullptr;
}
void GameWorldForgetColorOverride(MarkerColorOverride&) {}
void MaterialRelease(void*) {}

// TODO(0x000DCA20 / slot 24 / gadget +0xF4)
ape::ApeAnimScriptInstance* EntityStartScript(entity::Entity*, ape::ApeAnimScript*) { return nullptr; }
void EntitySetBusy(entity::Entity*, bool) {}
// NOTE: world objects and script object references are separate classes in the port.
ape::ApeRefCounted* EntityScriptObject(entity::Entity*) { return nullptr; }
uint32_t GadgetMode(entity::EntityComponent*) { return 0; }

// TODO(0x0048FA54 / 0x0048F298 listener groups, render object +0x2C)
void ListenerJoinGroup(void*, uint32_t) {}
void ListenerLeaveGroups(void*) {}
void EntitySetRenderLayer(entity::Entity*, uint32_t) {}

// The parser lives with the AI component (sly/levels/ai/AIBossGrizz.cpp).
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                       uint8_t scope)
{
    return levels::ai::ParseBossGrizzComponentDesc(root, elem, scope);
}

} // namespace components
} // namespace sly
