// openTIT - Calls from the entity classes of 0x000F0000-0x00200000 into
// systems that are not ported yet (audio requests, entity base slots that
// Entity.h does not declare yet, physics). TODO stubs live in
// GameEntityHooks.cpp; each names the console function or slot.
#pragma once

#include "ape/math/ApeMath.h"
#include "sly/game/AudioSystem.h"

#include <cstdint>

namespace ape::world { class ApeObj; }

namespace sly {
namespace entity {

class Entity;
class EntityComponent;
class EntityDef;

// Engine game object virtual slot 3 (0x00464B90): leaves the world.
void EntityShutdown(Entity* entity);
// Marks an anim script as orphaned (its flag word +0x88 gets bit 9).
void DetachAnimScript(void* script);
// Orphans an anim script instance (flags +0x88: set bit 9, clear bit 10) and
// hands it to the deferred release list (0x004D0280).
void OrphanAndReleaseAnimScript(void* script);
// Bit 8 of an anim script instance's flag word (+0x88): paused.
void SetAnimScriptPaused(void* script, bool paused);
// Render object of an entity's character/physics component (0x00252940).
ape::world::ApeObj* ComponentRenderObject(EntityComponent* component);
// The active minigame (game world +0x1281F4) has bit 30 of its word +0x34 set.
bool MinigameFlag30();
// Entity virtual slot 8 (+0x20): script state of a layer.
int EntityGetState(const Entity* entity, int layer);

// Starts a sound at a position or attached to an object (0x0044C474 with the
// remaining parameters zero); returns the voice (0 when nothing played).
audio::VoiceId PlayEntitySound(audio::SoundName* sound, const ape::Vec3* position, const void* attachment);
// Lets a looping voice finish its current loop (0x00443A78 with 1).
void ReleaseVoiceLoop(audio::VoiceId voice);
// Copies the request parameters of a playing voice; false when the voice has
// no request any more (0x00443CE0).
bool GetVoiceParams(audio::VoiceId voice, audio::SoundParams& out);

// Takes the entity's transform callback (0x000DD778) off the game world's
// phase-0 callback list again (0x0046B7A4).
void DequeueEntityTransformCallback(Entity* entity);

// Frees storage obtained from the Havok heap allocator (0x011DF31C, slot 5).
void HavokFree(void* data, uint32_t bytes);

// Havok's growable pointer array (data, size, capacity with the
// "do not free" flag in bit 31), as embedded in some entities.
template <class T>
struct HkArray {
    static constexpr uint32_t kDontFree = 0x80000000u;
    T* data = nullptr;
    int32_t size = 0;
    uint32_t capacityAndFlags = kDontFree;

    ~HkArray()
    {
        size = 0;
        if (!(capacityAndFlags & kDontFree))
            HavokFree(data, capacityAndFlags * uint32_t(sizeof(T)));
        data = nullptr;
        capacityAndFlags = kDontFree;
    }
};

} // namespace entity
} // namespace sly
