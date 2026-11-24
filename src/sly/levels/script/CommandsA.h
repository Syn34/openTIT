// openTIT - Game script commands, first alphabetical block (Activate... to Attach...).
//
// Commands are registered under the hash of their XML element name. Where the
// name could not be recovered the class is named after what it does and the
// registered hash is given in the comment.
#pragma once

#include "sly/levels/script/ScriptCommand.h"

namespace sly {
namespace levels {

// Hash 0x36DF0092 (name unknown, sorts before "ActivateMixState"): turns the
// interaction component of an entity on or off.
class ActivateInteractionCommand : public ApeAnimScriptCommand {
public:
    ActivateInteractionCommand();
    explicit ActivateInteractionCommand(XmlElement* element);
    ~ActivateInteractionCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;     // +0x0C "RefObject"
    uint32_t m_entity = 0;        // +0x10 "Entity"
    bool m_activate = true;       // +0x14 "activate"
    bool m_allowCancel = true;    // +0x15 "AllowCancel"
    bool m_forcePlayer = false;   // +0x16 "ForcePlayer"
};

// "ActivateMixState": push or remove an audio mix state.
class ActivateMixStateCommand : public ApeAnimScriptCommand {
public:
    ActivateMixStateCommand();
    explicit ActivateMixStateCommand(XmlElement* element);
    ~ActivateMixStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_activate = true;                         // +0x0C "activate"
    uint32_t m_mixStateName = 0;                    // +0x10 "MixState"
    const audio::MixState* m_mixState = nullptr;    // +0x14
    float m_crossfadeDuration = 0.0f;               // +0x18 "CrossfadeDuration"
};

// Hash 0x37A78BFD (name unknown): gives a part/material of an object to the
// player's scanner component.
class ObjectMaterialCommand : public ApeAnimScriptCommand {
public:
    ObjectMaterialCommand();
    ~ObjectMaterialCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;    // +0x0C "Entity"
    uint32_t m_object = 0;    // +0x10 "Object"
    uint32_t m_part = 0;      // +0x14 "part"
    uint32_t m_material = 0;  // +0x18 "Material"
};

// Hash 0xB09EA328 (name unknown): blends an animation layer weight from
// "From" to "To" over the state time.
class AnimLayerBlendCommand : public ApeAnimScriptCommand {
public:
    AnimLayerBlendCommand();
    ~AnimLayerBlendCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;        // +0x0C "Object"
    uint32_t m_entity = 0;        // +0x10 "Entity"
    uint32_t m_durationVar = 0;   // +0x14 "DurationVar"
    float m_duration = 0.0f;      // +0x18 "duration"
    float m_from = 0.0f;          // +0x1C "From"
    float m_to = 1.0f;            // +0x20 "To"
    int32_t m_animLayer = 0;      // +0x24 "AnimLayer"
};

// Hash 0x794FCBAC (name unknown): sets the clock of an animation layer.
class AnimLayerTimeCommand : public ApeAnimScriptCommand {
public:
    AnimLayerTimeCommand();
    ~AnimLayerTimeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;        // +0x0C "Object"
    uint32_t m_animTimeVar = 0;   // +0x10 "AnimTimeVar"
    float m_animTime = 0.0f;      // +0x14 "AnimTime"
    int32_t m_animLayer = 0;      // +0x18 "AnimLayer"
    bool m_scaleByLength = true;  // +0x1C NOTE: attribute 0x2FCB2115; scales the time by the clip length
};

// Hash 0xC495712B (name unknown): no parameters; switches a global game
// system on (0x002E4A0C on the object at 0x00F64C20).
class GlobalToggleCommand : public ApeAnimScriptCommand {
public:
    GlobalToggleCommand();
    ~GlobalToggleCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// "AttachCarryItem" (hash 0x81E249E9; NOTE: name from a hash match): attaches
// a character to another entity, optionally at a part with an offset/rotation.
class AttachCarryItemCommand : public ApeAnimScriptCommand {
public:
    AttachCarryItemCommand();
    explicit AttachCarryItemCommand(XmlElement* element);
    ~AttachCarryItemCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    ape::Vec3 m_offset = {0.0f, 0.0f, 0.0f, 0.0f};      // +0x10 "AttachOffset"
    ape::Vec4 m_rotation = {0.0f, 0.0f, 0.0f, 1.0f};    // +0x20 "AttachRot" (quaternion)
    uint32_t m_attachPart = 0;                          // +0x30 "AttachPart"
    uint32_t m_attacheeVar = 0;                         // +0x34 "AttacheeVar"
    uint32_t m_attacheeEntity = 0;                      // +0x38 "AttacheeEntity"
    uint32_t m_targetVar = 0;                           // +0x3C attribute 0x5538A9B9
    uint32_t m_targetEntity = 0;                        // +0x40 attribute 0x30A5A3F1
    bool m_hasPart = false;                             // +0x44
    bool m_hasOffset = false;                           // +0x45
    bool m_hasRotation = false;                         // +0x46
    bool m_ignoreRot = true;                            // +0x47 "IgnoreRot"
};

// "AttachEntity": parents one entity to another, with offsets for both ends.
class AttachEntityCommand : public ApeAnimScriptCommand {
public:
    AttachEntityCommand();
    explicit AttachEntityCommand(XmlElement* element);
    ~AttachEntityCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_parent = 0;                              // +0x0C "Parent"
    uint32_t m_child = 0;                               // +0x10 "Child"
    uint32_t m_parentPart = 0;                          // +0x14 attribute 0x20C00073
    ape::Vec3 m_parentOffset = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x20 attribute 0xEF4DDD13
    ape::Vec3 m_parentRotation = {0.0f, 0.0f, 0.0f, 0.0f};// +0x30 attribute 0xC7400937 (degrees in data)
    uint32_t m_childPart = 0;                           // +0x40 attribute 0xF91B96E2
    ape::Vec3 m_childOffset = {0.0f, 0.0f, 0.0f, 0.0f};   // +0x50 attribute 0x1F1D93C2
    ape::Vec3 m_childRotation = {0.0f, 0.0f, 0.0f, 0.0f}; // +0x60 attribute 0xA4F068E8 (degrees in data)
    float m_gravity = -1.0f;                            // +0x70 "Gravity"
    bool m_syncAnimation = false;                       // +0x74 "SyncAnimation"
    bool m_flag75 = false;                              // +0x75 attribute 0x3E3F7E94
    bool m_flag76 = false;                              // +0x76 attribute 0xD1A6825E
    bool m_flag77 = false;                              // +0x77 attribute 0xDC623C3E
    bool m_hasGravity = false;                          // +0x78
    bool m_flag79 = false;                              // +0x79 attribute 0x3B757A97
};

// "Blast": radial damage and impulse around an object.
class BlastCommand : public ApeAnimScriptCommand {
public:
    BlastCommand();
    explicit BlastCommand(XmlElement* element);
    ~BlastCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

    // Damage types that can be switched off in a "DamageTypes" child.
    static constexpr int kDamageTypeCount = 13;

private:
    uint32_t m_refObject = 0;                           // +0x0C "RefObject"
    ape::Vec3 m_offsetPos = {0.0f, 0.0f, 0.0f, 0.0f};   // +0x10 "OffsetPos"
    ape::Vec3 m_direction = {0.0f, 0.0f, 0.0f, 0.0f};   // +0x20 attribute 0x8C3BE0FF
    float m_radiusMin = 0.0f;                           // +0x30 "RadiusMin"
    float m_radiusMax = 0.0f;                           // +0x34 "RadiusMax"
    bool m_ignoreHeight = false;                        // +0x38 "IgnoreHeight"
    float m_damage = 0.0f;                              // +0x3C "damage"
    float m_impulse = 0.0f;                             // +0x40 "Impulse"
    uint32_t m_reactType = 0;                           // +0x44 "ReactType"
    uint32_t m_damageTypeMask = 0;                      // +0x48
    int32_t m_field4C = -1;                             // +0x4C attribute 0xC831C860, stored minus one
};

// "CameraClearEase" (hash 0xC04DBE16; NOTE: name from a hash match).
class CameraClearEaseCommand : public ApeAnimScriptCommand {
public:
    CameraClearEaseCommand();
    ~CameraClearEaseCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// Hash 0xD1F7C236 (name unknown): plays an animation on the game camera.
class CameraPlayAnimCommand : public ApeAnimScriptCommand {
public:
    CameraPlayAnimCommand();
    ~CameraPlayAnimCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_anim = 0;      // +0x0C "Anim"
    float m_speed = 1.0f;     // +0x10 "speed"
    bool m_loop = false;      // +0x14 "Loop"
    bool m_flag15 = false;    // +0x15 attribute 0x855E65C3
};

// "CameraShake".
class CameraShakeCommand : public ApeAnimScriptCommand {
public:
    CameraShakeCommand();
    explicit CameraShakeCommand(XmlElement* element);
    ~CameraShakeCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;            // +0x0C "RefObject"
    float m_field10 = 0.0f;              // +0x10 NOTE: never read from data
    float m_duration = 1.0f;             // +0x14 "duration"
    float m_magnitude = 1.0f;            // +0x18 "Magnitude"
    float m_startMagnitude = 0.0f;       // +0x1C "StartMagnitude"
    uint32_t m_falloffMode = 0x85D98325u;// +0x20 "FalloffMode" (default hash: NOTE name unknown)
    float m_falloffStartTime = 0.0f;     // +0x24 "FalloffStartTime"
    float m_pitch = 1.0f;                // +0x28 "Pitch"
    float m_yaw = 0.0f;                  // +0x2C "Yaw"
    float m_roll = 1.0f;                 // +0x30 "Roll"
    float m_pitchFrequency = 4.0f;       // +0x34 "PitchFrequency"
    float m_yawFrequency = 6.5f;         // +0x38 "YawFrequency"
    float m_rollFrequency = 2.5f;        // +0x3C "RollFrequency"
    float m_radius = 0.0f;               // +0x40 "Radius"
    ape::Vec3 m_origin = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x50 shake origin (RefObject position, set on Execute)
    float m_field60 = -1.0f;             // +0x60 NOTE: never read from data
};

// Hash 0x3D0E7A04 (name unknown): stops the game camera's animation.
class CameraStopAnimCommand : public ApeAnimScriptCommand {
public:
    CameraStopAnimCommand();
    ~CameraStopAnimCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_removeAnimation = false;  // +0x0C "RemoveAnimation"
};

// "CloseBinoccucom" (sic, hash 0x7F37BF6C): closes the player's binocucom gadget.
class CloseBinocucomCommand : public ApeAnimScriptCommand {
public:
    CloseBinocucomCommand();
    ~CloseBinocucomCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// "CompleteToGoal" (hash 0xF7635E87; NOTE: name from a hash match): marks a
// goal as completed.
class CompleteGoalCommand : public ApeAnimScriptCommand {
public:
    CompleteGoalCommand();
    ~CompleteGoalCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_goal = 0;      // +0x0C "Goal"
    uint32_t m_goalVar = 0;   // +0x10 "GoalVar": int variable with the goal name
};

// Hash 0xB8B01DB7 (name unknown; a match "CharSpecialCmd" exists but does not
// fit the alphabetical registration order): sends a special command to a
// character. The parameter at +0x20 depends on the command.
class CharacterSpecialCommand : public ApeAnimScriptCommand {
public:
    CharacterSpecialCommand();
    explicit CharacterSpecialCommand(XmlElement* element);
    ~CharacterSpecialCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

    // Values of "Command" (names unknown).
    static constexpr uint32_t kCommand_Direction = 0x76A67631u;  // reads "dir"/"direction"
    static constexpr uint32_t kCommand_Angle = 0x66B98D77u;      // reads "Angle"
    static constexpr uint32_t kCommand_Enable = 0x09AED71Du;     // reads "enable"
    static constexpr uint32_t kCommand_SkipAnim = 0xC15C9D78u;   // reads "SkipAnim"

private:
    uint32_t m_object = 0;      // +0x0C "object"
    uint32_t m_entity = 0;      // +0x10 "Entity"
    uint32_t m_command = 0;     // +0x14 "Command"
    // +0x20: direction for kCommand_Direction, else the flag in its first byte.
    ape::Vec3 m_direction = {0.0f, 0.0f, 0.0f, 0.0f};
    bool m_flag = true;
    float m_angle = 0.0f;       // +0x30 "Angle"
};

// Hash 0xCB291F57 (name unknown): passes an entity's name to the game state
// (0x002E2B34 on the object at 0x00F64C20).
class EntityGameStateCommand : public ApeAnimScriptCommand {
public:
    EntityGameStateCommand();
    ~EntityGameStateCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
};

// Hash 0xA1D6F663 (name unknown): clears the halfword +0x2C of the game
// object at 0x00F64C38.
class ClearCounterCommand : public ApeAnimScriptCommand {
public:
    ClearCounterCommand();
    ~ClearCounterCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// "Crossfade": fades an object's materials to replacement materials.
class CrossfadeCommand : public ApeAnimScriptCommand {
public:
    CrossfadeCommand();
    explicit CrossfadeCommand(XmlElement* element);
    ~CrossfadeCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

    static constexpr int kMaxChildren = 4;

private:
    using MaterialMap = ape::AVLMap<uint32_t, uint32_t>;  // "From" -> "To"
    struct ChildRemap {
        uint32_t name = 0;          // "Name" of the child entity
        MaterialMap materials;
    };

    MaterialMap m_materials;                   // +0x0C "Material" children
    ChildRemap m_children[kMaxChildren];       // "EntityChild" children
    uint32_t m_object = 0;                     // +0x224 "object"
    uint32_t m_defaultMat = 0;                 // +0x228 "DefaultMat"
    uint32_t m_field22C = 0;                   // +0x22C NOTE: runtime only
    uint32_t m_field230 = 0;                   // +0x230 attribute 0x11CCCBF1
    float m_duration = 0.0f;                   // +0x234 "duration"
    uint8_t m_fromAlpha = 0xFF;                // +0x238 "FromAlpha"
    uint8_t m_toAlpha = 0xFF;                  // +0x239 "ToAlpha"
    uint8_t m_childCount = 0;                  // +0x23A
    bool m_flag23B = true;                     // +0x23B attribute 0x16C2F884
    bool m_flag23C = false;                    // +0x23C attribute 0xF9D05ED8
    bool m_flag23D = false;                    // +0x23D attribute 0xC1DF2672
};

// "Damage": damages an entity.
class DamageCommand : public ApeAnimScriptCommand {
public:
    DamageCommand();
    ~DamageCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;          // +0x0C "object"
    float m_damage = 0.0f;          // +0x10 "damage"
    float m_knockbackImpulse = 0.0f;// +0x14 "KnockbackImpulse"
    uint32_t m_type = 0;            // +0x18 "Type"
};

// "DetachEntity": undoes AttachEntity.
class DetachEntityCommand : public ApeAnimScriptCommand {
public:
    DetachEntityCommand();
    ~DetachEntityCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_parent = 0;  // +0x0C "Parent": object variable or entity name
    uint32_t m_child = 0;   // +0x10 "Child": object variable or entity name
};

} // namespace levels
} // namespace sly
