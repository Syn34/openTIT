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
#include "sly/entity/Abilities.h"
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

// Something that watches a character (Character +0x530); slot 75 (+0x12C)
// takes the character off its list.
class CharacterWatcher {
public:
    virtual ~CharacterWatcher() = default;
    virtual void ForgetCharacter(class Character* character) = 0;
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

    // Bits of flags3C8.
    enum Flags3C8 : uint8_t {
        kFlag3C8_NoCostume  = 0x01,   // profile outfits are not applied
        kFlag3C8_Bit1       = 0x02,
        kFlag3C8_Visible    = 0x04,   // copy of the visibility while fading
        kFlag3C8_Opaque     = 0x08,   // forced opaque
        kFlag3C8_FadeBuffer = 0x10,   // the fade buffer must be pushed
        kFlag3C8_FadeB      = 0x20,
        kFlag3C8_FadeA      = 0x40,
    };
    // Queues the world callback that puts a fading character back on the
    // fade list once it is gone from the update lists.
    void QueueFadeRequeue();

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

    // -- 0x0011F000-0x0012A000 (CharacterMethods.cpp) ---------------------------
    // Slot 3 of the definition aside, these are the character's own helpers
    // and its overrides of entity slots 25-38 and 83-88.

    // Model variants (skinMode): the definition carries an alternate model
    // per variant; the class bits pick one (0x0011F0E4).
    enum SkinMode : int32_t {
        kSkin_Default = 0, kSkin_Variant1 = 1, kSkin_Variant2 = 2,
        kSkin_Variant3 = 3, kSkin_Variant4 = 4, kSkin_Variant5 = 5,
    };
    // Picks the model for the class bits and switches the character
    // component to it, optionally resetting the pose first.
    void UpdateSkin(bool resetPose);
    // Entity attached to the weapon part, if any (0x0011F2C8).
    Entity* WeaponEntity() const;
    // Replaces the attached weapon with a new one spawned from `defName` (0x0011F32C).
    void AttachWeapon(uint32_t defName);
    // Applies the costume, weapon and outfit selections of the player's
    // profile (0x0011F7B8). `force` swaps the model even while busy.
    void ApplyProfileCostume(bool force);
    // Finds the aim UI objects named by the definition (0x0011FCC0).
    void FindAimUiObjects();
    // Moves the current tint toward the target tint by `t` (0..1) and pushes
    // it to the model and the attached props when it changed (0x0011FFF0).
    void UpdateTint(float t, bool force);
    // Slot 30: tint toward `color`/`amount` (immediately with `immediate`);
    // passed on to the attached children (0x00120338).
    void SetTint(float amount, const uint8_t color[4], bool immediate) override;
    // Hooks the animation controller up to the character component, picks
    // the model and refreshes component 0x11 (0x00120490).
    void InitAnimation();
    // Rebuilds the ability states from the definition, the level's unlocks,
    // the purchases and the costume (0x001205FC).
    void RefreshAbilities();
    // Character slots 10/11.
    bool Release(bool kill) override;                // 0x00122518
    // Slot 11: back to the idle state with the physics and props reset
    // (0x00122830). NOTE: the entity base slot (empty) is not declared on Entity.
    virtual void ResetState();
    // Takes `entity` into the hands (null drops the current one): attached at
    // `part` (definition default when null) with `offset`/`rotation` (the
    // definition's carry offset and Euler angles when null) (0x00121FA8).
    void SetCarried(Entity* entity, const uint32_t* part, const ape::Vec3* offset,
                    const ape::Vec4* rotation, bool parentRotation);
    // Drops the carried entity; its listener is told first, without one the
    // entity is released when `release` (0x001244A4).
    void DropCarried(bool release);
    // Pushes the fade buffer to the model and the fading children (0x00123F70).
    void ApplyFadeBuffer();
    // Back to full opacity after a scripted fade (0x001243D0).
    void ResetFade();
    void SetActive(bool active) override;            // slot 44 (0x00124598)
    void SetRenderId(uint32_t id) override;          // slot 26 (0x00124A04)

    // Slot 13: follows the root animation's motion (0x0012852C).
    // NOTE: the entity base slot (0x000D8954) is not declared on Entity.
    virtual void ApplyRootMotion();
    void* FindAnimClip(uint32_t nameHash) override;          // slot 47
    void* FindAnimClipByName(const char* name) override;     // slot 48
    // Slots 72/73: axes of the surface the physics component moves on
    // (ground, water, wall). NOTE: names from the values returned.
    virtual ape::Vec3 GetUpVector();
    virtual ape::Vec3 GetSideVector();
    uint32_t InteractionOffer(int kind, Entity* other) override;   // slot 74
    bool CanInteractWith(Entity* other, int kind) override;        // slot 76

    // Starts the director script `scriptName` of the default library as the
    // character's script (+0x198), ending the current one first when a new
    // one starts or `stopCurrent` (0x00128C74).
    ape::ApeAnimScriptInstance* PlayCharacterScript(uint32_t scriptName, bool stopCurrent);
    // Swaps the character to another definition (0x00124D58, not ported).
    void SwapCharacter(EntityDef* newDef, bool keepState);
    // 0x00125950 / 0x00125998.
    void BecomeSly();
    void EndDisguise();
    void SetVisible(bool visible, bool immediate, bool children, bool components) override;   // slot 22

    // Hit reactions (slots 83-87) and damage (slot 88).
    void OnHitCategory0(EntityHitRecord& hit) override;   // physics impact
    void OnHitCategory1(EntityHitRecord& hit) override;   // melee
    void OnHitCategory2(EntityHitRecord& hit) override;   // blast
    void OnHitCategory3(EntityHitRecord& hit) override;   // environment
    void OnHitCategory4(EntityHitRecord& hit) override;   // projectile
    bool Damage(float amount, Entity* source, uint32_t type) override;
    // Friendly-fire test of the hit reactions.
    bool IsFriendlyHit(const Entity* attacker) const;
    // Applies a hit's damage, knockback and reaction (0x00125A80, not ported).
    void ApplyDamage(float amount, Entity* attacker, EntityHitRecord& hit, uint32_t type);

    // Counted references at +0x1A4 / +0x1AC (0x00121E8C / 0x00121F10).
    // NOTE: roles unknown; +0x1AC is set by the level AI.
    void SetReference1A4(Entity* entity);
    void SetReference1AC(Entity* entity);

    uint32_t GetRenderId() override;                 // slot 25 (0x00124954)
    // Slot 27: gravity factor of the physics body; while attached, the one
    // the parent's link restores on detach. NOTE: Entity calls slot 28
    // "Detached(blend)"; the value is the gravity factor it hands down.
    virtual float GetGravityFactor();
    void Detached(float gravityFactor) override;     // slot 28 (0x00124C94)
    // Slot 35: the position the character aims from (0x0012905C).
    virtual ape::Vec3 GetAimPosition();
    // Slot 38: turn rate of the character component (0x001290F0).
    virtual float GetTurnRate();

    // NOTE: takes the value of `controller` when a player character is
    // activated; role otherwise unknown.
    CharacterController* field_17C = nullptr;       // 0x17C
    CharacterController* controller = nullptr;      // 0x180 the one in charge (may alias the others)
    CharacterController* aiController = nullptr;    // 0x184 "ControlAI"
    CharacterController* extController = nullptr;   // 0x188 "TmpExtControl"
    uint32_t field_18C[3] = {};                     // 0x18C cleared on release
    void* animScript198 = nullptr;                  // 0x198 orphaned on release
    void* animScript19C = nullptr;                  // 0x19C
    Entity* reference1A4 = nullptr;                 // 0x1A4 counted
    Entity* carried = nullptr;                      // 0x1A8 entity held in the hands (counted)
    Entity* reference1AC = nullptr;                 // 0x1AC counted
    int32_t skinMode = kSkin_Default;               // 0x1B0 SkinMode
    uint32_t field_1B4 = 0;                         // 0x1B4 cleared by ResetState
    uint32_t field_1B8 = 0;                         // 0x1B8 cleared by ResetState / ResetFade
    ape::world::ApeObj* aimArcObject = nullptr;     // 0x1E4 aim UI arc
    ape::world::ApeObj* aimEndObject = nullptr;     // 0x1E8 aim UI end point
    DamageSettings damage;                          // 0x1EC
    uint8_t tintColor[4] = {};                      // 0x220 current
    uint8_t tintTargetColor[4] = {};                // 0x224
    float tintAmount = 0.0f;                        // 0x228 current
    float tintTargetAmount = 0.0f;                  // 0x22C
    uint8_t opacityColor[4] = {};                   // 0x230 tint applied with the opacity (alpha in [3])
    AbilitySet abilities;                           // 0x290
    int16_t field_300 = -1;                         // 0x300 NOTE: reset with the abilities
    int16_t field_308 = -1;                         // 0x308
    int16_t field_310 = -1;                         // 0x310
    void* animScripts[8] = {};                      // 0x31C (stride 0x10 on the console)
    // 0x394 NOTE: built with capacity 2 and its own comparator (0x00F32D90).
    ape::ApeTSortedMap<uint32_t> field_394;
    audio::SoundName* sounds[7] = {};               // 0x3AC
    uint8_t flags3C8 = 0;                           // 0x3C8 Flags3C8
    uint8_t field_3C9 = 0;                          // 0x3C9
    void* fadeBuffer = nullptr;                     // 0x3D0 owned per-part alpha buffer
    int32_t lodField = 0;                           // 0x3E0 handed to the render object
    uint8_t opacity = 0xFF;                         // 0x3E4
    camera::DefaultCameraMode* cameraMode = nullptr; // 0x4A0 holds cameraSettings as an override
    camera::CameraSettings cameraSettings;          // 0x4B0
    // 0x530 objects watching the character; each removes itself when told
    // the character left (their slot 75).
    ape::ApeTArray<class CharacterWatcher*> watchers530;
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

    // Character bounds from the physics component description, else the
    // entity default (slot 3, 0x0011F010; also AI characters' slot 3).
    // NOTE: the slot is not declared on EntityDef yet.
    void GetBounds(ape::Aabb& out) const;
    // Alternate model of a skin mode (variants 1-5), 0 for the default.
    uint32_t SkinModel(int mode) const;

    uint32_t characterName = 0;     // 0x0C0 "Sly", "Bentley", "Murray", ...
    char animPrefix[0x20] = {};     // 0x0C4 prefix of clip names without an animation map
    // 0x0F0 carry block: part (+0x174), offset (+0x180) and Euler angles in
    // degrees (+0x190) of a carried entity, and the drop settings (+0x1C0)
    // handed to the carried entity's listener.
    TunableBlock block_F0;          // 0x0F0
    uint32_t carryPart = 0;         // 0x174
    ape::Vec3 carryOffset{};        // 0x180
    ape::Vec3 carryAngles{};        // 0x190
    uint8_t dropSettings[0x10] = {};   // 0x1C0 NOTE: contents unknown
    TunableBlock block_2D0;         // 0x2D0
    TunableBlock block_304;         // 0x304
    TunableBlock cameraBlock;       // 0x3E0 (settings at +0x10)
    camera::CameraSettings cameraSettings;   // 0x3F0
    TunableBlock block_470;         // 0x470
    TunableBlock block_510;         // 0x510
    uint32_t abilityBits[4] = {};   // 0x550 abilities the character starts with
    // 0x560..0x570: models of skin variants 3, 2, 1, 4, 5 (in that order).
    uint32_t skinModels[5] = {};
    bool forwardRootMotion = false; // 0x580 bit 0 (animation controller flag 0x200). NOTE: name guessed
    bool allowPlayerInteraction = false;   // 0x59C interaction kind 3 allowed
    uint32_t aimArcName = 0;        // 0x5B0 aim UI object names
    uint32_t aimEndName = 0;        // 0x5B4
    char* playerName = nullptr;     // 0x588 owned ("playerName")
    EntityDef* linkedDefs[4] = {};  // 0x58C NOTE: what links them is not known
};

} // namespace entity
} // namespace sly
