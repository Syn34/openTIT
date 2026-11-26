// openTIT - Entity clip playback (virtual slots 49-51 of Entity).
#include "sly/entity/Entity.h"
#include "sly/entity/EntityTuning.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"

#include "ape/core/ApeString.h"

namespace sly {
namespace entity {

// @func 0x000DBE78 Entity::PlayClip
bool Entity::PlayClip(float speed, float blend, float duration, void* clip, uint32_t layer, bool loop,
                      uint32_t listener)
{
    if (blend < 0.0f)
        blend = g_entityBlendTimeDefault;
    auto* controller = static_cast<anim::AnimController*>(animController);
    if (!controller || !clip)
        return false;

    auto* binding = static_cast<anim::HkaAnimationBinding*>(clip);
    anim::AnimLayer* playing = controller->FindLayer(layer);
    if (playing) {
        // NOTE: the console also hands a further register argument of its
        // caller to the layer and controller Play calls; the ports of those
        // do not take it.
        playing->Play(binding, speed, blend, 0.0f, loop);
        playing->AttachListener(listener);
    } else {
        playing = controller->Play(binding, speed, blend, 0.0f, loop, listener);
    }
    // Stretch the clip to the requested length.
    if (playing && duration > 0.0f)
        SetAnimWeight(playing->Duration() / duration, layer);
    return true;
}

// @func 0x000DC010 Entity::PlayClipByHash
bool Entity::PlayClipByHash(float speed, float blend, float duration, uint32_t clip, uint32_t layer, bool loop,
                            uint32_t listener)
{
    return PlayClip(speed, blend, duration, FindAnimClip(clip), layer, loop, listener);
}

// @func 0x000DC0F8 Entity::PlayClipByName
bool Entity::PlayClipByName(float speed, float blend, float duration, const char* clip, uint32_t layer,
                            bool loop, const char* prefix, uint32_t listener)
{
    if (children) {
        for (EntityChildLink& link : *children) {
            if (!(link.flags & kChild_ForwardAnim))
                continue;
            link.child->PlayClipByName(speed, blend, 0.0f, clip, layer, loop, ape::TempFormat("%s_", link.name),
                                       listener);
        }
    }
    const char* fullName = prefix ? ape::TempFormat("%s%s", prefix, clip) : clip;
    return PlayClip(speed, blend, duration, FindAnimClipByName(fullName), layer, loop, listener);
}

} // namespace entity
} // namespace sly
