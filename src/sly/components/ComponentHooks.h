// openTIT - Calls out of the component code into systems that are not
// ported yet. Each is a TODO stub until its owner exists.
#pragma once

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }
namespace sly { namespace entity { struct ComponentDesc; class Entity; class EntityComponent; } }
namespace ape { class ApeAnimScript; class ApeAnimScriptInstance; }

namespace sly {
namespace components {

// Physics world of the running level (game world 0x00F65CA0 +0x1281D4).
void* LevelPhysicsWorld();

// AI brain of a ControlAI component fills the control input (0x000250A0).
void AIBrainFillInput(void* brain, uint8_t* input);
// Anim trees and blend maps by name (0x003482B0 / 0x00348650).
void* FindAnimTree(uint32_t nameHash);
void* FindAnimBlendMap(uint32_t nameHash);
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
// Entity virtual slot 24 (+0x60): NOTE: busy / engaged flag (base 0x007377D8).
void EntitySetBusy(entity::Entity* entity, bool busy);
// Gadget component (type 0x1A) word +0xF4 bits 20-27: the gadget mode.
uint32_t GadgetMode(entity::EntityComponent* gadget);

// <BossGrizz> component parser (forwards to levels::ai::ParseBossGrizzComponentDesc).
entity::ComponentDesc* ParseBossGrizzComponentDescHook(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                                       uint8_t scope);

} // namespace components
} // namespace sly
