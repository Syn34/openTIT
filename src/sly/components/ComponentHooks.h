// openTIT - Calls out of the component code into systems that are not
// ported yet. Each is a TODO stub until its owner exists.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }
namespace sly { namespace entity { struct ComponentDesc; class Entity; class EntityComponent; class EntityDef; } }
namespace ape::world { class ApeObj; }
namespace ape::anim { class AnimClipManager; }
namespace agl { class AglMaterial; }
namespace sly { namespace anim { class AnimController; } }
namespace sly { class GameEventListener; }
namespace ape { class ApeAnimScript; class ApeAnimScriptInstance; class ApeRefCounted; }

namespace sly {
namespace components {

// Physics world of the running level (game world 0x00F65CA0 +0x1281D4).
void* LevelPhysicsWorld();

// AI brain of a ControlAI component fills the control input (0x000250A0).
void AIBrainFillInput(void* brain, void* input);
// Anim trees and blend maps by name (0x003482B0 / 0x00348650).
void* FindAnimTree(uint32_t nameHash);
void* FindAnimBlendMap(uint32_t nameHash);
// Anim set slots of the game (the AnimClipManager reached through
// 0x00F557F8). NOTE: the port hands out ape::anim::g_animClipManager.
ape::anim::AnimClipManager* GameAnimClipManager();
// 0x00351A40: creates a controller's layers from an anim tree.
void AnimControllerBuildLayers(anim::AnimController* controller, void* tree);
// 0x004A59B4: material named `name` of a geometry instance (null if none).
agl::AglMaterial* GeomFindMaterial(ape::world::ApeObj* geom, uint32_t name);
// 0x0046B5F8 on the game world: `callback(owner, context)` runs on world
// callback list `list` later this frame.
void QueueWorldCallback(int list, void* callback, void* context, entity::Entity* owner);
// 0x0046B7A4 on the game world: takes `callback(owner, context)` off list `list`.
void DequeueWorldCallback(int list, void* callback, void* context, entity::Entity* owner);
// IK component update (0x002934E0) and vortex pull (0x002B0B10) are not ported.

// -- HUD markers ---------------------------------------------------------------------
// 0x00367900: instance of the HUD template `templateName` registered as
// `name`; null when the template is missing.
void* HudCloneObject(uint32_t templateName, uint32_t name);
// HudObject state (+0xA0 >> 1): 0 while idle.
uint32_t HudObjectAnimState(void* hudObject);
// 0x00362BEC: hides the element at once; 0x0036325C: hide/show request;
// slot 5: (de)activates it.
void HudObjectHideNow(void* hudObject);
void HudObjectShow(void* hudObject, bool a, bool intro, bool b);
void HudObjectSetActive(void* hudObject, bool active);
// Colour override of a marker (0x00257910): with `enable` the element's
// highlight material is copied into the record (made on first use) and the
// record is linked to the element; without, the original is restored.
struct MarkerColorOverride {
    void* hudObject = nullptr;   // +0x00 element whose material is overridden (null: none)
    void* material = nullptr;    // +0x04 private copy of the material
};
void MarkerSetColorOverride(void* hudObject, MarkerColorOverride& record, bool enable);
// 0x0046B7A4 on the game world (list 6, callback 0x00F357E0): forgets a
// colour override record; the copied material is released by slot 2.
void GameWorldForgetColorOverride(MarkerColorOverride& record);
void MaterialRelease(void* material);

// -- entities ------------------------------------------------------------------------
// 0x000DCA20: runs `script` as a child of the entity's own anim script.
ape::ApeAnimScriptInstance* EntityStartScript(entity::Entity* entity, ape::ApeAnimScript* script);
// The entity's world object (+0xA0) as an anim script object reference.
ape::ApeRefCounted* EntityScriptObject(entity::Entity* entity);
// Entity virtual slot 24 (+0x60): NOTE: busy / engaged flag (base 0x007377D8).
void EntitySetBusy(entity::Entity* entity, bool busy);
// Gadget component (type 0x1A) word +0xF4 bits 20-27: the gadget mode.
uint32_t GadgetMode(entity::EntityComponent* gadget);

// -- event listener groups -------------------------------------------------------------
// Game-side listener object embedded in components (base 0x0048F27C,
// destructor 0x0048F6B0): members of a listener group receive the messages
// posted to it (0x00470660). 0x0048FA54 joins a group, 0x0048F298 leaves all.
void ListenerJoinGroup(GameEventListener* listener, uint32_t group);
// What a listener receives (sender entity, group key, message, argument).
struct ListenerMessage {
    entity::Entity* sender = nullptr;   // +0x00
    uint32_t key = 0;                   // +0x04
    uint32_t message = 0;               // +0x08
    uint32_t argument = 0;              // +0x0C
};
void ListenerLeaveGroups(GameEventListener* listener);
// Entity world object (slot 36) word +0x2C: render layer.
void EntitySetRenderLayer(entity::Entity* entity, uint32_t layer);

// -- marker geometry ---------------------------------------------------------------
// The geometry a contextual marker shows: the placed instance of that name
// (made visible and asked for its type-7 object, virtual slot 14), else a new
// instance of the geometry definition (0x005A05E8); null when neither exists.
ape::world::ApeObj* MarkerGeometry(uint32_t geomName);
// Locator of the owner's world object definition (+0xD0 +0x6C) by name
// (ApeLocatorSet::Find), or null.
void* EntityFindLocator(entity::Entity* entity, uint32_t name);

// -- projectiles -----------------------------------------------------------------------
// Muzzle transform of the owner's active weapon (skin component, weapon slot
// `slot`): 0x00250F40 fills the matrix (starting from `muzzle`).
void SkinWeaponMuzzle(entity::EntityComponent* skin, ape::Matrix34& muzzle, uint32_t slot);
// Skin component +0x4F0: index of the active weapon slot.
uint32_t SkinActiveWeaponSlot(entity::EntityComponent* skin);
// 0x000EDE30: spawns a projectile of `def` (a projectile definition, def
// flags 0x2000) fired by `owner` at `transform` with `velocity` towards
// `target` ("EntityCompWeapon_Proj_%i"); null if nothing was spawned.
entity::Entity* SpawnWeaponProjectile(float speed, entity::EntityDef* def, entity::Entity* owner,
                                      const ape::Matrix34& transform, const ape::Vec3& velocity,
                                      const ape::Matrix34& target);
// Point instance of the loaded level by name (instance list 0x00F8F154);
// null if there is none. Its world transform is at +0x50.
void* FindPointInstance(uint32_t name);
const ape::Matrix34& PointInstanceTransform(void* point);
// Turret-like owners (type bit 57) aim at a point before firing: 0x001CF620
// sets the wanted direction, 0x001CF690 tells whether the aim has reached it.
void TurretAimAt(entity::Entity* turret, const ape::Vec3& point);
bool TurretIsAimed(entity::Entity* turret);

// <BossGrizz> component parser (forwards to levels::ai::ParseBossGrizzComponentDesc).
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                       uint8_t scope);

} // namespace components
} // namespace sly
