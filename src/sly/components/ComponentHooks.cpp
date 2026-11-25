// openTIT - Component hooks (stubs).
#include "sly/components/ComponentHooks.h"

#include "ape/anim/ApeAnimClip.h"
#include "sly/anim/AnimController.h"

#include "sly/entity/Entity.h"

#include "sly/levels/ai/AIBossGrizz.h"

namespace sly {
namespace components {

// TODO(0x00F65CA0 game world +0x1281D4): the level physics world is not ported.
void* LevelPhysicsWorld() { return nullptr; }


// TODO(0x000250A0): the AI brain lives in sly/levels/ai.
void AIBrainFillInput(void* brain, void* input) { (void)brain; (void)input; }
void* FindAnimTree(uint32_t nameHash) { return anim::FindAnimTree(nameHash); }
ape::anim::AnimClipManager* GameAnimClipManager() { return ape::anim::g_animClipManager; }
void AnimControllerBuildLayers(anim::AnimController* controller, void* tree)
{
    controller->BuildLayers(static_cast<const anim::AnimTree*>(tree));
}
void* FindAnimBlendMap(uint32_t nameHash) { return anim::FindAnimBlendMap(nameHash); }
// TODO(0x004A59B4 geometry material lookup)
agl::AglMaterial* GeomFindMaterial(ape::world::ApeObj*, uint32_t) { return nullptr; }
// TODO(0x0046B5F8 game world callback lists)
void QueueWorldCallback(int, void*, void*, entity::Entity*) {}
void DequeueWorldCallback(int, void*, void*, entity::Entity*) {}

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

// TODO(slot 24 / gadget +0xF4)
ape::ApeAnimScriptInstance* EntityStartScript(entity::Entity* entity, ape::ApeAnimScript* script)
{
    return entity ? entity->StartScript(script, 0, true) : nullptr;
}
void EntitySetBusy(entity::Entity*, bool) {}
// NOTE: world objects and script object references are separate classes in the port.
ape::ApeRefCounted* EntityScriptObject(entity::Entity*) { return nullptr; }
uint32_t GadgetMode(entity::EntityComponent*) { return 0; }

// TODO(0x0048FA54 / 0x0048F298 listener groups, render object +0x2C)
void ListenerJoinGroup(void*, uint32_t) {}
void ListenerLeaveGroups(void*) {}
void EntitySetRenderLayer(entity::Entity*, uint32_t) {}

// TODO(0x005801A0 / 0x005A05E8 / ApeLocatorSet::Find): geometry instancing.
ape::world::ApeObj* MarkerGeometry(uint32_t) { return nullptr; }
void* EntityFindLocator(entity::Entity*, uint32_t) { return nullptr; }

// The parser lives with the AI component (sly/levels/ai/AIBossGrizz.cpp).
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                       uint8_t scope)
{
    return levels::ai::ParseBossGrizzComponentDesc(root, elem, scope);
}

// TODO(0x00250F40 / 0x000EDE30): weapon muzzle and projectile spawning live
// with the skin component and the entity module.
void SkinWeaponMuzzle(entity::EntityComponent*, ape::Matrix34&, uint32_t) {}
uint32_t SkinActiveWeaponSlot(entity::EntityComponent*) { return 0; }
entity::Entity* SpawnWeaponProjectile(float, entity::EntityDef*, entity::Entity*, const ape::Matrix34&,
                                      const ape::Vec3&, const ape::Matrix34&)
{
    return nullptr;
}
// TODO(0x00F8F154): the level's point instance list is not ported.
void* FindPointInstance(uint32_t) { return nullptr; }
const ape::Matrix34& PointInstanceTransform(void*)
{
    static const ape::Matrix34 kIdentity{};
    return kIdentity;
}
// TODO(0x001CF620 / 0x001CF690): turret entity aiming (other range).
void TurretAimAt(entity::Entity*, const ape::Vec3&) {}
bool TurretIsAimed(entity::Entity*) { return true; }

} // namespace components
} // namespace sly
