// openTIT - Built-in anim script commands that act on world objects.
#include "ape/anim/ApeAnimScriptObjectCommands.h"

#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/xml/ApeXml.h"
#include "ape/input/ApeInputDevice.h"
#include "ape/world/ApeRegions.h"

namespace ape {

namespace {
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrObjectHash = AnimHash("ObjectHash");
constexpr uint32_t kAttrTarget = AnimHash("Target");
constexpr uint32_t kAttrTargetHash = AnimHash("TargetHash");
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrDef = AnimHash("def");
constexpr uint32_t kAttrDefHash = AnimHash("defHash");
constexpr uint32_t kAttrCloneDef = AnimHash("CloneDef");
constexpr uint32_t kAttrHideDef = AnimHash("HideDef");
constexpr uint32_t kAttrTranslationOnly = AnimHash("TranslationOnly");
constexpr uint32_t kAttrInheritScale = AnimHash("InheritScale");
constexpr uint32_t kAttrLocator = AnimHash("Locator");
constexpr uint32_t kAttrLocatorHash = 0xABA3F684u;    // NOTE: name unknown
constexpr uint32_t kAttrFlag11605B48 = 0x11605B48u;   // NOTE: name unknown
constexpr uint32_t kAttrVisible = AnimHash("visible");
constexpr uint32_t kAttrFromWorld = 0x6334AE6Fu;      // NOTE: name unknown
constexpr uint32_t kAttrFlagF5897C08 = 0xF5897C08u;   // NOTE: name unknown
constexpr uint32_t kAttrSetDirty = AnimHash("SetDirty");
constexpr uint32_t kAttrFov = AnimHash("FOV");
constexpr uint32_t kAttrOcclPlaneHash = 0x3F4EC4ADu;  // NOTE: name unknown
constexpr uint32_t kAttrActive = AnimHash("active");
constexpr uint32_t kAttrEnabled = AnimHash("enabled");
constexpr uint32_t kAttrVar = AnimHash("var");
constexpr uint32_t kAttrRate = AnimHash("rate");
constexpr uint32_t kAttrPlaneDist = AnimHash("PlaneDist");
constexpr uint32_t kAttrBounce = AnimHash("bounce");
constexpr uint32_t kAttrDecay = AnimHash("Decay");
constexpr uint32_t kAttrAmplitude = AnimHash("Amplitude");
constexpr uint32_t kAttrMaterial = AnimHash("Material");
constexpr uint32_t kAttrMaterialHash = AnimHash("materialHash");
constexpr uint32_t kAttrAdd = AnimHash("add");
constexpr uint32_t kAttrColor = AnimHash("Color");
constexpr uint32_t kAttrColorVars[4] = {AnimHash("VarR"), AnimHash("VarG"), AnimHash("VarB"), AnimHash("VarA")};
constexpr uint32_t kAttrFlag54FEB607 = 0x54FEB607u;   // NOTE: name unknown
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrResetTime = AnimHash("ResetTime");
constexpr uint32_t kAttrScale = AnimHash("Scale");
constexpr uint32_t kAttrMatrix = AnimHash("matrix");
constexpr uint32_t kAttrAttachField1C = 0xFB888634u;  // NOTE: name unknown
constexpr uint32_t kAttrAttachFlag7 = 0x72C27306u;    // NOTE: name unknown
constexpr uint32_t kAttrAttachFlag6 = 0xE48AEDBDu;    // NOTE: name unknown
constexpr uint32_t kAttrAttachFlag5 = 0xFF40848Eu;    // NOTE: name unknown
constexpr uint32_t kAttrAttachFlag4 = 0x96423570u;    // NOTE: name unknown
constexpr uint32_t kAttrAttachFlag3D = 0x7631EAF5u;   // NOTE: name unknown
constexpr uint32_t kAttrPos[3] = {AnimHash("posX"), AnimHash("posY"), AnimHash("posZ")};
constexpr uint32_t kAttrRot[3] = {AnimHash("rotX"), AnimHash("rotY"), AnimHash("rotZ")};
constexpr uint32_t kAttrLocal = AnimHash("local");
constexpr uint32_t kAttrVarPos[3] = {AnimHash("varposX"), AnimHash("varposY"), AnimHash("varposZ")};
constexpr uint32_t kAttrVarRot[3] = {AnimHash("varrotX"), AnimHash("varrotY"), AnimHash("varrotZ")};
constexpr uint32_t kAttrWorld[3] = {AnimHash("worldX"), AnimHash("worldY"), AnimHash("worldZ")};
constexpr uint32_t kAttrVariable = AnimHash("variable");
constexpr uint32_t kAttrFrom = AnimHash("From");
constexpr uint32_t kAttrFromVar = AnimHash("FromVar");
constexpr uint32_t kAttrTo = AnimHash("To");
constexpr uint32_t kAttrToVar = AnimHash("ToVar");
constexpr uint32_t kAttrDuration = AnimHash("Duration");
constexpr uint32_t kAttrDurationVar = AnimHash("DurationVar");
constexpr uint32_t kAttrFlag71B29C0F = 0x71B29C0Fu;   // NOTE: name unknown
constexpr uint32_t kAttrScript = AnimHash("script");

AnimScriptObjectHooks g_objectHooks;

void WarnName(uint32_t nameHash)
{
    (void)TempHexString(nameHash);
}

// "name" given as a hash attribute, else an int attribute with the hash.
uint32_t ReadHashOrInt(xml::XmlElement* element, uint32_t hashAttr, uint32_t intAttr)
{
    uint32_t value = 0;
    if (element->GetHash(hashAttr, &value, 0) == xml::kQuery_Ok)
        return value;
    int32_t intValue = 0;
    element->GetInt(intAttr, &intValue, 0);
    return (uint32_t)intValue;
}

// Object operand given as a string attribute (hashed at load time).
uint32_t ReadObjectName(xml::XmlElement* element)
{
    if (const char* name = element->GetString("object"))
        return HashNameNoCase(name);
    int32_t value = 0;
    element->GetInt(kAttrObjectHash, &value, 0);
    return (uint32_t)value;
}

bool ReadBool(xml::XmlElement* element, uint32_t attr, bool defaultValue)
{
    bool value = defaultValue;
    element->GetBool(attr, &value, defaultValue);
    return value;
}

Vec3 ReadVec3(xml::XmlElement* element, uint32_t attr, const Vec3& defaultValue)
{
    float value[3] = {defaultValue.x, defaultValue.y, defaultValue.z};
    element->GetVec3(attr, value, value);
    return {value[0], value[1], value[2], 0.0f};
}
} // namespace

AnimScriptObjectHooks& GetAnimScriptObjectHooks()
{
    return g_objectHooks;
}

uint32_t ReadObjectOperand(xml::XmlElement* element)
{
    return ReadHashOrInt(element, kAttrObject, kAttrObjectHash);
}

// ---------------------------------------------------------------------------
// SwapDef

// @func 0x004CA4E0 ApeAnimScriptSwapDefCommand::~ApeAnimScriptSwapDefCommand
// @func 0x004CA4F0 ApeAnimScriptSwapDefCommand::~ApeAnimScriptSwapDefCommand (deleting)
ApeAnimScriptSwapDefCommand::~ApeAnimScriptSwapDefCommand() = default;

// @func 0x004CA534 ApeAnimScriptSwapDefCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSwapDefCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptSwapDefCommand();
    command->m_object = ReadObjectOperand(element);
    element->GetHash(kAttrDef, &command->m_def, 0);
    command->m_cloneDef = ReadBool(element, kAttrCloneDef, false);
    return command;
}

// TODO(0x004CA668 ApeAnimScriptSwapDefCommand::Execute) calls into the entity definition system.

// @func 0x004CA874 ApeAnimScriptSwapDefCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSwapDefCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetObject

// @func 0x004C1CB8 ApeAnimScriptSetObjectCommand::~ApeAnimScriptSetObjectCommand
// @func 0x004C1CC8 ApeAnimScriptSetObjectCommand::~ApeAnimScriptSetObjectCommand (deleting)
ApeAnimScriptSetObjectCommand::~ApeAnimScriptSetObjectCommand() = default;

// @func 0x004C1D0C ApeAnimScriptSetObjectCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetObjectCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptSetObjectCommand();
    command->m_object = ReadObjectOperand(element);
    command->m_target = ReadHashOrInt(element, kAttrTarget, kAttrTargetHash);
    element->GetHash(kAttrPart, &command->m_part, 0);
    element->GetBool(kAttrFromWorld, &command->m_fromWorld, false);
    return command;
}

// @func 0x004C1E6C ApeAnimScriptSetObjectCommand::Execute
void ApeAnimScriptSetObjectCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    ApeAnimScriptVar* var = instance->FindVar(m_target);
    if (!var) {
        WarnName(m_target);
        return;
    }
    if (var->GetType() != AnimScriptVarType::Object)
        return;
    ApeRefCounted* object = instance->ResolveObject(m_object, m_fromWorld);
    if (!object) {
        WarnName(m_object);
        return;
    }
    if (m_part) {
        ApeRefCounted* part = g_objectHooks.findPart ? g_objectHooks.findPart(object, m_part) : nullptr;
        if (part)
            object = part;
        else
            WarnName(m_part);
    }
    auto* objectVar = static_cast<ApeAnimScriptObjectVar*>(var);
    objectVar->SetObject(object);
    objectVar->SetNotifyObject(false);
}

// @func 0x004C1FDC ApeAnimScriptSetObjectCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetObjectCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SpawnObject

// @func 0x004C1FE4 ApeAnimScriptSpawnObjectCommand::ApeAnimScriptSpawnObjectCommand
ApeAnimScriptSpawnObjectCommand::ApeAnimScriptSpawnObjectCommand(xml::XmlElement* element)
{
    m_def = ReadHashOrInt(element, kAttrDef, kAttrDefHash);
    m_object = ReadObjectOperand(element);
    m_target = ReadHashOrInt(element, kAttrTarget, kAttrTargetHash);
    element->GetHash(kAttrPart, &m_part, 0);
    m_locator = ReadHashOrInt(element, kAttrLocator, kAttrLocatorHash);
    m_cloneDef = ReadBool(element, kAttrCloneDef, m_cloneDef);
    m_hideDef = ReadBool(element, kAttrHideDef, m_hideDef);
    m_translationOnly = ReadBool(element, kAttrTranslationOnly, m_translationOnly);
    m_inheritScale = ReadBool(element, kAttrInheritScale, m_inheritScale);
    m_flag11605B48 = ReadBool(element, kAttrFlag11605B48, m_flag11605B48);
    bool visible = false;
    if (element->GetBool(kAttrVisible, &visible, false) == xml::kQuery_Ok)
        m_visible = visible ? 1 : 0;
}

// @func 0x00794210 ApeAnimScriptSpawnObjectCommand::~ApeAnimScriptSpawnObjectCommand
// @func 0x00794220 ApeAnimScriptSpawnObjectCommand::~ApeAnimScriptSpawnObjectCommand (deleting)
ApeAnimScriptSpawnObjectCommand::~ApeAnimScriptSpawnObjectCommand() = default;

// @func 0x004C2324 ApeAnimScriptSpawnObjectCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSpawnObjectCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptSpawnObjectCommand(element);
}

// TODO(0x004C2394 ApeAnimScriptSpawnObjectCommand::SpawnClone) creates "%s_clone"/"%s_inst" entities.
// TODO(0x004C27E0 ApeAnimScriptSpawnObjectCommand::Execute) needs the entity spawner.

// @func 0x004C30B4 ApeAnimScriptSpawnObjectCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSpawnObjectCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetVisible

// @func 0x004C3DC0 ApeAnimScriptSetVisibleCommand::~ApeAnimScriptSetVisibleCommand
// @func 0x004C3DD0 ApeAnimScriptSetVisibleCommand::~ApeAnimScriptSetVisibleCommand (deleting)
ApeAnimScriptSetVisibleCommand::~ApeAnimScriptSetVisibleCommand() = default;

// @func 0x004C3E14 ApeAnimScriptSetVisibleCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetVisibleCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptSetVisibleCommand();
    command->m_object = ReadObjectOperand(element);
    element->GetHash(kAttrPart, &command->m_part, 0);
    element->GetBool(kAttrVisible, &command->m_visible, false);
    element->GetBool(kAttrFlagF5897C08, &command->m_flagF5897C08, false);
    element->GetBool(kAttrFromWorld, &command->m_fromWorld, false);
    element->GetBool(kAttrSetDirty, &command->m_setDirty, false);
    return command;
}

// TODO(0x004C3F9C ApeAnimScriptSetVisibleCommand::Execute) walks entity parts.

// @func 0x004C4334 ApeAnimScriptSetVisibleCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetVisibleCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetFOV

// @func 0x004C4354 ApeAnimScriptSetFovCommand::~ApeAnimScriptSetFovCommand
// @func 0x004C4364 ApeAnimScriptSetFovCommand::~ApeAnimScriptSetFovCommand (deleting)
ApeAnimScriptSetFovCommand::~ApeAnimScriptSetFovCommand() = default;

// @func 0x004C43B0 ApeAnimScriptSetFovCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetFovCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptSetFovCommand();
    command->m_object = ReadObjectName(element);
    element->GetFloat(kAttrFov, &command->m_fov, 0.0f);
    return command;
}

// TODO(0x004C44BC ApeAnimScriptSetFovCommand::Execute) sets a camera's field of view.

// @func 0x004C452C ApeAnimScriptSetFovCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetFovCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// Occlusion plane

// @func 0x004C4538 ApeAnimScriptOcclusionPlaneCommand::~ApeAnimScriptOcclusionPlaneCommand
// @func 0x004C4548 ApeAnimScriptOcclusionPlaneCommand::~ApeAnimScriptOcclusionPlaneCommand (deleting)
ApeAnimScriptOcclusionPlaneCommand::~ApeAnimScriptOcclusionPlaneCommand() = default;

// @func 0x004C458C ApeAnimScriptOcclusionPlaneCommand::Create
ApeAnimScriptCommand* ApeAnimScriptOcclusionPlaneCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptOcclusionPlaneCommand();
    if (const char* name = element->GetString("occlPlane")) {
        command->m_plane = HashNameNoCase(name);
    } else {
        int32_t value = 0;
        element->GetInt(kAttrOcclPlaneHash, &value, 0);
        command->m_plane = (uint32_t)value;
    }
    element->GetBool(kAttrActive, &command->m_active, false);
    return command;
}

// @func 0x004C4684 ApeAnimScriptOcclusionPlaneCommand::Execute
// Switches the named occluder quad on or off (quad flag bit 0).
void ApeAnimScriptOcclusionPlaneCommand::Execute(ApeAnimScriptInstance* /*instance*/, ApeAnimScriptState* /*state*/)
{
    world::ApeQuad* quad = world::FindQuad(m_plane);
    if (!quad)
        return;
    if (m_active)
        quad->flags |= 1;
    else
        quad->flags &= uint16_t(~1u);
}

// @func 0x004C46EC ApeAnimScriptOcclusionPlaneCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptOcclusionPlaneCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// Enable object

// @func 0x004CC510 ApeAnimScriptEnableObjectCommand::~ApeAnimScriptEnableObjectCommand
// @func 0x004CC520 ApeAnimScriptEnableObjectCommand::~ApeAnimScriptEnableObjectCommand (deleting)
ApeAnimScriptEnableObjectCommand::~ApeAnimScriptEnableObjectCommand() = default;

// @func 0x004CC564 ApeAnimScriptEnableObjectCommand::Create
ApeAnimScriptCommand* ApeAnimScriptEnableObjectCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptEnableObjectCommand();
    command->m_object = ReadObjectOperand(element);
    element->GetBool(kAttrEnabled, &command->m_enabled, false);
    element->GetBool(kAttrFlagF5897C08, &command->m_flagF5897C08, false);
    return command;
}

// TODO(0x004CC704 ApeAnimScriptEnableObjectCommand::Execute)

// @func 0x004CC74C ApeAnimScriptEnableObjectCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptEnableObjectCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// Object rate

// @func 0x004CC754 ApeAnimScriptObjectRateCommand::~ApeAnimScriptObjectRateCommand
// @func 0x004CC764 ApeAnimScriptObjectRateCommand::~ApeAnimScriptObjectRateCommand (deleting)
ApeAnimScriptObjectRateCommand::~ApeAnimScriptObjectRateCommand() = default;

// @func 0x004CC7B0 ApeAnimScriptObjectRateCommand::Create
ApeAnimScriptCommand* ApeAnimScriptObjectRateCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptObjectRateCommand();
    command->m_object = ReadObjectOperand(element);
    element->GetHash(kAttrVar, &command->m_var, 0);
    element->GetFloat(kAttrRate, &command->m_rate, 0.0f);
    element->GetBool(kAttrFlagF5897C08, &command->m_flagF5897C08, false);
    element->GetBool(kAttrFromWorld, &command->m_fromWorld, false);
    return command;
}

// TODO(0x004CC928 ApeAnimScriptObjectRateCommand::Execute)

// @func 0x004CCA88 ApeAnimScriptObjectRateCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptObjectRateCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetDeflector

// @func 0x004CCA9C ApeAnimScriptSetDeflectorCommand::~ApeAnimScriptSetDeflectorCommand
// @func 0x004CCAAC ApeAnimScriptSetDeflectorCommand::~ApeAnimScriptSetDeflectorCommand (deleting)
ApeAnimScriptSetDeflectorCommand::~ApeAnimScriptSetDeflectorCommand() = default;

// @func 0x004CCAF8 ApeAnimScriptSetDeflectorCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetDeflectorCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptSetDeflectorCommand();
    command->m_object = ReadObjectOperand(element);
    element->GetFloat(kAttrPlaneDist, &command->m_planeDist, 0.0f);
    element->GetFloat(kAttrBounce, &command->m_bounce, 0.0f);
    return command;
}

// TODO(0x004CCC30 ApeAnimScriptSetDeflectorCommand::Execute) configures a particle deflector.

// @func 0x004CCD10 ApeAnimScriptSetDeflectorCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetDeflectorCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// Rumble

// @func 0x004C5BD0 ApeAnimScriptRumbleCommand::~ApeAnimScriptRumbleCommand
// @func 0x004C5BE0 ApeAnimScriptRumbleCommand::~ApeAnimScriptRumbleCommand (deleting)
ApeAnimScriptRumbleCommand::~ApeAnimScriptRumbleCommand() = default;

// @func 0x004C5C28 ApeAnimScriptRumbleCommand::Create
ApeAnimScriptCommand* ApeAnimScriptRumbleCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptRumbleCommand();
    if (const char* name = element->GetString("controller"))
        command->m_controller = HashNameNoCase(name);
    element->GetFloat(kAttrDecay, &command->m_decay, 0.0f);
    element->GetFloat(kAttrAmplitude, &command->m_amplitude, 0.0f);
    return command;
}

// @func 0x004C5D38 ApeAnimScriptRumbleCommand::Execute
// Vibrates the pad of the controller variable (or the default pad when the
// operand is not a controller variable).
void ApeAnimScriptRumbleCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* /*state*/)
{
    ApeInputDevice* device = g_objectHooks.defaultRumbleDevice;
    ApeAnimScriptVar* var = instance->FindVar(m_controller);
    if (var && var->GetType() == AnimScriptVarType::Controller)
        device = static_cast<ApeAnimScriptControllerVar*>(var)->GetDevice();
    if (device)
        device->StartRumble(m_amplitude, m_decay, 0);
}

// @func 0x004C5DDC ApeAnimScriptRumbleCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptRumbleCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetColor

// @func 0x004C96E4 ApeAnimScriptSetColorCommand::ApeAnimScriptSetColorCommand
ApeAnimScriptSetColorCommand::ApeAnimScriptSetColorCommand(xml::XmlElement* element)
{
    m_object = ReadObjectOperand(element);
    m_material = ReadHashOrInt(element, kAttrMaterial, kAttrMaterialHash);
    element->GetBool(kAttrAdd, &m_add, m_add);
    // NOTE: the default colour is an engine constant (0x01164CA0), assumed white.
    const xml::Color8 white = {0xFF, 0xFF, 0xFF, 0xFF};
    xml::Color8 color = white;
    element->GetColor(kAttrColor, &color, &white);
    m_color[0] = color.r;
    m_color[1] = color.g;
    m_color[2] = color.b;
    m_color[3] = color.a;
    for (int i = 0; i < 4; ++i)
        element->GetHash(kAttrColorVars[i], &m_colorVar[i], 0);
}

// @func 0x004C98D4 ApeAnimScriptSetColorCommand::~ApeAnimScriptSetColorCommand
// @func 0x004C98E4 ApeAnimScriptSetColorCommand::~ApeAnimScriptSetColorCommand (deleting)
ApeAnimScriptSetColorCommand::~ApeAnimScriptSetColorCommand() = default;

// @func 0x004C9928 ApeAnimScriptSetColorCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetColorCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptSetColorCommand(element);
}

// TODO(0x004C99B0 ApeAnimScriptSetColorCommand::ApplyToMaterial)
// TODO(0x004C9D60 ApeAnimScriptSetColorCommand::Execute) tints entity materials.

// @func 0x004CA4D8 ApeAnimScriptSetColorCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetColorCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// EnableProcAnim

// @func 0x00793BB4 ApeAnimScriptEnableProcAnimCommand::~ApeAnimScriptEnableProcAnimCommand
// @func 0x007944DC ApeAnimScriptEnableProcAnimCommand::~ApeAnimScriptEnableProcAnimCommand (deleting)
ApeAnimScriptEnableProcAnimCommand::~ApeAnimScriptEnableProcAnimCommand() = default;

// @func 0x004CCD18 ApeAnimScriptEnableProcAnimCommand::Create
ApeAnimScriptCommand* ApeAnimScriptEnableProcAnimCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptEnableProcAnimCommand();
    command->m_object = ReadObjectOperand(element);
    element->GetBool(kAttrFlag54FEB607, &command->m_flag54FEB607, command->m_flag54FEB607);
    if (element->GetBool(kAttrEnable, &command->m_enable, command->m_enable) == xml::kQuery_Ok)
        command->m_hasEnable = true;
    element->GetBool(kAttrResetTime, &command->m_resetTime, command->m_resetTime);
    return command;
}

// TODO(0x004CCE6C ApeAnimScriptEnableProcAnimCommand::Execute)

// @func 0x004CCF14 ApeAnimScriptEnableProcAnimCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptEnableProcAnimCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetScale

// @func 0x00793B94 ApeAnimScriptSetScaleCommand::~ApeAnimScriptSetScaleCommand
// @func 0x00794454 ApeAnimScriptSetScaleCommand::~ApeAnimScriptSetScaleCommand (deleting)
ApeAnimScriptSetScaleCommand::~ApeAnimScriptSetScaleCommand() = default;

// @func 0x004CB328 ApeAnimScriptSetScaleCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetScaleCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptSetScaleCommand();
    command->m_object = ReadObjectName(element);
    const Vec3 ones = {1.0f, 1.0f, 1.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
    const float defaults[3] = {1.0f, 1.0f, 1.0f};
    if (element->GetVec3(kAttrScale, scale, defaults) == xml::kQuery_Ok) {
        command->m_scale = {scale[0], scale[1], scale[2], 0.0f};
    } else {
        command->m_scale = ones;
        // A single number scales uniformly.
        float uniform = 0.0f;
        if (element->GetFloat(kAttrScale, &uniform, 0.0f) == xml::kQuery_Ok)
            command->m_scale = {uniform, uniform, uniform, 0.0f};
    }
    element->GetBool(kAttrFlagF5897C08, &command->m_flagF5897C08, command->m_flagF5897C08);
    return command;
}

// TODO(0x004CB4A0 ApeAnimScriptSetScaleCommand::Execute)

// @func 0x004CB5E8 ApeAnimScriptSetScaleCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetScaleCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// SetMatrixVar

// @func 0x004CB094 ApeAnimScriptSetMatrixVarCommand::~ApeAnimScriptSetMatrixVarCommand
// @func 0x004CB0A4 ApeAnimScriptSetMatrixVarCommand::~ApeAnimScriptSetMatrixVarCommand (deleting)
ApeAnimScriptSetMatrixVarCommand::~ApeAnimScriptSetMatrixVarCommand() = default;

// @func 0x004CB0E8 ApeAnimScriptSetMatrixVarCommand::Create
ApeAnimScriptCommand* ApeAnimScriptSetMatrixVarCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptSetMatrixVarCommand();
    element->GetHash(kAttrVariable, &command->m_variable, 0);
    element->GetHash(kAttrObject, &command->m_object, command->m_object);
    return command;
}

// @func 0x004CB1B4 ApeAnimScriptSetMatrixVarCommand::Execute
void ApeAnimScriptSetMatrixVarCommand::Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    ApeAnimScriptVar* var = instance->FindVar(m_variable);
    if (!var || var->GetType() != AnimScriptVarType::Matrix) {
        WarnName(m_variable);
        return;
    }
    ApeRefCounted* object = instance->ResolveObject(m_object, false);
    if (!object && g_objectHooks.defaultTransformObject)
        object = g_objectHooks.defaultTransformObject();
    Matrix34 transform;
    if (!object || !g_objectHooks.getWorldTransform || !g_objectHooks.getWorldTransform(object, transform)) {
        WarnName(m_object);
        return;
    }
    static_cast<ApeAnimScriptMatrixVar*>(var)->SetMatrix(transform);
}

// @func 0x004CB308 ApeAnimScriptSetMatrixVarCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptSetMatrixVarCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// InterpScale

// @func 0x004CB5F8 ApeAnimScriptInterpScaleCommand::ApeAnimScriptInterpScaleCommand
ApeAnimScriptInterpScaleCommand::ApeAnimScriptInterpScaleCommand(xml::XmlElement* element)
{
    // NOTE: the remaining attributes are only read when "object" was found.
    if (element->GetHash(kAttrObject, &m_object, 0) != xml::kQuery_Ok)
        return;
    const Vec3 ones = {1.0f, 1.0f, 1.0f, 0.0f};
    m_from = ReadVec3(element, kAttrFrom, ones);
    element->GetHash(kAttrFromVar, &m_fromVar, m_fromVar);
    m_to = ReadVec3(element, kAttrTo, ones);
    element->GetHash(kAttrToVar, &m_toVar, m_toVar);
    element->GetFloat(kAttrDuration, &m_duration, m_duration);
    element->GetHash(kAttrDurationVar, &m_durationVar, m_durationVar);
}

// @func 0x004CB784 ApeAnimScriptInterpScaleCommand::~ApeAnimScriptInterpScaleCommand
// @func 0x004CB794 ApeAnimScriptInterpScaleCommand::~ApeAnimScriptInterpScaleCommand (deleting)
ApeAnimScriptInterpScaleCommand::~ApeAnimScriptInterpScaleCommand() = default;

// @func 0x004CB7D8 ApeAnimScriptInterpScaleCommand::Create
ApeAnimScriptCommand* ApeAnimScriptInterpScaleCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptInterpScaleCommand(element);
}

// TODO(0x004CB850 ApeAnimScriptInterpScaleCommand::Execute) starts a scale interpolator.

// ---------------------------------------------------------------------------
// CreateProcAnim

// @func 0x00793BA4 ApeAnimScriptCreateProcAnimCommand::~ApeAnimScriptCreateProcAnimCommand
// @func 0x00794498 ApeAnimScriptCreateProcAnimCommand::~ApeAnimScriptCreateProcAnimCommand (deleting)
ApeAnimScriptCreateProcAnimCommand::~ApeAnimScriptCreateProcAnimCommand() = default;

// @func 0x004CC0BC ApeAnimScriptCreateProcAnimCommand::Create
ApeAnimScriptCommand* ApeAnimScriptCreateProcAnimCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptCreateProcAnimCommand();
    command->m_object = ReadObjectOperand(element);
    command->m_flag71B29C0F = ReadBool(element, kAttrFlag71B29C0F, false);
    return command;
}

// TODO(0x004CC1BC ApeAnimScriptCreateProcAnimCommand::Execute)

// @func 0x004CC254 ApeAnimScriptCreateProcAnimCommand::CreateDebugElement
xml::XmlElement* ApeAnimScriptCreateProcAnimCommand::CreateDebugElement() const
{
    return nullptr;
}

// ---------------------------------------------------------------------------
// PlayGuiScript

// @func 0x004CC2D4 ApeAnimScriptPlayGuiScriptCommand::~ApeAnimScriptPlayGuiScriptCommand
// @func 0x004CC2E4 ApeAnimScriptPlayGuiScriptCommand::~ApeAnimScriptPlayGuiScriptCommand (deleting)
ApeAnimScriptPlayGuiScriptCommand::~ApeAnimScriptPlayGuiScriptCommand() = default;

// @func 0x004CC328 ApeAnimScriptPlayGuiScriptCommand::Create
ApeAnimScriptCommand* ApeAnimScriptPlayGuiScriptCommand::Create(xml::XmlElement* element) const
{
    auto* command = new ApeAnimScriptPlayGuiScriptCommand();
    element->GetHash(kAttrScript, &command->m_script, 0);
    element->GetHash(kAttrObject, &command->m_object, 0);
    return command;
}

// TODO(0x004CC3F4 ApeAnimScriptPlayGuiScriptCommand::Execute) plays a GUI animation.

// ---------------------------------------------------------------------------
// AttachObject

// @func 0x004B9ED8 ApeAnimScriptAttachObjectCommand::ApeAnimScriptAttachObjectCommand
ApeAnimScriptAttachObjectCommand::ApeAnimScriptAttachObjectCommand() = default;

// @func 0x004C30C8 ApeAnimScriptAttachObjectCommand::ApeAnimScriptAttachObjectCommand(xml)
ApeAnimScriptAttachObjectCommand::ApeAnimScriptAttachObjectCommand(xml::XmlElement* element)
{
    m_hasMatrixVar = element->GetHash(kAttrMatrix, &m_matrixVar, 0) == xml::kQuery_Ok;
    // Each operand is a name hash, or else a raw hash as an int.
    if (element->GetHash(kAttrObject, &m_object, 0) != xml::kQuery_Ok)
        element->GetInt(kAttrObjectHash, reinterpret_cast<int32_t*>(&m_object), 0);
    if (element->GetHash(AnimHash("target"), &m_target, 0) != xml::kQuery_Ok)
        element->GetInt(kAttrTargetHash, reinterpret_cast<int32_t*>(&m_target), 0);
    if (element->GetHash(AnimHash("locator"), &m_locator, 0) != xml::kQuery_Ok)
        element->GetInt(kAttrLocatorHash, reinterpret_cast<int32_t*>(&m_locator), 0);
    element->GetHash(kAttrPart, &m_part, 0);
    element->GetHash(kAttrAttachField1C, &m_field_1C, 0);

    bool value = false;
    element->GetBool(kAttrAttachFlag6, &value, m_flagE48AEDBD);
    m_flagE48AEDBD = value;
    element->GetBool(kAttrAttachFlag5, &value, m_flagFF40848E);
    m_flagFF40848E = value;
    element->GetBool(kAttrAttachFlag4, &value, m_flag96423570);
    m_flag96423570 = value;
    element->GetBool(AnimHash("inheritscale"), &value, m_inheritScale);
    m_inheritScale = value;
    element->GetBool(kAttrAttachFlag7, &value, m_flag72C27306);
    m_flag72C27306 = value;

    float pos[3] = {};
    float rot[3] = {};
    for (int i = 0; i < 3; ++i)
        element->GetFloat(kAttrPos[i], &pos[i], 0.0f);
    for (int i = 0; i < 3; ++i)
        element->GetFloat(kAttrRot[i], &rot[i], 0.0f);
    m_hasWorldX = element->GetFloat(kAttrWorld[0], &m_world[0], 0.0f) == xml::kQuery_Ok;
    m_hasWorldY = element->GetFloat(kAttrWorld[1], &m_world[1], 0.0f) == xml::kQuery_Ok;
    m_hasWorldZ = element->GetFloat(kAttrWorld[2], &m_world[2], 0.0f) == xml::kQuery_Ok;
    element->GetBool(kAttrAttachFlag3D, &value, false);
    m_flag7631EAF5 = value;

    // The offset only applies without a matrix variable.
    // NOTE: the console's vector code converting the rotation was not
    // decoded; the angles are kept as read.
    if (!m_hasMatrixVar) {
        for (int i = 0; i < 3; ++i) {
            m_offset[i] = pos[i];
            m_rotation[i] = rot[i];
        }
    }
}

// @func 0x004C3658 ApeAnimScriptAttachObjectCommand::Create
ApeAnimScriptCommand* ApeAnimScriptAttachObjectCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptAttachObjectCommand(element);
}

// ---------------------------------------------------------------------------
// MoveObject

// @func 0x004C4708 ApeAnimScriptMoveObjectCommand::ApeAnimScriptMoveObjectCommand(xml)
ApeAnimScriptMoveObjectCommand::ApeAnimScriptMoveObjectCommand(xml::XmlElement* element)
{
    if (element->GetHash(kAttrObject, &m_object, 0) != xml::kQuery_Ok)
        element->GetInt(kAttrObjectHash, reinterpret_cast<int32_t*>(&m_object), 0);
    m_hasMatrixVar = element->GetHash(kAttrMatrix, &m_matrixVar, 0) == xml::kQuery_Ok;
    m_hasTarget = element->GetHash(AnimHash("target"), &m_target, 0) == xml::kQuery_Ok;
    element->GetBool(kAttrLocal, &m_local, m_local);
    element->GetBool(kAttrAttachFlag6, &m_flagE48AEDBD, m_flagE48AEDBD);

    for (int i = 0; i < 3; ++i)
        if (element->GetFloat(kAttrPos[i], &m_position[i], 0.0f) == xml::kQuery_Ok)
            m_hasPosition = true;
    for (int i = 0; i < 3; ++i)
        if (element->GetFloat(kAttrRot[i], &m_rotation[i], 0.0f) == xml::kQuery_Ok)
            m_hasRotation = true;

    // Offset rotation: x, then y, then z, as quaternions of the half angles.
    // NOTE: angles taken as radians and the multiplication order assumed;
    // the console's vector code feeding FastSin/FastCos was not decoded.
    const float hx = m_rotation[0] * 0.5f;
    const float hy = m_rotation[1] * 0.5f;
    const float hz = m_rotation[2] * 0.5f;
    const Quat axes[3] = {
        {FastSin(hx), 0.0f, 0.0f, FastCos(hx)},
        {0.0f, FastSin(hy), 0.0f, FastCos(hy)},
        {0.0f, 0.0f, FastSin(hz), FastCos(hz)},
    };
    Quat xy;
    Quat xyz;
    QuatMul(xy, axes[0], axes[1]);
    QuatMul(xyz, xy, axes[2]);
    const Vec3 zero{0.0f, 0.0f, 0.0f, 0.0f};
    Matrix34FromQuat(m_offset, xyz, zero);

    for (int i = 0; i < 3; ++i)
        if (element->GetHash(kAttrVarPos[i], &m_posVars[i], 0) == xml::kQuery_Ok)
            m_hasPosition = true;
    for (int i = 0; i < 3; ++i)
        if (element->GetHash(kAttrVarRot[i], &m_rotVars[i], 0) == xml::kQuery_Ok)
            m_hasPosition = true;
}

// @func 0x004C4D9C ApeAnimScriptMoveObjectCommand::Create
ApeAnimScriptCommand* ApeAnimScriptMoveObjectCommand::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptMoveObjectCommand(element);
}

} // namespace ape
