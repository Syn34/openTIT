// openTIT - Characters (see Character.h).
#include "sly/entity/Character.h"

#include "ape/memory/ApeMem.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/GameCamera.h"
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
    if (fadeBuffer) {
        ape::MemFree(fadeBuffer);
        fadeBuffer = nullptr;
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
    if (playerName)
        ape::FreeIfNotNull(playerName);
}

// @func 0x00124370 Character::SetForceOpaque
void Character::SetForceOpaque(bool on)
{
    if (on)
        SetOpacity(0xFF);
    flags3C8 = on ? (flags3C8 | 0x08) : (flags3C8 & ~0x08);
}

// @func 0x0012999C Character::IsVariant04318BE6
bool Character::IsVariant04318BE6() const
{
    return (classBits & 0x20) && CharacterDefWord3D4(def) == 0x04318BE6u;
}

// @func 0x00129BC0 Character::StopScriptedAnimation
void Character::StopScriptedAnimation()
{
    EntityComponent* driver = GetComponent(kComp_17);
    if (driver && AnimDriverState(driver) == 7)
        AnimDriverStop(driver, false);
}

// @func 0x0012BD5C Character::SetComponent0EMode
void Character::SetComponent0EMode()
{
    if (EntityComponent* component = EntityComponent0E(this))
        SetComponentWord1C(component, 0xCABDE7BAu);
}

// @func 0x0013AE28 Character::ReleaseCameraOverride
void Character::ReleaseCameraOverride()
{
    GameCamera* camera = MainGameCamera();
    if (camera && camera->HasMode(cameraMode)) {
        cameraMode->RemoveOverride(&cameraSettings);
        cameraMode = nullptr;
    }
}

// @func 0x0011EFDC CharacterDef::GetDisplayName
const char* CharacterDef::GetDisplayName() const
{
    return (playerName && *playerName) ? playerName : name;
}

// @func 0x00121B40 CharacterDef::LinkedDef
EntityDef* CharacterDef::LinkedDef(uint32_t index) const
{
    return index < 4 ? linkedDefs[index] : nullptr;
}

// @func 0x00121B68 CharacterDef::FindLinkedDef
int CharacterDef::FindLinkedDef(const EntityDef* linked) const
{
    for (int i = 0; i < 4 && linkedDefs[i]; ++i) {
        if (linkedDefs[i] == linked)
            return i;
    }
    return -1;
}

} // namespace entity
} // namespace sly
