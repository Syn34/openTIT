// openTIT - Calls of the game shell into modules that are not ported yet.
// Each declaration names the console function it stands for; the bodies in
// GameShellHooks.cpp are safe defaults until those modules land.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape::world { class ApeObj; }

namespace sly {
namespace entity { class Entity; class EntityComponent; }

// The binocucom gadgets (list 0x00F64A58) let the game pause: none is in
// modes 6..9 (0x002AB3E0).
bool GadgetsAllowPause();
// Pauses or resumes every binocucom gadget (0x002AB340; nested count
// 0x010592E8).
void SetGadgetsPaused(bool paused);
// The entity a travel dialog was opened for is told the player declined
// (0x002B7704).
void TravelTargetDeclined(entity::Entity* target);
// An AI controller component of type 0x1B takes the contextual marker
// object (0x002B7EA0).
void AiSetContextualMarker(entity::EntityComponent* controller, ape::world::ApeObj* marker);
// A gadget component goes back to its holster (0x002A8AE0).
void GadgetPutAway(entity::EntityComponent* gadget, bool a, bool b, bool c);
// A gadget component shows the meter variant (0x002ABB60).
void GadgetSetMeterVariant(entity::EntityComponent* gadget, bool on);
// Debug switch of the level AI object (0x00F557F0 +0x1C) that forces the
// meter variant on.
bool AiDebugMeterSwitch();
// State of a character's state machine layer (entity virtual slot 8).
int CharacterState(const entity::Entity* character, uint32_t layer);
// The game world's frame object (+0x128584) refreshes (its slot 7).
void FrameObjectRefresh(void* frameObject);
// A character is thrown (0x0012B908) or dropped (0x0012BB9C) towards a
// point.
void CharacterThrowTowards(entity::Entity* character, const ape::Vec3& target);
void CharacterDropTowards(entity::Entity* character, const ape::Vec3& target);
// Component 0x24 back to its start (0x002549F8) with its flag.
void Component24Restart(entity::EntityComponent* component, bool flag);

// HUD element 0xB7E98FE4 plays its intro (0x004902B8).
void ShowCharacterHint();
// Closes the GUI popup over the game and its event handler (0x004645AC,
// 0x00464804).
void CloseGuiPopup();
// Shows or hides the AI entities (0x0048A068).
void SetAiEntitiesVisible(bool visible);

} // namespace sly
