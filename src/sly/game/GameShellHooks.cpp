// openTIT - Default bodies of the game shell's calls into unported modules
// (see GameShellHooks.h).
#include "sly/game/GameShellHooks.h"

namespace sly {

bool GadgetsAllowPause() { return true; }
void SetGadgetsPaused(bool) {}
void TravelTargetDeclined(entity::Entity*) {}
void AiSetContextualMarker(entity::EntityComponent*, ape::world::ApeObj*) {}
void GadgetPutAway(entity::EntityComponent*, bool, bool, bool) {}
void GadgetSetMeterVariant(entity::EntityComponent*, bool) {}
bool AiDebugMeterSwitch() { return false; }
int CharacterState(const entity::Entity*, uint32_t) { return 0; }
void FrameObjectRefresh(void*) {}
void CharacterThrowTowards(entity::Entity*, const ape::Vec3&) {}
void CharacterDropTowards(entity::Entity*, const ape::Vec3&) {}
void Component24Restart(entity::EntityComponent*, bool) {}

} // namespace sly
