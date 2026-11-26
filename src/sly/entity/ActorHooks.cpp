// openTIT - Stubs for the calls out of the actor classes (see ActorHooks.h).
#include "sly/entity/ActorHooks.h"

#include "sly/entity/Character.h"
#include "sly/entity/CharacterHooks.h"

namespace sly {
namespace entity {

namespace {
constexpr uint32_t kCharacterTypeBit = 0x8u;   // entity type bit of the characters
}

void InteractionStop(EntityComponent* interaction)
{
    static_cast<InteractionComponent*>(interaction)->Stop();
}

// @func 0x002B7704 InteractionComponent::Stop
bool InteractionComponent::Stop()
{
    if (owner)
        owner->InteractionCancel();
    if (user) {
        if (user->typeBits[0] & kCharacterTypeBit)
            static_cast<Character*>(user)->LeaveInteractState();
        user->Release(false);
    }
    user = nullptr;
    InteractionSetOffer(this, 0);
    return true;
}

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
