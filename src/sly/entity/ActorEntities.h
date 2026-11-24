// openTIT - Scripted actors: the entity base of most interactive world
// objects (console vtable 0x00B12898, destructor 0x007476B0 in the entity
// helper range) and the actor classes of 0x001A0000-0x00200000 whose
// teardown is ported. Class names are descriptive (the original names are
// not known); the console vtable is given for each.
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/game/AudioSystem.h"
#include "sly/game/Tween.h"

namespace ape::world { class ApeObj; }

namespace sly {
namespace entity {

// Actor base (0x00B12898 + 0x00B12A44/0x00B12A6C at +0x17C/+0x180).
class ActorEntity : public ScriptedEntity, public EntityInterface17C, public EntityInterface180 {
public:
    // The console's out-of-line destructor is 0x007476B0 (not in this range);
    // derived classes inline it.
    ~ActorEntity() override { delete field_184; }

    ape::ApeTArray<void*>* field_184 = nullptr;   // 0x184 allocated on demand
};

// Interactive prop that refuses player interaction while its definition says
// so (vtable 0x00B17BB8, 0x1B0 bytes; slot 76 0x001DC3A8).
class InteractivePropA : public ActorEntity {
public:
    ~InteractivePropA() override;
};

// Player-steered searchlight (vtable 0x00B18260, 0x1D0 bytes); the spotlight
// camera mode rides on it.
class SearchlightActor : public ActorEntity {
public:
    ~SearchlightActor() override;
};

// Interactive prop driven by an anim script variable (vtable 0x00B18908,
// 0x240 bytes; slot 76 0x001F6808).
class InteractivePropB : public ActorEntity {
public:
    ~InteractivePropB() override;
};

// Actor with seven per-instance sounds (vtable 0x00B159B8); base of the
// three classes below.
class SoundActor : public ActorEntity {
public:
    ~SoundActor() override;

    struct SoundSlot {
        audio::SoundName* sound = nullptr;
        uint32_t field_04[3] = {};
    };
    SoundSlot sounds[7];                      // 0x1A8
};

// vtable 0x00B16B48 (0x260 bytes).
class SoundActorB16B48 : public SoundActor {
public:
    ~SoundActorB16B48() override;
    ape::ApeTArray<void*> field_22C;          // 0x22C
    ape::ApeTArray<void*> field_240;          // 0x240
};

// vtable 0x00B17270 (0x360 bytes).
class SoundActorB17270 : public SoundActor {
public:
    ~SoundActorB17270() override;
    ape::ApeTArray<void*> field_32C;          // 0x32C emptied before it is freed
};

// vtable 0x00B174C0 (0x3D0 bytes).
class SoundActorB174C0 : public SoundActor {
public:
    ~SoundActorB174C0() override;
    ape::ApeTArray<void*> field_22C;          // 0x22C
};

// Actor holding a world object reference (vtable 0x00B18488, 0x200 bytes).
class ObjectActor : public ActorEntity {
public:
    ~ObjectActor() override;
    ape::world::ApeObj* object_1F4 = nullptr; // 0x1F4 counted reference
};

// Actor with two tweens and two arrays (vtable 0x00B186C8).
class TweenActor : public ActorEntity {
public:
    ~TweenActor() override;
    FloatTween tween_220;                     // 0x220
    ape::ApeTArray<void*> field_280;          // 0x280
    IntTween tween_36C;                       // 0x36C
    ape::ApeTArray<void*> field_3A0;          // 0x3A0
};

} // namespace entity
} // namespace sly
