// openTIT - Default bodies of the game shell's calls into unported modules
// (see GameShellHooks.h).
#include "sly/entity/Entity.h"
#include "sly/game/GameShellHooks.h"

namespace sly {

bool GadgetsAllowPause() { return true; }
void SetGadgetsPaused(bool) {}
void TravelTargetDeclined(entity::Entity*) {}
void AiSetContextualMarker(entity::EntityComponent* controller, ape::world::ApeObj* marker)
{
    static_cast<entity::InteractionComponent*>(controller)->SetContextualMarker(marker);
}
void GadgetPutAway(entity::EntityComponent*, bool, bool, bool) {}
void GadgetSetMeterVariant(entity::EntityComponent*, bool) {}
bool AiDebugMeterSwitch() { return false; }
int CharacterState(const entity::Entity*, uint32_t) { return 0; }
void CameraMode1SetTarget(void*, entity::Entity*) {}
void CharacterThrowTowards(entity::Entity*, const ape::Vec3&) {}
void CharacterDropTowards(entity::Entity*, const ape::Vec3&) {}
void Component24Restart(entity::EntityComponent*, bool) {}
int VehicleDefCameraKind(const void*) { return 1; }
float VehicleDefCameraFov(const void*) { return 55.0f; }
void HudFrameBegin(float) {}
void SetEngineUpdateMasks(uint8_t) {}
void HudFrameEnd(float) {}

} // namespace sly
