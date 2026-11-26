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
ape::world::ApeObj* MKGeometryObject(uint32_t) { return nullptr; }
ape::ApeRefCounted* MKObjectScriptRef(ape::world::ApeObj*) { return nullptr; }

} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
void* MKFindSplineDef(uint32_t) { return nullptr; }
void MKQueueSequence(uint32_t) {}
} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
void MKPlayerBeginFinishingMove(entity::Entity*) {}
void MKQueueSequenceBound(uint32_t, const MKSequenceBinding*, size_t) {}
bool MKFrameFrozen() { return false; }
bool MKSequenceQueued(uint32_t) { return false; }
entity::GameCamera* MKGameCamera() { return nullptr; }
void MKRippleKnock(entity::Entity*) {}
void MKCameraRemoveMode(entity::GameCamera*, void*) {}
void MKClearPlayerTargetingFlag(entity::Entity*) {}
void MKClearCharacterHooks(entity::Entity*) {}
void MKStopListening(entity::Entity*) {}
} // namespace gameplay
} // namespace sly

namespace sly {
namespace gameplay {
} // namespace gameplay
} // namespace sly
