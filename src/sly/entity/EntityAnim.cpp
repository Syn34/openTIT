// openTIT - Entity clip playback (virtual slots 49-51 of Entity).
#include "sly/entity/Entity.h"
#include "sly/entity/EntityTuning.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/EntityHooks.h"

#include <cmath>

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

// @func 0x000DC730 Entity::RootMotionDelta
void Entity::RootMotionDelta(ape::Matrix34& out, bool consume, uint32_t layer, float dt)
{
    (void)consume;
    void* playing = FindPlayingAnim(layer);
    if (!playing)
        return;
    AnimLayerRootMotion(playing, dt, out);
    if ((flags2 & kEnt2_RootMotionObjectScale) && object && object->HasScale()) {
        const ape::Vec3& scale = object->GetScale();
        out.translation.x *= scale.x;
        out.translation.y *= scale.y;
        out.translation.z *= scale.z;
    }
    if (!(flags2 & kEnt2_RootMotionDamped))
        return;
    // Scale the translation and keep only part of the rotation (blended
    // towards no rotation along the shorter way).
    const ape::Vec3 move = {out.translation.x * rootMotionScale.x, out.translation.y * rootMotionScale.y,
                            out.translation.z * rootMotionScale.z, 0.0f};
    ape::Quat turn;
    ape::QuatFromMatrix34(turn, out);
    if (turn.w < 0.0f) {
        turn.x = -turn.x;
        turn.y = -turn.y;
        turn.z = -turn.z;
        turn.w = -turn.w;
    }
    const float w = rootMotionWeight;
    ape::Quat kept = {turn.x * w, turn.y * w, turn.z * w, (turn.w - 1.0f) * w + 1.0f};
    const float inv = 1.0f / std::sqrt(kept.y * kept.y + kept.x * kept.x + kept.z * kept.z + kept.w * kept.w);
    kept = {kept.x * inv, kept.y * inv, kept.z * inv, kept.w * inv};
    ape::Matrix34FromQuat(out, kept, move);
}

} // namespace entity
} // namespace sly
