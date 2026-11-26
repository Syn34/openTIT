// openTIT - Calls from the mega knight code into systems that are not ported
// yet. MegaKnightHooks.cpp holds TODO stubs with safe defaults; each
// declaration names the console function or member it stands for.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { class ApeRefCounted; }

namespace sly {
namespace entity { class Entity; }
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
// Camera controller (boss +0x290, 0x003055E8): blends to a camera record.
void MKCameraControllerSet(void* controller, const void* cameraDef, float ease);
// Platform object (boss +0x368, 0x00308F5C): reset, optionally `full`.
void MKPlatformReset(void* platform, bool full);
// Position of the platform object (+0x20).
ape::Vec3 MKPlatformPosition(const void* platform);

} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
// Spline definition by name (ApeTDefList<ApeSplineDef>::Find on 0x00F8F158).
void* MKFindSplineDef(uint32_t nameHash);
// Stamp manager (boss +0x294): no stamp is still falling (+0x5C), at least
// `count` stamps landed (+0x3C) and none is queued (+0x50).
bool MKStampsSettled(const void* stampManager, uint32_t count);
// Stamp manager: frees the pending stamp records (+0x48, 0x70 bytes each).
void MKStampsClearPending(void* stampManager);
// 0x0046C3B8 on the game world: queues the cut-scene sequence `name`.
void MKQueueSequence(uint32_t name);
} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
// Release of the boss's helper objects (camera controller +0x290: camera
// mode handed back to the game camera; stamp manager +0x294 0x0030B4AC;
// platform +0x368 with its two scripts).
void MKDeleteCameraController(void* controller);
void MKDeleteStampManager(void* stampManager);
void MKDeletePlatform(void* platform);
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
// Stamp manager reset (words +0x30/+0x5C/+0x60 cleared, then 0x0030B4AC).
void MKStampsReset(void* stampManager);
} // namespace gameplay
} // namespace sly
