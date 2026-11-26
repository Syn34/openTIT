// openTIT - Stubs for the calls out of the actor classes (see ActorHooks.h).
#include "sly/entity/ActorHooks.h"

namespace sly {
namespace entity {

// TODO(0x002B7704 InteractionComponent::Stop)
void InteractionStop(EntityComponent* interaction) { (void)interaction; }

// TODO(0x0046B5F8 SlyGame::QueueCallback list 3, callback 0x001E9C64)
void GameWorldQueueSafeCallback(Entity* safe) { (void)safe; }

// TODO(0x0046426C motion sensor history query)
bool AppPlayerMotionSamples(AppPlayer& player, float window, ape::Vec3* out, int maxSamples, int& count)
{
    (void)player; (void)window; (void)out; (void)maxSamples;
    count = 0;
    return false;
}

} // namespace entity
} // namespace sly
