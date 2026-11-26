// openTIT - Calls from the mega knight code into systems that are not ported
// yet. MegaKnightHooks.cpp holds TODO stubs with safe defaults; each
// declaration names the console function or member it stands for.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { class ApeRefCounted; }
namespace ape::world { class ApeObj; }

namespace sly {
namespace entity { class Entity; class GameCamera; }
namespace gameplay {

// Plays `clip` through the entity's character component (slot 15 +
// 0x002775A8) with a speed and a blend time.
void MKPlayClip(entity::Entity* entity, uint32_t clip, float speed, float blend, bool loop);
// 0x0029879C(component, 0, 0) on the character component: the current clip
// is done (true without a component).
bool MKClipDone(entity::Entity* entity);
// Rope entities: the player hangs on it (0x001F465C), put it back
// (0x001F4460), let go of it (0x001ED9D4).
bool MKRopeHeld(entity::Entity* rope);
void MKRopeReset(entity::Entity* rope);
void MKRopeRelease(entity::Entity* rope);
// 0x000D951C + 0x00266E6C: state of an entity's interaction component.
void MKSetInteractionState(entity::Entity* entity, int state);
// The player character (game world +0x1078, character entities only).
entity::Entity* MKPlayerCharacter();
// Targeting gadget of the player: adds a marker record (0x0029F760; null
// when full) and fills it (entity +0x08, offset +0x10, kind +0x24, type
// +0x28, flag word +0x78), or removes the record of an entity (0x0029F7A0).
void MKAddTargetMarker(entity::Entity* player, entity::Entity* target, const ape::Vec3& offset);
void MKRemoveTargetMarker(entity::Entity* player, entity::Entity* target);
// The script object of an entity (bound to the phase scripts' object
// variable 0x0027469E).
ape::ApeRefCounted* MKEntityScriptObject(entity::Entity* entity);
// The geometry the platform shows: the placed instance of that name (made
// visible and asked for its type-7 object, virtual slot 14), else a new
// instance of the geometry definition (0x005A05E8).
ape::world::ApeObj* MKGeometryObject(uint32_t geomName);
// Reference-counted view of a world object for script object variables.
ape::ApeRefCounted* MKObjectScriptRef(ape::world::ApeObj* object);

} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
// Spline definition by name (ApeTDefList<ApeSplineDef>::Find on 0x00F8F158).
void* MKFindSplineDef(uint32_t nameHash);
// 0x0046C3B8 on the game world: queues the cut-scene sequence `name`.
void MKQueueSequence(uint32_t name);
} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
// Release of the boss's helper objects (camera controller +0x290: camera
// mode handed back to the game camera; stamp manager +0x294 0x0030B4AC;
// the platform is deleted by the boss itself).
// The game camera stops using a mode it holds (GameCamera::HasTarget 0x000FC5DC
// then 0x000FBB88 with -1).
// Player character: its +0x17C interface, slot 3 (called when it reaches a
// shoulder from a rope). NOTE: meaning assumed.
void MKPlayerBeginFinishingMove(entity::Entity* player);
// Game world +0x128214: frame time is frozen.
bool MKFrameFrozen();
// 0x0046C750 on the game world: sequence `name` is queued or running.
bool MKSequenceQueued(uint32_t name);
// The game camera entity (game world +0x1281E0).
entity::GameCamera* MKGameCamera();
// Floor ripple: entities of class bit 0x20 caught by a raised ring (0x001FB640
// with their position).
void MKRippleKnock(entity::Entity* entity);
void MKCameraRemoveMode(entity::GameCamera* camera, void* mode);
// Player targeting gadget byte +0x922 bit 3 (cleared when the boss goes).
void MKClearPlayerTargetingFlag(entity::Entity* player);
// Character component +0x24 -> words +0x34/+0x38 cleared on release.
void MKClearCharacterHooks(entity::Entity* boss);
// 0x0048F298(this, 1): stops listening to game events.
void MKStopListening(entity::Entity* boss);
} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
} // namespace gameplay
} // namespace sly
