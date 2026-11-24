// openTIT - Characters (see Character.h).
#include "sly/entity/Character.h"

#include "ape/memory/ApeMem.h"
#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

// @func 0x00121BDC Character::~Character
// @func 0x00121E24 Character::~Character (deleting)
// @func 0x00121E6C Character::~Character (base object)
Character::~Character()
{
    EntityShutdown(this);

    // The active controller may be one of the other two; delete each once.
    if (extController != controller && extController)
        delete extController;
    extController = nullptr;
    if (aiController && aiController != controller)
        delete aiController;
    aiController = nullptr;
    if (controller) {
        delete controller;
        controller = nullptr;
    }

    for (void*& script : animScripts) {
        if (script) {
            DetachAnimScript(script);
            script = nullptr;
        }
    }
    if (field_3D0) {
        ape::MemFree(field_3D0);
        field_3D0 = nullptr;
    }
    audio::SoundNameRelease(sounds[2]);
    audio::SoundNameRelease(sounds[0]);
    audio::SoundNameRelease(sounds[1]);
    for (int i = 3; i < 7; ++i)
        audio::SoundNameRelease(sounds[i]);
}

// @func 0x0012492C Character::ApplyVisibility
void Character::ApplyVisibility(bool visible)
{
    Entity::ApplyVisibility(visible);
}

// @func 0x00129148 Character::GetRenderObject
ape::world::ApeObj* Character::GetRenderObject()
{
    EntityComponent* component = GetComponent00();
    return component ? ComponentRenderObject(component) : nullptr;
}

// @func 0x0011BBC4 CharacterDef::~CharacterDef
// @func 0x0011BCF4 CharacterDef::~CharacterDef (base object)
// @func 0x0011BC50 CharacterDef::~CharacterDef (deleting)
CharacterDef::~CharacterDef()
{
    if (field_588)
        ape::FreeIfNotNull(field_588);
}

} // namespace entity
} // namespace sly
