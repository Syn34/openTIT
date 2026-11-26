// openTIT - Mega knight hooks (TODO stubs, see MegaKnightHooks.h).
#include "sly/gameplay/MegaKnightHooks.h"

namespace sly {
namespace gameplay {

void MKPlayClip(entity::Entity*, uint32_t, float, float, bool) {}
bool MKClipDone(entity::Entity*) { return true; }
bool MKRopeHeld(entity::Entity*) { return false; }
void MKRopeReset(entity::Entity*) {}
void MKRopeRelease(entity::Entity*) {}
void MKSetInteractionState(entity::Entity*, int) {}
entity::Entity* MKPlayerCharacter() { return nullptr; }
void MKAddTargetMarker(entity::Entity*, entity::Entity*, const ape::Vec3&) {}
void MKRemoveTargetMarker(entity::Entity*, entity::Entity*) {}
ape::ApeRefCounted* MKEntityScriptObject(entity::Entity*) { return nullptr; }
void MKCameraControllerSet(void*, const void*, float) {}
ape::world::ApeObj* MKGeometryObject(uint32_t) { return nullptr; }
ape::ApeRefCounted* MKObjectScriptRef(ape::world::ApeObj*) { return nullptr; }

} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
void* MKFindSplineDef(uint32_t) { return nullptr; }
bool MKStampsSettled(const void*, uint32_t) { return true; }
void MKStampsClearPending(void*) {}
void MKQueueSequence(uint32_t) {}
} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
void MKDeleteCameraController(void*) {}
void MKDeleteStampManager(void*) {}
void MKClearPlayerTargetingFlag(entity::Entity*) {}
void MKClearCharacterHooks(entity::Entity*) {}
void MKStopListening(entity::Entity*) {}
} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
void MKStampsReset(void*) {}
} // namespace gameplay
} // namespace sly
