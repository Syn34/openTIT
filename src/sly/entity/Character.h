// openTIT - Characters (console vtable 0x00B14168, 0x550 bytes) and their
// definition (vtable 0x00B14138).
//
// Every playable and AI character is a Character: a scripted entity with up
// to three controllers (the AI controller "ControlAI", a temporary external
// controller "TmpExtControl" and the one currently driving it), damage
// settings, per-character sounds, follow-camera settings and a large state
// machine of movement states (0x00122000-0x00166000). Only the members the
// ported code touches are modelled.
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"
#include "sly/camera/CameraSettings.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/game/AudioSystem.h"

namespace sly {
namespace entity {

// Base of the small tunable blocks embedded in definitions and characters
// (console vtable 0x00B0DB3C: destructor plus two callbacks, code at
// 0x0073799C-0x007379F4).
class TunableBlock {
public:
    virtual ~TunableBlock() = default;
    virtual void Load(const void* xml) { (void)xml; }    // NOTE: slot 2, name assumed
    virtual void Slot3() {}
};

// Damage settings of a character (vtable 0x00B0DB6C, Entity.Character.Damage.*).
class DamageSettings : public TunableBlock {
public:
    float impactScalarDyn = 0.0f;      // +0x04 (character +0x1F0)
    float impactScalarKey = 0.0f;
    float melee = 0.0f;
    float blast = 0.0f;
    float environment = 0.0f;
    float environmentFire = 0.0f;
    float environmentElectric = 0.0f;
    float environmentPoisonDuration = 0.0f;
    float projectile = 0.0f;
    bool projectileReflect = false;
    bool projectileRandomReflect = false;
    float projectileAngle = 0.0f;
    float projectileVelScalar = 0.0f;
};

// Something that drives a character (AI, player input, scripted control).
// Only the virtual destructor is used here.
class CharacterController {
public:
    virtual ~CharacterController() = default;
};

class Character : public ScriptedEntity {
public:
    ~Character() override;

    Entity* field_17C = nullptr;                    // 0x17C
    CharacterController* controller = nullptr;      // 0x180 the one in charge (may alias the others)
    CharacterController* aiController = nullptr;    // 0x184 "ControlAI"
    CharacterController* extController = nullptr;   // 0x188 "TmpExtControl"
    DamageSettings damage;                          // 0x1EC
    void* animScripts[8] = {};                      // 0x31C (stride 0x10 on the console)
    // 0x394 NOTE: built with capacity 2 and its own comparator (0x00F32D90).
    ape::ApeTSortedMap<uint32_t> field_394;
    audio::SoundName* sounds[7] = {};               // 0x3AC
    void* field_3D0 = nullptr;                      // 0x3D0 owned buffer
    camera::CameraSettings cameraSettings;          // 0x4B0
    ape::ApeTArray<void*> field_530;                // 0x530
};

// Character definition (0x5A0 bytes on the console). The tunable blocks are
// kept opaque; the camera block holds the CameraSettings the follow camera
// takes (+0x3F0).
class CharacterDef : public ScriptedEntityDef {
public:
    ~CharacterDef() override;

    TunableBlock block_F0;          // 0x0F0
    TunableBlock block_2D0;         // 0x2D0
    TunableBlock block_304;         // 0x304
    TunableBlock cameraBlock;       // 0x3E0 (settings at +0x10)
    camera::CameraSettings cameraSettings;   // 0x3F0
    TunableBlock block_470;         // 0x470
    TunableBlock block_510;         // 0x510
    void* field_588 = nullptr;      // 0x588 owned buffer
};

} // namespace entity
} // namespace sly
