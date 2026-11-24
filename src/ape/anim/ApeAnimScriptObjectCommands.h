// openTIT - Built-in anim script commands that act on world objects.
//
// These commands are parsed by the script module but carry out their work
// through the entity, render and GUI systems. Only the data side (XML
// parsing, variable handling) is ported so far; where the effect needs a
// system outside this module the Execute() is still a TODO.
//
// Object operands are given either as a name ("object", hashed) or as a
// ready-made hash ("ObjectHash"); they name an object variable of the
// running instance.
#pragma once

#include "ape/anim/ApeAnimScript.h"
#include "ape/math/ApeMath.h"

namespace ape {

// "SwapDef": give an object another definition.
class ApeAnimScriptSwapDefCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptSwapDefCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    uint32_t m_def = 0;           // "def": object variable holding the new def
    bool m_cloneDef = false;      // "CloneDef"
};

// "SetObject": bind an object variable to another object (or one of its parts).
class ApeAnimScriptSetObjectCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptSetObjectCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_target = 0;        // "Target"/"TargetHash": variable to set
    uint32_t m_object = 0;
    uint32_t m_part = 0;          // "part": named sub object
    // NOTE: attribute 0x6334AE6F (name unknown): look the object up in the world.
    bool m_fromWorld = false;
};

// "SpawnObject".
class ApeAnimScriptSpawnObjectCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptSpawnObjectCommand() = default;
    explicit ApeAnimScriptSpawnObjectCommand(xml::XmlElement* element);
    ~ApeAnimScriptSpawnObjectCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_def = 0;           // "def"/"defHash"
    uint32_t m_object = 0;
    uint32_t m_target = 0;        // "Target"/"TargetHash"
    uint32_t m_part = 0;          // "part"
    uint32_t m_locator = 0;       // "Locator"/0xABA3F684
    uint32_t m_field20 = 0;
    uint32_t m_field24 = 0;
    bool m_cloneDef = false;      // "CloneDef"
    bool m_hideDef = false;       // "HideDef"
    bool m_translationOnly = false; // "TranslationOnly"
    bool m_inheritScale = false;  // "InheritScale"
    bool m_flag11605B48 = false;  // NOTE: name unknown
    uint8_t m_visible = 2;        // "visible": 0/1, 2 = keep the def's setting
};

// "SetVisible".
class ApeAnimScriptSetVisibleCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptSetVisibleCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    uint32_t m_part = 0;          // "part"
    bool m_visible = false;       // "visible"
    bool m_flagF5897C08 = false;  // NOTE: name unknown (also used by other commands)
    bool m_fromWorld = false;     // attribute 0x6334AE6F
    bool m_setDirty = false;      // "SetDirty"
};

// "SetFOV": camera field of view.
class ApeAnimScriptSetFovCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptSetFovCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    float m_fov = 0.0f;           // "FOV"
};

// Command 0x17CD0663 (name unknown): toggles an occlusion plane.
class ApeAnimScriptOcclusionPlaneCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptOcclusionPlaneCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_plane = 0;         // "occlPlane" (or int 0x3F4EC4AD)
    bool m_active = false;        // "active"
};

// Command 0xCE800E6C (name unknown): enables or disables an object.
class ApeAnimScriptEnableObjectCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptEnableObjectCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    bool m_enabled = false;       // "enabled"
    bool m_flagF5897C08 = false;
};

// Command 0x2EAE8975 (name unknown): drives an object from a variable at a rate.
class ApeAnimScriptObjectRateCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptObjectRateCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    float m_rate = 0.0f;          // "rate"
    uint32_t m_var = 0;           // "var"
    bool m_flagF5897C08 = false;
    bool m_fromWorld = false;     // attribute 0x6334AE6F
};

// "SetDeflector".
class ApeAnimScriptSetDeflectorCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptSetDeflectorCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    float m_planeDist = 0.0f;     // "PlaneDist"
    float m_bounce = 1.0f;        // "bounce"
};

// "Rumble": pad vibration.
class ApeAnimScriptRumbleCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptRumbleCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_controller = 0;    // "controller": controller variable
    float m_amplitude = 0.0f;     // "Amplitude"
    float m_decay = 0.0f;         // "Decay"
};

// "SetColor": tint an object or one of its materials.
class ApeAnimScriptSetColorCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptSetColorCommand() = default;
    explicit ApeAnimScriptSetColorCommand(xml::XmlElement* element);
    ~ApeAnimScriptSetColorCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;        // "object"/"ObjectHash"
    uint32_t m_material = 0;      // "Material"/"materialHash"
    uint8_t m_color[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // "Color"
    bool m_add = false;           // "add"
    uint32_t m_colorVar[4] = {};  // per-channel variables (hashes 0x28162B, 0x281620, 0x28161B, 0x28161A)
};

// "EnableProcAnim".
class ApeAnimScriptEnableProcAnimCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptEnableProcAnimCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    bool m_flag54FEB607 = false;  // NOTE: name unknown
    bool m_enable = false;        // "enable"
    bool m_hasEnable = false;     // "enable" was given
    bool m_resetTime = false;     // "ResetTime"
};

// "SetScale".
class ApeAnimScriptSetScaleCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptSetScaleCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    Vec3 m_scale = {1.0f, 1.0f, 1.0f, 0.0f};  // "Scale" (vec3 or uniform float)
    bool m_flagF5897C08 = true;
};

// "SetMatrixVar": copy an object's world transform into a matrix variable.
class ApeAnimScriptSetMatrixVarCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptSetMatrixVarCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_variable = 0;      // "variable"
    uint32_t m_object = 0;        // "object"
};

// "InterpScale": scale an object from one value to another over time.
class ApeAnimScriptInterpScaleCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptInterpScaleCommand() = default;
    explicit ApeAnimScriptInterpScaleCommand(xml::XmlElement* element);
    ~ApeAnimScriptInterpScaleCommand() override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    Vec3 m_from = {1.0f, 1.0f, 1.0f, 0.0f};  // "From"
    uint32_t m_fromVar = 0;                  // "FromVar"
    Vec3 m_to = {1.0f, 1.0f, 1.0f, 0.0f};    // "To"
    uint32_t m_toVar = 0;                    // "ToVar"
    float m_duration = 0.0f;                 // "Duration"
    uint32_t m_durationVar = 0;              // "DurationVar"
};

// "CreateProcAnim".
class ApeAnimScriptCreateProcAnimCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptCreateProcAnimCommand() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_object = 0;
    bool m_flag71B29C0F = false;  // NOTE: name unknown
};

// "PlayGuiScript".
class ApeAnimScriptPlayGuiScriptCommand : public ApeAnimScriptCommand {
public:
    ~ApeAnimScriptPlayGuiScriptCommand() override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_script = 0;        // "script"
    uint32_t m_object = 0;        // "object": GUI variable
};

// "AttachObject": parents an object to another one (optionally at a bone
// or locator, with an offset and rotation).
// TODO(0x004B9ED8 ApeAnimScriptAttachObjectCommand::ApeAnimScriptAttachObjectCommand)
// TODO(0x004C30C8 ApeAnimScriptAttachObjectCommand::ApeAnimScriptAttachObjectCommand(xml))
// TODO(0x004C3658 ApeAnimScriptAttachObjectCommand::Create)
// TODO(0x004C36D0 ApeAnimScriptAttachObjectCommand::Execute)
class ApeAnimScriptAttachObjectCommand : public ApeAnimScriptCommand {
public:
    // @func 0x004C3604 ApeAnimScriptAttachObjectCommand::~ApeAnimScriptAttachObjectCommand
    // @func 0x004C3614 ApeAnimScriptAttachObjectCommand::~ApeAnimScriptAttachObjectCommand (deleting)
    ~ApeAnimScriptAttachObjectCommand() override = default;
    // @func 0x004C3DB4 ApeAnimScriptAttachObjectCommand::CreateDebugElement
    xml::XmlElement* CreateDebugElement() const override { return nullptr; }
};

// "MoveObject": moves an object along a path or towards a target.
// TODO(0x004C4708 ApeAnimScriptMoveObjectCommand::ApeAnimScriptMoveObjectCommand(xml))
// TODO(0x004C4D9C ApeAnimScriptMoveObjectCommand::Create)
// TODO(0x004C586C ApeAnimScriptMoveObjectCommand::Execute)
class ApeAnimScriptMoveObjectCommand : public ApeAnimScriptCommand {
public:
    // @func 0x004C4D48 ApeAnimScriptMoveObjectCommand::~ApeAnimScriptMoveObjectCommand
    // @func 0x004C4D58 ApeAnimScriptMoveObjectCommand::~ApeAnimScriptMoveObjectCommand (deleting)
    ~ApeAnimScriptMoveObjectCommand() override = default;
    // @func 0x004C5BA0 ApeAnimScriptMoveObjectCommand::CreateDebugElement
    xml::XmlElement* CreateDebugElement() const override { return nullptr; }
};

// "SetPostProc": changes the post-processing effects.
// TODO(0x004B9CD8 ApeAnimScriptSetPostProcCommand::ApeAnimScriptSetPostProcCommand)
// TODO(0x004CCF28 ApeAnimScriptSetPostProcCommand::ApeAnimScriptSetPostProcCommand(xml))
// TODO(0x004CDC94 ApeAnimScriptSetPostProcCommand::Create)
// TODO(0x004CEF20 ApeAnimScriptSetPostProcCommand::Execute)
class ApeAnimScriptSetPostProcCommand : public ApeAnimScriptCommand {
public:
    // @func 0x004CDC34 ApeAnimScriptSetPostProcCommand::~ApeAnimScriptSetPostProcCommand
    // @func 0x004CDC44 ApeAnimScriptSetPostProcCommand::~ApeAnimScriptSetPostProcCommand (deleting)
    ~ApeAnimScriptSetPostProcCommand() override = default;
};

// Hooks into the entity system used by the commands above.
struct AnimScriptObjectHooks {
    // Sub object `partHash` of `object` (borrowed) or null.
    ApeRefCounted* (*findPart)(ApeRefCounted* object, uint32_t partHash) = nullptr;
    // World transform of `object`; false if it has none.
    bool (*getWorldTransform)(ApeRefCounted* object, Matrix34& out) = nullptr;
    // Object used by SetMatrixVar when its object is not found.
    ApeRefCounted* (*defaultTransformObject)() = nullptr;
};
AnimScriptObjectHooks& GetAnimScriptObjectHooks();

// Reads an object operand: "object" as a name, else "ObjectHash" as an int.
uint32_t ReadObjectOperand(xml::XmlElement* element);

} // namespace ape
