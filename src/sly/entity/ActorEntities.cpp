// openTIT - Scripted actors (see ActorEntities.h).
#include "sly/entity/ActorEntities.h"

#include "ape/memory/ApeMem.h"
#include "ape/world/ApeObj.h"
#include "sly/camera/CameraHooks.h"
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

// Minimal version of 0x000E281C (entity module).
ActorDef::~ActorDef()
{
    if (parts) {
        for (Part& part : *parts) {
            if (part.havokObject)
                camera::HavokRemoveReference(part.havokObject);
        }
        delete parts;
    }
}

// @func 0x001AD158 SoundActorDef::~SoundActorDef
// @func 0x001AD188 SoundActorDef::~SoundActorDef (deleting)
SoundActorDef::~SoundActorDef() = default;

// @func 0x001DC8B8 InteractivePropADef::~InteractivePropADef
// @func 0x001DC910 InteractivePropADef::~InteractivePropADef (deleting)
InteractivePropADef::~InteractivePropADef()
{
    if (buffer)
        ape::FreeIfNotNull(buffer);
}

// @func 0x001E52E8 SafeDef::~SafeDef
// @func 0x001E5318 SafeDef::~SafeDef (deleting)
SafeDef::~SafeDef() = default;

// @func 0x001DC04C InteractivePropA::~InteractivePropA
// @func 0x001DC104 InteractivePropA::~InteractivePropA (deleting)
// @func 0x001DC1D4 InteractivePropA::~InteractivePropA (deleting thunk, +0x180)
// @func 0x001DC1FC InteractivePropA::~InteractivePropA (deleting thunk, +0x17C)
// @func 0x001DC22C InteractivePropA::~InteractivePropA (thunk, +0x180)
// @func 0x001DC2E8 InteractivePropA::~InteractivePropA (thunk, +0x17C)
InteractivePropA::~InteractivePropA() = default;

// @func 0x001DC590 InteractivePropA::Update
bool InteractivePropA::Update(float dt)
{
    // NOTE: the console looks the interaction component up and discards it.
    (void)EntityInteraction(this);
    return ActorEntity::Update(dt);
}

// @func 0x001EA650 SearchlightActor::~SearchlightActor
// @func 0x001EA708 SearchlightActor::~SearchlightActor (deleting)
// @func 0x001EA7D8 SearchlightActor::~SearchlightActor (deleting thunk, +0x180)
// @func 0x001EA800 SearchlightActor::~SearchlightActor (deleting thunk, +0x17C)
// @func 0x001EA830 SearchlightActor::~SearchlightActor (thunk, +0x180)
// @func 0x001EA8EC SearchlightActor::~SearchlightActor (thunk, +0x17C)
SearchlightActor::~SearchlightActor() = default;

// @func 0x001EA9A8 SearchlightActor::Release
bool SearchlightActor::Release(bool kill)
{
    return ActorEntity::Release(kill);
}

// @func 0x001EB0B4 SearchlightActor::InteractionOffer
uint32_t SearchlightActor::InteractionOffer(int /*kind*/, Entity* /*other*/)
{
    return 0x10;
}

// @func 0x001EB09C SearchlightActor::InteractionNeeds
uint32_t SearchlightActor::InteractionNeeds(int kind, Entity* /*other*/)
{
    return Def()->interactionNeeds[kind];
}

// @func 0x001EB0BC SearchlightActor::CameraOffset
ape::Vec3 SearchlightActor::CameraOffset() const
{
    const ape::Vec3& offset = Def()->cameraOffset;
    return { offset.x, offset.y, offset.z, 0.0f };
}

// @func 0x001EB0DC SearchlightActor::Field150
ape::Vec4 SearchlightActor::Field150() const
{
    return Def()->field_150;
}

// @func 0x001F62DC InteractivePropB::~InteractivePropB
// @func 0x001F6394 InteractivePropB::~InteractivePropB (deleting)
// @func 0x001F6464 InteractivePropB::~InteractivePropB (deleting thunk, +0x180)
// @func 0x001F648C InteractivePropB::~InteractivePropB (deleting thunk, +0x17C)
// @func 0x001F64BC InteractivePropB::~InteractivePropB (thunk, +0x180)
// @func 0x001F6578 InteractivePropB::~InteractivePropB (thunk, +0x17C)
InteractivePropB::~InteractivePropB() = default;

// @func 0x001AE468 SoundActor::~SoundActor
// @func 0x001AE574 SoundActor::~SoundActor (deleting)
// @func 0x001AE5BC SoundActor::~SoundActor (deleting thunk, +0x180)
// @func 0x001AE608 SoundActor::~SoundActor (deleting thunk, +0x17C)
// @func 0x001AE654 SoundActor::~SoundActor (thunk, +0x180)
// @func 0x001AE67C SoundActor::~SoundActor (thunk, +0x17C)
SoundActor::~SoundActor()
{
    for (SoundSlot& slot : sounds)
        audio::SoundNameRelease(slot.sound);
}

// @func 0x001AE980 SoundActor::SetActive
void SoundActor::SetActive(bool active)
{
    if (animScript)
        SetAnimScriptPaused(animScript, !active);
    ActorEntity::SetActive(active);
}

// @func 0x001C33C4 SoundActorB16B48::~SoundActorB16B48
// @func 0x001C345C SoundActorB16B48::~SoundActorB16B48 (deleting)
// @func 0x001C3510 SoundActorB16B48::~SoundActorB16B48 (deleting thunk, +0x180)
// @func 0x001C35C8 SoundActorB16B48::~SoundActorB16B48 (deleting thunk, +0x17C)
// @func 0x001C3688 SoundActorB16B48::~SoundActorB16B48 (thunk, +0x180)
// @func 0x001C3724 SoundActorB16B48::~SoundActorB16B48 (thunk, +0x17C)
SoundActorB16B48::~SoundActorB16B48() = default;

// @func 0x001CBE10 SoundActorB17270::~SoundActorB17270
// @func 0x001CBE90 SoundActorB17270::~SoundActorB17270 (deleting)
// @func 0x001CBF2C SoundActorB17270::~SoundActorB17270 (deleting thunk, +0x180)
// @func 0x001CBFCC SoundActorB17270::~SoundActorB17270 (deleting thunk, +0x17C)
// @func 0x001CC06C SoundActorB17270::~SoundActorB17270 (thunk, +0x180)
// @func 0x001CC0F0 SoundActorB17270::~SoundActorB17270 (thunk, +0x17C)
SoundActorB17270::~SoundActorB17270()
{
    field_32C.Clear();
}

// @func 0x001D1D94 SoundActorB174C0::~SoundActorB174C0
// @func 0x001D1E0C SoundActorB174C0::~SoundActorB174C0 (deleting)
// @func 0x001D1EA0 SoundActorB174C0::~SoundActorB174C0 (deleting thunk, +0x180)
// @func 0x001D1F38 SoundActorB174C0::~SoundActorB174C0 (deleting thunk, +0x17C)
// @func 0x001D1FD8 SoundActorB174C0::~SoundActorB174C0 (thunk, +0x180)
// @func 0x001D2054 SoundActorB174C0::~SoundActorB174C0 (thunk, +0x17C)
SoundActorB174C0::~SoundActorB174C0() = default;

// @func 0x001EB5E0 ObjectActor::~ObjectActor
// @func 0x001EB6E4 ObjectActor::~ObjectActor (deleting)
// @func 0x001EB72C ObjectActor::~ObjectActor (deleting thunk, +0x180)
// @func 0x001EB778 ObjectActor::~ObjectActor (deleting thunk, +0x17C)
// @func 0x001EB7C4 ObjectActor::~ObjectActor (thunk, +0x180)
// @func 0x001EB7EC ObjectActor::~ObjectActor (thunk, +0x17C)
ObjectActor::~ObjectActor()
{
    if (object_1F4)
        object_1F4->Release();
}

// @func 0x001ED5F4 TweenActor::~TweenActor
// @func 0x001ED72C TweenActor::~TweenActor (deleting)
// @func 0x001ED774 TweenActor::~TweenActor (deleting thunk, +0x180)
// @func 0x001ED7C0 TweenActor::~TweenActor (deleting thunk, +0x17C)
// @func 0x001ED80C TweenActor::~TweenActor (thunk, +0x180)
// @func 0x001ED834 TweenActor::~TweenActor (thunk, +0x17C)
TweenActor::~TweenActor() = default;

// @func 0x001EEE40 TweenActor::Update
bool TweenActor::Update(float dt)
{
    return ActorEntity::Update(dt);
}

// @func 0x001EEE78 TweenActor::PostUpdate
bool TweenActor::PostUpdate(float dt)
{
    return ActorEntity::PostUpdate(dt);
}

// @func 0x001F4088 TweenActor::UpdateTransform
void TweenActor::UpdateTransform(float /*dt*/) {}

// @func 0x001F509C TweenActor::ApplyVisibility
void TweenActor::ApplyVisibility(bool visible)
{
    if (!visible && field_358)
        Reset(true);
}

// TODO(0x001EE680 TweenActor::Reset)
void TweenActor::Reset(bool flag)
{
    (void)flag;
}

// @func 0x001F7128 InteractivePropB::SetVisible
void InteractivePropB::SetVisible(bool visible, bool immediate, bool children, bool components)
{
    Entity::SetVisible(visible, immediate, children, components);
}

} // namespace entity
} // namespace sly
