// openTIT - The per-entity steps of the frame update (0x000CEA48-0x000CF660)
// and the root motion slot (0x000D8954).
//
// On the console a job sorts the live entities into an update list and a
// post-update list (0x01019BE0 / 0x0101DBF0); the main thread then runs the
// steps below for every entry (0x000CF398 / 0x000CF898).
#include "sly/entity/Entity.h"

#include "sly/camera/CameraHooks.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/game/AudioSystem.h"

#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"
#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"

#include <cmath>

namespace sly {
namespace entity {

namespace {
bool SameTransform(const ape::Matrix34& a, const ape::Matrix34& b)
{
    auto same = [](const ape::Vec3& p, const ape::Vec3& q) { return p.x == q.x && p.y == q.y && p.z == q.z; };
    return same(a.xAxis, b.xAxis) && same(a.yAxis, b.yAxis) && same(a.zAxis, b.zAxis) &&
           same(a.translation, b.translation);
}

// Per-frame class bits (+0xB8). NOTE: meanings unknown; bit 7 mirrors the
// "hidden" flag bits 5/6 of the entity flags.
constexpr uint64_t kClassFrame_02 = 0x02;
constexpr uint64_t kClassFrame_08 = 0x08;
constexpr uint64_t kClassFrame_40 = 0x40;
constexpr uint64_t kClassFrame_80 = 0x80;
constexpr uint32_t kEnt_Flag20 = 0x20;
constexpr uint32_t kEnt_Flag40 = 0x40;

// World callback list 2: entities with children place them after the
// update (0x000DD778).
// @func 0x000DD778 QueuedUpdateTransform
void QueuedUpdateTransform(void* owner, Entity* entity)
{
    (void)owner;
    entity->UpdateTransform();
}

const ape::Vec4 kNoRotation = { 0.0f, 0.0f, 0.0f, 1.0f };   // 0x01164C90
const ape::Vec3 kNoOffset = { 0.0f, 0.0f, 0.0f, 0.0f };     // 0x01164B40
} // namespace

// @func 0x000CEA48 Entity::AttachTransform
ape::Matrix34 Entity::AttachTransform(int bone, const ape::world::ApeObj* attachObject, const ape::Vec4& rotation,
                                      const ape::Vec3& offset) const
{
    ape::Matrix34 frame = transform;
    if (bone == -1) {
        if (attachObject)
            frame = attachObject->GetWorld();
    } else {
        frame = BoneTransform(bone, true, true);
    }
    ape::Matrix34 local;
    ape::Matrix34FromQuat(local, rotation, offset);
    ape::Matrix34 out;
    ape::world::Matrix34Mul(out, local, frame);
    return out;
}

// @func 0x000CEDD8 Entity::UpdateMotion
void Entity::UpdateMotion(float dt)
{
    if (std::fabs(dt) >= 1.0e-5f) {
        const float inv = 1.0f / dt;
        const ape::Vec3& p = Position();
        velocity = { (p.x - lastPosition.x) * inv, (p.y - lastPosition.y) * inv, (p.z - lastPosition.z) * inv,
                     velocity.pad };
        lastPosition = { p.x, p.y, p.z, lastPosition.pad };
    }
    // Attached sounds follow their bone or object; a voice that has ended
    // is stopped and its slot filled with the last one.
    for (uint32_t i = 0; i < SoundSlotCount(); ++i) {
        EntitySoundSlot& slot = sounds[i];
        const ape::Matrix34 at =
            AttachTransform(slot.id, static_cast<const ape::world::ApeObj*>(slot.attachObject), kNoRotation, kNoOffset);
        if (VoiceUpdate3D(slot.voice, at.translation, velocity))
            continue;
        audio::StopVoice(slot.voice, true);
        const uint32_t last = SoundSlotCount() - 1;
        flags = (flags & ~kEnt_SoundCountMask) | (last << kEnt_SoundCountShift);
        slot = sounds[last];
        sounds[last] = EntitySoundSlot{};
        --i;
    }
}

// @func 0x000D8954 Entity::ApplyRootMotion
void Entity::ApplyRootMotion(float dt)
{
    if (animController) {
        const ape::Matrix34 delta = EntityRootMotionDelta(this, dt);
        if (!SameTransform(delta, ape::world::kWorldIdentity)) {
            ape::Matrix34 moved;
            ape::world::Matrix34Mul(moved, delta, transform);
            SetTransform(moved);
            SetLocalTransform(moved);
        }
    }
    // Keep going while the animation plays, and forever for a looping one.
    auto* layer = animController ? static_cast<anim::AnimController*>(animController)->FindLayer(0) : nullptr;
    if (layer && !layer->IsDone(false))
        return;
    if (!layer || !(layer->flags & anim::AnimLayer::kFlag_Looping))
        flags &= ~uint32_t(kEnt_RootMotion);
}

// @func 0x000CF0C0 EntityUpdateStep
void EntityUpdateStep(Entity* entity, float dt)
{
    if (!entity)
        return;
    entity->flags &= ~uint32_t(kEnt_TransformChanged);
    if (entity->flags & (kEnt_Flag40 | kEnt_Flag20))
        entity->classBits = (entity->classBits & ~(kClassFrame_02 | kClassFrame_08)) | kClassFrame_80;
    else
        entity->classBits &= ~(kClassFrame_02 | kClassFrame_08 | kClassFrame_80);
    entity->classBits &= ~kClassFrame_40;
    // NOTE: +0xD4 ("cullDistance") scales the entity's frame time.
    if (!entity->Update(dt * entity->cullDistance)) {
        if (entity->IsAlive())
            entity->Release(true);
        return;
    }
    if (entity->children && entity->children->Count() != 0)
        GameWorldQueueCallback(2, &QueuedUpdateTransform, entity, entity);
    entity->UpdateMotion(dt);
}

// @func 0x000CF534 EntityPostUpdateStep
// Visible entities (unless hidden, or when flags bit 15 forces it) run
// slot 5; a root object follows the entity unless someone else moved it;
// then root motion and the animation update.
void EntityPostUpdateStep(Entity* entity, float dt)
{
    if (!entity)
        return;
    const float scaledDt = dt * entity->cullDistance;
    const uint32_t f = entity->flags;
    const bool shown = (f & kEnt_Visible) || (f & kEnt_10000);
    const bool hidden = (f & kEnt_Flag40) || (f & kEnt_Flag20);
    if (shown && (!hidden || (f & kEnt_8000))) {
        if (!entity->PostUpdate(scaledDt)) {
            if (entity->IsAlive())
                entity->Release(true);
            return;
        }
    }
    ape::world::ApeObj* object = entity->object;
    if (!(entity->flags & kEnt_NoObjectSync) && object && !object->GetParent() &&
        !(entity->flags & kEnt_TransformChanged)) {
        if (!SameTransform(object->m_local, entity->transform)) {
            ape::Matrix34Copy(object->m_local, entity->transform);
            object->MarkDirty();
        }
    }
    if (entity->animController) {
        EntityComponent* character = entity->GetCharacterComponent();
        if (entity->flags & kEnt_RootMotion)
            entity->ApplyRootMotion(scaledDt);
        if (character)
            CharacterCompBeginAnim(character);
        static_cast<anim::AnimController*>(entity->animController)->UpdateLayers(scaledDt);
        if (character)
            CharacterCompEndAnim(character);
    }
    if (entity->flags2 & kEnt2_AlphaSet)
        entity->flags2 &= ~uint16_t(kEnt2_AlphaSet);
}

} // namespace entity
} // namespace sly
