// openTIT - Game script commands, third alphabetical block (Entity... to FireProjectile).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"

namespace sly {
namespace levels {

using ape::xml::Color8;

namespace {
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrVar = AnimHash("var");
constexpr uint32_t kAttrState = AnimHash("state");
constexpr uint32_t kAttrEnabled = AnimHash("Enabled");
constexpr uint32_t kAttrDeferred = AnimHash("Deferred");
constexpr uint32_t kAttrF5897C08 = 0xF5897C08u;    // NOTE: name unknown (also seen in the engine scripts)
constexpr uint32_t kAttrAllowRepeat = AnimHash("AllowRepeat");
constexpr uint32_t kAttrNoTarget = AnimHash("NoTarget");
constexpr uint32_t kAttrVisibleCD9F = 0xCD9F562Bu;  // NOTE: name unknown
constexpr uint32_t kAttrProcessComponents = AnimHash("ProcessComponents");
constexpr uint32_t kAttrVisible = AnimHash("visible");
constexpr uint32_t kAttrCollectInfo14 = 0x9752205Bu;
constexpr uint32_t kAttrCollectInfo18 = 0x94DB5F0Eu;
constexpr uint32_t kAttrCollectInfo1C = 0x385F3FA3u;
constexpr uint32_t kAttrCollectInfo20 = 0xC830A35Du;
constexpr uint32_t kAttrCollectInfo24 = 0x6EC19EE9u;
constexpr uint32_t kAttrDesaturationTarget = 0x89E68B87u;
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrDuration = AnimHash("duration");
constexpr uint32_t kAttrIntensityStart = AnimHash("IntensityStart");
constexpr uint32_t kAttrIntensityEnd = AnimHash("IntensityEnd");
constexpr uint32_t kAttrColorStart = 0xF7E02627u;
constexpr uint32_t kAttrColorEnd = 0xDC8F9BE0u;
constexpr uint32_t kAttrBlendMode = AnimHash("blendmode");
constexpr uint32_t kAttrDesaturationType = AnimHash("DesaturationType");
constexpr uint32_t kAttrMaterial = AnimHash("Material");
constexpr uint32_t kAttrInterpVar = AnimHash("InterpVar");
constexpr uint32_t kAttrFrom = AnimHash("From");
constexpr uint32_t kAttrTo = AnimHash("To");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrProjectileAA44 = 0xAA44DED3u;
constexpr uint32_t kAttrProjectile8C63 = 0x8C63F207u;
constexpr uint32_t kAttrOffset = AnimHash("Offset");
constexpr uint32_t kAttrProjectileFDC3 = 0x0FDC3B3Eu;
constexpr uint32_t kAttrTgtObject = AnimHash("TgtObject");
constexpr uint32_t kAttrTgtEntity = AnimHash("TgtEntity");
constexpr uint32_t kAttrTgtPart = AnimHash("TgtPart");
constexpr uint32_t kAttrTgtOffset = AnimHash("TgtOffset");
constexpr uint32_t kAttrProjectileDef = AnimHash("ProjectileDef");
constexpr uint32_t kAttrSpeed = AnimHash("speed");
constexpr uint32_t kAttrLaunchAngle = AnimHash("LaunchAngle");
constexpr uint32_t kAttrProjectileEAC0 = 0xEAC031A0u;
constexpr uint32_t kAttrProjectileObj = AnimHash("ProjectileObj");
constexpr uint32_t kAttrLifetimeMax = AnimHash("LifetimeMax");
constexpr uint32_t kAttrProjectile2605 = 0x26055815u;
constexpr uint32_t kAttrProjectile0291 = 0x0291E67Fu;
constexpr uint32_t kAttrProjectile77FB = 0x77FB0D02u;

constexpr float kDegToRad = 0.017453292f;
constexpr float kMinDuration = 1.0e-5f;

// Default fade colours (console globals 0x01164CA0 / 0x01164CA8, filled at
// start-up). NOTE: assumed to be opaque white and opaque black.
constexpr Color8 kDefaultFromColor = {0xFF, 0xFF, 0xFF, 0xFF};
constexpr Color8 kDefaultToColor = {0x00, 0x00, 0x00, 0xFF};

// Blend factor 0..255 from a fraction (the console converts through a
// 64-bit integer and clamps unsigned).
uint32_t BlendFactor(float fraction)
{
    uint32_t factor = static_cast<uint32_t>(static_cast<int64_t>(fraction * 255.0f));
    return factor < 0xFF ? factor : 0xFF;
}

// Per-channel lerp in 8-bit fixed point, with the console's unsigned wrap.
uint8_t LerpChannel(uint8_t from, uint8_t to, uint32_t factor)
{
    return static_cast<uint8_t>(from + static_cast<uint8_t>(((uint32_t)to - (uint32_t)from) * factor >> 8));
}

Color8 LerpColor(const Color8& from, const Color8& to, uint32_t factor)
{
    return {LerpChannel(from.r, to.r, factor), LerpChannel(from.g, to.g, factor),
            LerpChannel(from.b, to.b, factor), LerpChannel(from.a, to.a, factor)};
}
} // namespace

// ---------------------------------------------------------------------------
// "EntityKillFx" (hash 0xDE3DFE53; NOTE: name from a hash match)

class EntityKillFxCommand : public ApeAnimScriptCommand {
public:
    EntityKillFxCommand();
    ~EntityKillFxCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
    uint32_t m_child = 0;   // +0x14 "Child"
};

// @func 0x000744E8 EntityKillFxCommand::EntityKillFxCommand
EntityKillFxCommand::EntityKillFxCommand() = default;

// @func 0x00074538 EntityKillFxCommand::~EntityKillFxCommand
// @func 0x00074568 EntityKillFxCommand::~EntityKillFxCommand (deleting)
EntityKillFxCommand::~EntityKillFxCommand() = default;

// @func 0x000745C0 EntityKillFxCommand::Create
ApeAnimScriptCommand* EntityKillFxCommand::Create(XmlElement* element) const
{
    auto* command = new EntityKillFxCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrChild, &command->m_child, 0);
    element->GetHash(kAttrObject, &command->m_object, 0);
    return command;
}

// TODO(0x000746B0 EntityKillFxCommand::Execute)

// ---------------------------------------------------------------------------
// "EntityGetCollectInfo" (hash 0x3CED7265; NOTE: name from a hash match):
// copies collectable information of an entity into five script variables.

class EntityGetCollectInfoCommand : public ApeAnimScriptCommand {
public:
    EntityGetCollectInfoCommand();
    ~EntityGetCollectInfoCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_var = 0;          // +0x0C "var"
    uint32_t m_entity = 0;       // +0x10 "Entity"
    uint32_t m_outVars[5] = {};  // +0x14.. output variables (attribute names unknown)
};

// @func 0x000747D8 EntityGetCollectInfoCommand::EntityGetCollectInfoCommand
EntityGetCollectInfoCommand::EntityGetCollectInfoCommand() = default;

// @func 0x0073CAB0 EntityGetCollectInfoCommand::~EntityGetCollectInfoCommand
// @func 0x0073CAE0 EntityGetCollectInfoCommand::~EntityGetCollectInfoCommand (deleting)
EntityGetCollectInfoCommand::~EntityGetCollectInfoCommand() = default;

// @func 0x00074838 EntityGetCollectInfoCommand::Create
ApeAnimScriptCommand* EntityGetCollectInfoCommand::Create(XmlElement* element) const
{
    static constexpr uint32_t kOutAttrs[5] = {kAttrCollectInfo14, kAttrCollectInfo18, kAttrCollectInfo1C,
                                              kAttrCollectInfo20, kAttrCollectInfo24};
    auto* command = new EntityGetCollectInfoCommand();
    element->GetHash(kAttrVar, &command->m_var, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    for (int i = 0; i < 5; ++i)
        element->GetHash(kOutAttrs[i], &command->m_outVars[i], command->m_outVars[i]);
    return command;
}

// TODO(0x000749C0 EntityGetCollectInfoCommand::Execute)

// ---------------------------------------------------------------------------
// "EntitySetEnabled" (hash 0xA21A00C2; NOTE: name from a hash match)

class EntitySetEnabledCommand : public ApeAnimScriptCommand {
public:
    EntitySetEnabledCommand();
    ~EntitySetEnabledCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_var = 0;       // +0x0C "var"
    uint32_t m_entity = 0;    // +0x10 "Entity"
    bool m_enabled = false;   // +0x14 "Enabled"
    bool m_deferred = false;  // +0x15 "Deferred"
    bool m_flag16 = false;    // +0x16 attribute 0xF5897C08
};

// @func 0x00074CDC EntitySetEnabledCommand::EntitySetEnabledCommand
EntitySetEnabledCommand::EntitySetEnabledCommand() = default;

// @func 0x0073CB38 EntitySetEnabledCommand::~EntitySetEnabledCommand
// @func 0x0073CB68 EntitySetEnabledCommand::~EntitySetEnabledCommand (deleting)
EntitySetEnabledCommand::~EntitySetEnabledCommand() = default;

// @func 0x00074D34 EntitySetEnabledCommand::Create
ApeAnimScriptCommand* EntitySetEnabledCommand::Create(XmlElement* element) const
{
    auto* command = new EntitySetEnabledCommand();
    element->GetHash(kAttrVar, &command->m_var, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetBool(kAttrEnabled, &command->m_enabled, command->m_enabled);
    element->GetBool(kAttrDeferred, &command->m_deferred, command->m_deferred);
    element->GetBool(kAttrF5897C08, &command->m_flag16, command->m_flag16);
    return command;
}

// TODO(0x00074E70 EntitySetEnabledCommand::Execute)

// ---------------------------------------------------------------------------
// "EntitySetScriptState" (hash 0x53017FA7; NOTE: name from a hash match)

class EntitySetScriptStateCommand : public ApeAnimScriptCommand {
public:
    EntitySetScriptStateCommand();
    ~EntitySetScriptStateCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;   // +0x0C "RefObject"
    uint32_t m_entity = 0;      // +0x10 "Entity"
    uint32_t m_child = 0;       // +0x14 "Child"
    uint32_t m_state = 0;       // +0x18 "state"
    bool m_flag1C = false;      // +0x1C attribute 0xF5897C08
    bool m_allowRepeat = true;  // +0x1D "AllowRepeat"
};

// @func 0x000750DC EntitySetScriptStateCommand::EntitySetScriptStateCommand
EntitySetScriptStateCommand::EntitySetScriptStateCommand() = default;

// @func 0x0073CBC0 EntitySetScriptStateCommand::~EntitySetScriptStateCommand
// @func 0x0073CBF0 EntitySetScriptStateCommand::~EntitySetScriptStateCommand (deleting)
EntitySetScriptStateCommand::~EntitySetScriptStateCommand() = default;

// @func 0x00075144 EntitySetScriptStateCommand::Create
ApeAnimScriptCommand* EntitySetScriptStateCommand::Create(XmlElement* element) const
{
    auto* command = new EntitySetScriptStateCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetHash(kAttrState, &command->m_state, command->m_state);
    element->GetBool(kAttrF5897C08, &command->m_flag1C, command->m_flag1C);
    element->GetBool(kAttrAllowRepeat, &command->m_allowRepeat, command->m_allowRepeat);
    return command;
}

// TODO(0x000752A8 EntitySetScriptStateCommand::Execute)

// ---------------------------------------------------------------------------
// "EntitySetGadgetState" (hash 0x3E5144C6; NOTE: name from a hash match)

class EntitySetGadgetStateCommand : public ApeAnimScriptCommand {
public:
    EntitySetGadgetStateCommand();
    ~EntitySetGadgetStateCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    uint32_t m_entity = 0;     // +0x10 "Entity"
    uint32_t m_child = 0;      // +0x14 "Child"
    uint32_t m_state = 0;      // +0x18 "state"
};

// @func 0x00075424 EntitySetGadgetStateCommand::EntitySetGadgetStateCommand
EntitySetGadgetStateCommand::EntitySetGadgetStateCommand() = default;

// @func 0x0073CC48 EntitySetGadgetStateCommand::~EntitySetGadgetStateCommand
// @func 0x0073CC78 EntitySetGadgetStateCommand::~EntitySetGadgetStateCommand (deleting)
EntitySetGadgetStateCommand::~EntitySetGadgetStateCommand() = default;

// @func 0x0007547C EntitySetGadgetStateCommand::Create
ApeAnimScriptCommand* EntitySetGadgetStateCommand::Create(XmlElement* element) const
{
    auto* command = new EntitySetGadgetStateCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetHash(kAttrState, &command->m_state, command->m_state);
    return command;
}

// TODO(0x0007558C EntitySetGadgetStateCommand::Execute)

// ---------------------------------------------------------------------------
// "EntitySetNoTarget" (hash 0x9D57A511; NOTE: name from a hash match)

class EntitySetNoTargetCommand : public ApeAnimScriptCommand {
public:
    EntitySetNoTargetCommand();
    ~EntitySetNoTargetCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;    // +0x0C "object"
    uint32_t m_entity = 0;    // +0x10 "Entity"
    uint32_t m_child = 0;     // +0x14 "Child"
    bool m_noTarget = false;  // +0x18 "NoTarget"
};

// @func 0x00075710 EntitySetNoTargetCommand::EntitySetNoTargetCommand
EntitySetNoTargetCommand::EntitySetNoTargetCommand() = default;

// @func 0x0073CCD0 EntitySetNoTargetCommand::~EntitySetNoTargetCommand
// @func 0x0073CD00 EntitySetNoTargetCommand::~EntitySetNoTargetCommand (deleting)
EntitySetNoTargetCommand::~EntitySetNoTargetCommand() = default;

// @func 0x00075768 EntitySetNoTargetCommand::Create
ApeAnimScriptCommand* EntitySetNoTargetCommand::Create(XmlElement* element) const
{
    auto* command = new EntitySetNoTargetCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetBool(kAttrNoTarget, &command->m_noTarget, command->m_noTarget);
    return command;
}

// TODO(0x00075880 EntitySetNoTargetCommand::Execute)

// ---------------------------------------------------------------------------
// "EntitySetVisible" (hash 0x1DDC65F3; NOTE: name from a hash match)

class EntitySetVisibleCommand : public ApeAnimScriptCommand {
public:
    EntitySetVisibleCommand();
    ~EntitySetVisibleCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;          // +0x0C "RefObject"
    uint32_t m_entity = 0;             // +0x10 "Entity"
    uint32_t m_child = 0;              // +0x14 "Child"
    bool m_flag18 = false;             // +0x18 attribute 0xF5897C08
    bool m_flag19 = true;              // +0x19 attribute 0xCD9F562B
    bool m_processComponents = true;   // +0x1A "ProcessComponents"
    bool m_visible = false;            // +0x1B "visible"
};

// @func 0x00075A14 EntitySetVisibleCommand::EntitySetVisibleCommand
EntitySetVisibleCommand::EntitySetVisibleCommand() = default;

// @func 0x0073CD58 EntitySetVisibleCommand::~EntitySetVisibleCommand
// @func 0x0073CD88 EntitySetVisibleCommand::~EntitySetVisibleCommand (deleting)
EntitySetVisibleCommand::~EntitySetVisibleCommand() = default;

// @func 0x00075A7C EntitySetVisibleCommand::Create
ApeAnimScriptCommand* EntitySetVisibleCommand::Create(XmlElement* element) const
{
    auto* command = new EntitySetVisibleCommand();
    element->GetHash(kAttrRefObject, &command->m_refObject, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetBool(kAttrF5897C08, &command->m_flag18, command->m_flag18);
    element->GetBool(kAttrVisibleCD9F, &command->m_flag19, command->m_flag19);
    element->GetBool(kAttrProcessComponents, &command->m_processComponents, command->m_processComponents);
    element->GetBool(kAttrVisible, &command->m_visible, command->m_visible);
    return command;
}

// TODO(0x00075C04 EntitySetVisibleCommand::Execute)

// ---------------------------------------------------------------------------
// FadeDesaturation

class FadeDesaturationCommand : public ApeAnimScriptCommand {
public:
    enum BlendMode : int32_t { kBlend_Opaque = 0, kBlend_Blend = 2, kBlend_Additive = 3, kBlend_Subtractive = 4 };
    enum DesaturationType : int32_t { kType_Monochrome = 0, kType_Negative = 1, kType_Solid = 2 };

    FadeDesaturationCommand();
    explicit FadeDesaturationCommand(XmlElement* element);
    ~FadeDesaturationCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_target = 0;              // +0x0C attribute 0x89E68B87
    float m_intensityStart = 1.0f;      // +0x10 "IntensityStart"
    float m_intensityEnd = 1.0f;        // +0x14 "IntensityEnd"
    Color8 m_colorStart = kDefaultFromColor;  // +0x18 attribute 0xF7E02627
    Color8 m_colorEnd = kDefaultFromColor;    // +0x1C attribute 0xDC8F9BE0
    float m_duration = 1.0f;            // +0x20 "duration"
    int32_t m_blendMode = kBlend_Blend; // +0x24 "blendmode"
    int32_t m_type = kType_Monochrome;  // +0x28 "DesaturationType"
    bool m_enable = true;               // +0x2C "enable"
};

// @func 0x00075D14 FadeDesaturationCommand::FadeDesaturationCommand
FadeDesaturationCommand::FadeDesaturationCommand() = default;

// @func 0x0073CDE0 FadeDesaturationCommand::~FadeDesaturationCommand
// @func 0x0073CE10 FadeDesaturationCommand::~FadeDesaturationCommand (deleting)
FadeDesaturationCommand::~FadeDesaturationCommand() = default;

// @func 0x00075DD4 FadeDesaturationCommand::FadeDesaturationCommand
FadeDesaturationCommand::FadeDesaturationCommand(XmlElement* element)
{
    element->GetHash(kAttrDesaturationTarget, &m_target, m_target);
    element->GetBool(kAttrEnable, &m_enable, m_enable);
    element->GetFloat(kAttrDuration, &m_duration, m_duration);
    if (!m_enable)
        return;

    element->GetFloat(kAttrIntensityStart, &m_intensityStart, m_intensityStart);
    element->GetFloat(kAttrIntensityEnd, &m_intensityEnd, m_intensityEnd);
    element->GetColor(kAttrColorStart, &m_colorStart, &m_colorStart);
    element->GetColor(kAttrColorEnd, &m_colorEnd, &m_colorEnd);

    uint32_t blend = 0;
    if (element->GetHash(kAttrBlendMode, &blend, 0) == ape::xml::kQuery_Ok && blend) {
        switch (blend) {
        case AnimHash("Subtractive"): m_blendMode = kBlend_Subtractive; break;
        case AnimHash("Blend"): m_blendMode = kBlend_Blend; break;
        case AnimHash("Additive"): m_blendMode = kBlend_Additive; break;
        case AnimHash("Opaque"): m_blendMode = kBlend_Opaque; break;
        default: break;
        }
    }
    uint32_t type = 0;
    if (element->GetHash(kAttrDesaturationType, &type, blend) == ape::xml::kQuery_Ok && type) {
        switch (type) {
        case AnimHash("Negative"): m_type = kType_Negative; break;
        case AnimHash("Solid"): m_type = kType_Solid; break;
        case AnimHash("Monochrome"): m_type = kType_Monochrome; break;
        default: break;
        }
    }
}

// @func 0x000760E0 FadeDesaturationCommand::Create
ApeAnimScriptCommand* FadeDesaturationCommand::Create(XmlElement* element) const
{
    return new FadeDesaturationCommand(element);
}

// TODO(0x00076150 FadeDesaturationCommand::Execute)

// ---------------------------------------------------------------------------
// FadeColor

class FadeColorCommand : public ApeAnimScriptCommand {
public:
    FadeColorCommand();
    ~FadeColorCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

    // Current colour: driven by the float variable "InterpVar" when given,
    // else by the state clock over "duration".
    Color8 ComputeColor(ApeAnimScriptInstance* instance, const ApeAnimScriptState* state) const;

private:
    uint32_t m_object = 0;                  // +0x0C "object"
    uint32_t m_material = 0;                // +0x10 "Material"
    uint32_t m_interpVar = 0;               // +0x14 "InterpVar"
    Color8 m_from = kDefaultFromColor;      // +0x18 "From"
    Color8 m_to = kDefaultToColor;          // +0x1C "To"
    float m_duration = 0.0f;                // +0x20 "duration"
};

// @func 0x000763A0 FadeColorCommand::FadeColorCommand
FadeColorCommand::FadeColorCommand() = default;

// @func 0x00076460 FadeColorCommand::~FadeColorCommand
// @func 0x00076490 FadeColorCommand::~FadeColorCommand (deleting)
FadeColorCommand::~FadeColorCommand() = default;

// @func 0x000764F0 FadeColorCommand::Create
ApeAnimScriptCommand* FadeColorCommand::Create(XmlElement* element) const
{
    auto* command = new FadeColorCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok) {
        element->GetHash(kAttrMaterial, &command->m_material, 0);
        element->GetHash(kAttrInterpVar, &command->m_interpVar, 0);
        element->GetColor(kAttrFrom, &command->m_from, &kDefaultFromColor);
        element->GetColor(kAttrTo, &command->m_to, &kDefaultToColor);
        element->GetFloat(kAttrDuration, &command->m_duration, 0.0f);
    }
    return command;
}

// @func 0x00076708 FadeColorCommand::ComputeColor
Color8 FadeColorCommand::ComputeColor(ApeAnimScriptInstance* instance, const ApeAnimScriptState* state) const
{
    if (m_interpVar) {
        ApeAnimScriptVar* var = instance->FindVar(m_interpVar);
        if (var && var->GetType() == AnimScriptVarType::Float)
            return LerpColor(m_from, m_to, BlendFactor(var->GetFloat()));
        return m_to;
    }
    if (state && m_duration >= kMinDuration && state->GetTime() < m_duration)
        return LerpColor(m_from, m_to, BlendFactor(state->GetTime() / m_duration));
    return m_to;
}

// TODO(0x00076970 FadeColorCommand::Execute) applies ComputeColor() to the
// object's materials.

// ---------------------------------------------------------------------------
// FadeEntity

class FadeEntityCommand : public ApeAnimScriptCommand {
public:
    FadeEntityCommand();
    ~FadeEntityCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;  // +0x0C "RefObject"
    float m_from = 1.0f;       // +0x10 "From"
    float m_to = 0.0f;         // +0x14 "To"
    float m_duration = 0.0f;   // +0x18 "duration"
    bool m_flag1C = false;     // +0x1C attribute 0xF5897C08
};

// @func 0x00076ED4 FadeEntityCommand::FadeEntityCommand
FadeEntityCommand::FadeEntityCommand() = default;

// @func 0x00076F38 FadeEntityCommand::~FadeEntityCommand
// @func 0x00076F68 FadeEntityCommand::~FadeEntityCommand (deleting)
FadeEntityCommand::~FadeEntityCommand() = default;

// @func 0x00076FC8 FadeEntityCommand::Create
ApeAnimScriptCommand* FadeEntityCommand::Create(XmlElement* element) const
{
    auto* command = new FadeEntityCommand();
    if (element->GetHash(kAttrRefObject, &command->m_refObject, 0) == ape::xml::kQuery_Ok) {
        element->GetFloat(kAttrFrom, &command->m_from, 1.0f);
        element->GetFloat(kAttrTo, &command->m_to, 0.0f);
        element->GetFloat(kAttrDuration, &command->m_duration, 0.0f);
        element->GetBool(kAttrF5897C08, &command->m_flag1C, false);
    }
    return command;
}

// TODO(0x00077138 FadeEntityCommand::Execute)

// ---------------------------------------------------------------------------
// "FadeLightIntensity" (hash 0x5DA7E559; NOTE: name from a hash match)

class FadeLightIntensityCommand : public ApeAnimScriptCommand {
public:
    FadeLightIntensityCommand();
    ~FadeLightIntensityCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object"
    float m_field10 = -1.0f;   // +0x10 NOTE: runtime (start intensity when "From" is negative?)
    float m_from = -1.0f;      // +0x14 "From"
    float m_to = -1.0f;        // +0x18 "To"
    float m_duration = 0.0f;   // +0x1C "duration"
};

// @func 0x00077284 FadeLightIntensityCommand::FadeLightIntensityCommand
FadeLightIntensityCommand::FadeLightIntensityCommand() = default;

// @func 0x000772F4 FadeLightIntensityCommand::~FadeLightIntensityCommand
// @func 0x00077324 FadeLightIntensityCommand::~FadeLightIntensityCommand (deleting)
FadeLightIntensityCommand::~FadeLightIntensityCommand() = default;

// @func 0x00077388 FadeLightIntensityCommand::Create
ApeAnimScriptCommand* FadeLightIntensityCommand::Create(XmlElement* element) const
{
    auto* command = new FadeLightIntensityCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok) {
        element->GetFloat(kAttrFrom, &command->m_from, -1.0f);
        element->GetFloat(kAttrTo, &command->m_to, -1.0f);
        element->GetFloat(kAttrDuration, &command->m_duration, 0.0f);
    }
    return command;
}

// TODO(0x000774D8 FadeLightIntensityCommand::Execute)

// ---------------------------------------------------------------------------
// FadeObject

class FadeObjectCommand : public ApeAnimScriptCommand {
public:
    FadeObjectCommand();
    ~FadeObjectCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;     // +0x0C "object"
    uint32_t m_part = 0;       // +0x10 "part"
    float m_from = 1.0f;       // +0x14 "From"
    float m_to = 0.0f;         // +0x18 "To"
    float m_duration = 0.0f;   // +0x1C "duration"
    bool m_flag20 = false;     // +0x20 attribute 0xF5897C08
};

// @func 0x00077630 FadeObjectCommand::FadeObjectCommand
FadeObjectCommand::FadeObjectCommand() = default;

// @func 0x00077698 FadeObjectCommand::~FadeObjectCommand
// @func 0x000776C8 FadeObjectCommand::~FadeObjectCommand (deleting)
FadeObjectCommand::~FadeObjectCommand() = default;

// @func 0x00077728 FadeObjectCommand::Create
ApeAnimScriptCommand* FadeObjectCommand::Create(XmlElement* element) const
{
    auto* command = new FadeObjectCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok) {
        element->GetHash(kAttrPart, &command->m_part, 0);
        element->GetFloat(kAttrFrom, &command->m_from, 1.0f);
        element->GetFloat(kAttrTo, &command->m_to, 0.0f);
        element->GetFloat(kAttrDuration, &command->m_duration, 0.0f);
        element->GetBool(kAttrF5897C08, &command->m_flag20, false);
    }
    return command;
}

// TODO(0x000778C0 FadeObjectCommand::Execute)

// ---------------------------------------------------------------------------
// "FadeOverrideColor" (hash 0x48AF45FB; NOTE: name from a hash match)

class FadeOverrideColorCommand : public ApeAnimScriptCommand {
public:
    FadeOverrideColorCommand();
    ~FadeOverrideColorCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

    // Colour at the state's current time; returns true once the fade is over.
    bool ComputeColor(const ApeAnimScriptState* state, Color8* out) const;

private:
    uint32_t m_object = 0;                // +0x0C "object"
    Color8 m_from = kDefaultFromColor;    // +0x10 "From"
    Color8 m_to = kDefaultToColor;        // +0x14 "To"
    float m_duration = 0.0f;              // +0x18 "duration"
};

// @func 0x00077A8C FadeOverrideColorCommand::FadeOverrideColorCommand
FadeOverrideColorCommand::FadeOverrideColorCommand() = default;

// @func 0x00077B3C FadeOverrideColorCommand::~FadeOverrideColorCommand
// @func 0x00077B6C FadeOverrideColorCommand::~FadeOverrideColorCommand (deleting)
FadeOverrideColorCommand::~FadeOverrideColorCommand() = default;

// @func 0x00077BC8 FadeOverrideColorCommand::Create
ApeAnimScriptCommand* FadeOverrideColorCommand::Create(XmlElement* element) const
{
    auto* command = new FadeOverrideColorCommand();
    if (element->GetHash(kAttrObject, &command->m_object, 0) == ape::xml::kQuery_Ok) {
        element->GetColor(kAttrFrom, &command->m_from, &kDefaultFromColor);
        element->GetColor(kAttrTo, &command->m_to, &kDefaultToColor);
        element->GetFloat(kAttrDuration, &command->m_duration, 0.0f);
    }
    return command;
}

// @func 0x00077D90 FadeOverrideColorCommand::ComputeColor
bool FadeOverrideColorCommand::ComputeColor(const ApeAnimScriptState* state, Color8* out) const
{
    *out = m_to;
    if (!state || m_duration < kMinDuration || m_duration <= state->GetTime())
        return true;
    *out = LerpColor(m_from, m_to, BlendFactor(state->GetTime() / m_duration));
    return false;
}

// TODO(0x00077EB0 FadeOverrideColorCommand::Execute)

// ---------------------------------------------------------------------------
// FireProjectile

class FireProjectileCommand : public ApeAnimScriptCommand {
public:
    FireProjectileCommand();
    explicit FireProjectileCommand(XmlElement* element);
    ~FireProjectileCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_refObject = 0;                          // +0x0C "RefObject"
    uint32_t m_entity = 0;                             // +0x10 "Entity"
    uint32_t m_field14 = 0;                            // +0x14 attribute 0xAA44DED3
    ape::Vec3 m_offset = {0.0f, 0.0f, 0.0f, 0.0f};     // +0x20 "Offset"
    uint32_t m_field30 = 0;                            // +0x30 attribute 0x0FDC3B3E
    uint32_t m_tgtObject = 0;                          // +0x34 "TgtObject"
    uint32_t m_tgtEntity = 0;                          // +0x38 "TgtEntity"
    uint32_t m_tgtPart = 0;                            // +0x3C "TgtPart"
    ape::Vec3 m_tgtOffset = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x40 "TgtOffset"
    uint32_t m_projectileDef = 0;                      // +0x50 "ProjectileDef"
    float m_speed = 50.0f;                             // +0x54 "speed"
    ape::Vec3 m_launchAngle = {0.0f, 0.0f, 0.0f, 0.0f};// +0x60 "LaunchAngle" (degrees in data)
    uint32_t m_field70 = 0;                            // +0x70 attribute 0xEAC031A0
    uint32_t m_projectileObj = 0;                      // +0x74 "ProjectileObj"
    float m_lifetimeMax = 0.0f;                        // +0x78 "LifetimeMax"
    float m_field7C = 0.0f;                            // +0x7C attribute 0x26055815
    float m_field80 = 0.0f;                            // +0x80 attribute 0x0291E67F
    float m_field84 = 0.0f;                            // +0x84 attribute 0x77FB0D02
    bool m_hasLifetimeMax = false;                     // +0x88
    bool m_hasField7C = false;                         // +0x89
    bool m_hasField80 = false;                         // +0x8A
    bool m_hasField84 = false;                         // +0x8B
    bool m_flag8C = false;                             // +0x8C attribute 0x8C63F207
};

// @func 0x00077FE0 FireProjectileCommand::FireProjectileCommand
FireProjectileCommand::FireProjectileCommand() = default;

// @func 0x0073CE68 FireProjectileCommand::~FireProjectileCommand
// @func 0x0073CE98 FireProjectileCommand::~FireProjectileCommand (deleting)
FireProjectileCommand::~FireProjectileCommand() = default;

// @func 0x000780D0 FireProjectileCommand::FireProjectileCommand
FireProjectileCommand::FireProjectileCommand(XmlElement* element)
{
    element->GetHash(kAttrRefObject, &m_refObject, 0);
    element->GetHash(kAttrEntity, &m_entity, m_entity);
    element->GetHash(kAttrProjectileAA44, &m_field14, m_field14);
    element->GetBool(kAttrProjectile8C63, &m_flag8C, m_flag8C);
    element->GetVec3(kAttrOffset, &m_offset.x, &m_offset.x);
    element->GetHash(kAttrProjectileFDC3, &m_field30, m_field30);
    element->GetHash(kAttrTgtObject, &m_tgtObject, m_tgtObject);
    element->GetHash(kAttrTgtEntity, &m_tgtEntity, m_tgtEntity);
    element->GetHash(kAttrTgtPart, &m_tgtPart, m_tgtPart);
    element->GetVec3(kAttrTgtOffset, &m_tgtOffset.x, &m_tgtOffset.x);
    element->GetHash(kAttrProjectileDef, &m_projectileDef, m_projectileDef);
    element->GetFloat(kAttrSpeed, &m_speed, m_speed);
    if (element->GetVec3(kAttrLaunchAngle, &m_launchAngle.x, &m_launchAngle.x) == ape::xml::kQuery_Ok) {
        m_launchAngle.x *= kDegToRad;
        m_launchAngle.y *= kDegToRad;
        m_launchAngle.z *= kDegToRad;
    }
    element->GetHash(kAttrProjectileEAC0, &m_field70, m_field70);
    element->GetHash(kAttrProjectileObj, &m_projectileObj, m_projectileObj);
    m_hasLifetimeMax = element->GetFloat(kAttrLifetimeMax, &m_lifetimeMax, m_lifetimeMax) == ape::xml::kQuery_Ok;
    m_hasField7C = element->GetFloat(kAttrProjectile2605, &m_field7C, m_field7C) == ape::xml::kQuery_Ok;
    m_hasField80 = element->GetFloat(kAttrProjectile0291, &m_field80, m_field80) == ape::xml::kQuery_Ok;
    m_hasField84 = element->GetFloat(kAttrProjectile77FB, &m_field84, m_field84) == ape::xml::kQuery_Ok;
}

// @func 0x000784C8 FireProjectileCommand::Create
ApeAnimScriptCommand* FireProjectileCommand::Create(XmlElement* element) const
{
    return new FireProjectileCommand(element);
}

// TODO(0x00078540 FireProjectileCommand::Execute)

// ---------------------------------------------------------------------------

void RegisterScriptCommandsF()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("EntityKillFx"), new EntityKillFxCommand());
    prototypes.Add(AnimHash("EntityGetCollectInfo"), new EntityGetCollectInfoCommand());
    prototypes.Add(AnimHash("EntitySetEnabled"), new EntitySetEnabledCommand());
    prototypes.Add(AnimHash("EntitySetScriptState"), new EntitySetScriptStateCommand());
    prototypes.Add(AnimHash("EntitySetGadgetState"), new EntitySetGadgetStateCommand());
    prototypes.Add(AnimHash("EntitySetNoTarget"), new EntitySetNoTargetCommand());
    prototypes.Add(AnimHash("EntitySetVisible"), new EntitySetVisibleCommand());
    prototypes.Add(AnimHash("FadeColor"), new FadeColorCommand());
    prototypes.Add(AnimHash("FadeDesaturation"), new FadeDesaturationCommand());
    prototypes.Add(AnimHash("FadeEntity"), new FadeEntityCommand());
    prototypes.Add(AnimHash("FadeLightIntensity"), new FadeLightIntensityCommand());
    prototypes.Add(AnimHash("FadeObject"), new FadeObjectCommand());
    prototypes.Add(AnimHash("FadeOverrideColor"), new FadeOverrideColorCommand());
    prototypes.Add(AnimHash("FireProjectile"), new FireProjectileCommand());
}

} // namespace levels
} // namespace sly
