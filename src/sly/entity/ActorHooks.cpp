// openTIT - Stubs for the calls out of the actor classes (see ActorHooks.h).
#include "sly/entity/ActorHooks.h"

#include "sly/entity/Character.h"
#include "sly/entity/CharacterHooks.h"

namespace sly {
namespace entity {

void InteractionStop(EntityComponent* interaction)
{
    static_cast<InteractionComponent*>(interaction)->Stop();
}

// TODO(skin component +0x4D8 / +0x500, components module)
void SkinSetOwnerGroup(EntityComponent* skin, uint32_t group) { (void)skin; (void)group; }
void SkinSetShotCallback(EntityComponent* skin, void (*callback)(Entity* owner)) { (void)skin; (void)callback; }

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
