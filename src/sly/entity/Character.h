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

namespace sly::camera { class DefaultCameraMode; }

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
    void ApplyVisibility(bool visible) override;
    // Characters never ask anything of an interaction partner.
    // @func 0x00127ECC Character::InteractionNeeds
    uint32_t InteractionNeeds(int kind, Entity* other) override { (void)kind; (void)other; return 0; }
    // Render object of the character component.
    ape::world::ApeObj* GetRenderObject() override;

    // Bit 3 of +0x3C8; turning it on also restores full opacity.
    // NOTE: name from use (0x00124370).
    void SetForceOpaque(bool on);
    // Sets bit 1 of +0x3C8 (0x0012D344).
    void SetFlag3C8Bit1() { flags3C8 |= 0x02; }
    // Opacity of the character and its attachments (0x001241A0).
    void SetOpacity(uint8_t alpha);
    // Character variant check: class bit 5 and definition word +0x3D4 ==
    // 0x04318BE6 (0x0012999C). NOTE: meaning of the variant unknown.
    bool IsVariant04318BE6() const;
    // Stops the animation driver (component 0x17) when it is playing a
    // script (state 7) (0x00129BC0).
    void StopScriptedAnimation();
    // Component 0x0E: selects mode 0xCABDE7BA (word +0x1C) (0x0012BD5C).
    void SetComponent0EMode();
    // Takes the character's camera settings override off the default camera
    // mode that holds it (0x0013AE28).
    void ReleaseCameraOverride();

    Entity* field_17C = nullptr;                    // 0x17C
    CharacterController* controller = nullptr;      // 0x180 the one in charge (may alias the others)
    CharacterController* aiController = nullptr;    // 0x184 "ControlAI"
    CharacterController* extController = nullptr;   // 0x188 "TmpExtControl"
    DamageSettings damage;                          // 0x1EC
    void* animScripts[8] = {};                      // 0x31C (stride 0x10 on the console)
    // 0x394 NOTE: built with capacity 2 and its own comparator (0x00F32D90).
    ape::ApeTSortedMap<uint32_t> field_394;
    audio::SoundName* sounds[7] = {};               // 0x3AC
    uint8_t flags3C8 = 0;                           // 0x3C8
    void* field_3D0 = nullptr;                      // 0x3D0 owned buffer
    camera::DefaultCameraMode* cameraMode = nullptr; // 0x4A0 holds cameraSettings as an override
    camera::CameraSettings cameraSettings;          // 0x4B0
    ape::ApeTArray<void*> field_530;                // 0x530
};

// Character definition (0x5A0 bytes on the console). The tunable blocks are
// kept opaque; the camera block holds the CameraSettings the follow camera
// takes (+0x3F0).
class CharacterDef : public ScriptedEntityDef {
public:
    ~CharacterDef() override;
    // Definition slot 2: the player name when set, else the definition name.
    // NOTE: the slot is not declared on EntityDef yet.
    const char* GetDisplayName() const;
    // Linked definitions (up to four, null terminated) (0x00121B40 / 0x00121B68).
    EntityDef* LinkedDef(uint32_t index) const;
    int FindLinkedDef(const EntityDef* linked) const;

    TunableBlock block_F0;          // 0x0F0
    TunableBlock block_2D0;         // 0x2D0
    TunableBlock block_304;         // 0x304
    TunableBlock cameraBlock;       // 0x3E0 (settings at +0x10)
    camera::CameraSettings cameraSettings;   // 0x3F0
    TunableBlock block_470;         // 0x470
    TunableBlock block_510;         // 0x510
    char* playerName = nullptr;     // 0x588 owned ("playerName")
    EntityDef* linkedDefs[4] = {};  // 0x58C NOTE: what links them is not known
};

} // namespace entity
} // namespace sly
