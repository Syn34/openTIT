// openTIT - Component descriptions parsed from entity XML.
#include "sly/components/ComponentDescs.h"

#include "sly/components/ComponentHooks.h"
#include "sly/objects/TriggerEnv.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMem.h"
#include "ape/tuning/ApeTuning.h"
#include "ape/xml/ApeXml.h"

namespace sly {
namespace components {

namespace {

using ape::xml::XmlElement;

// Attribute name hashes (HashNameNoCase of the names). Names marked unknown
// had no match among the strings of the executable.
constexpr uint32_t kAttr_Set            = 0x00014042u;
constexpr uint32_t kAttr_Tree           = 0x00276B9Eu;
constexpr uint32_t kAttr_Behavior       = 0x88B4A252u;
constexpr uint32_t kAttr_Map            = 0x0001293Cu;
constexpr uint32_t kAttr_BlendMap       = 0xF690C76Bu;
constexpr uint32_t kAttr_Part           = 0x00255BF3u;
constexpr uint32_t kAttr_RotSpeed       = 0x2D3AC4D0u;
constexpr uint32_t kAttr_LockY          = 0x0453E88Eu;
constexpr uint32_t kAttr_FacePlayer1D   = 0x05AB7B55u;   // NOTE: name unknown
constexpr uint32_t kAttr_Type           = 0x0027873Au;
constexpr uint32_t kAttr_Radius         = 0x8FA16092u;
constexpr uint32_t kAttr_RefRadius      = 0x84323AC5u;
constexpr uint32_t kAttr_Height         = 0x7ECB8527u;
constexpr uint32_t kAttr_Mass           = 0x0023FEF4u;
constexpr uint32_t kAttr_StartEnabled   = 0xBEF25C9Fu;
constexpr uint32_t kAttr_MaterialObject = 0x4D0C3326u;
constexpr uint32_t kAttr_AnchorEntity   = 0x6EADF2D8u;
constexpr uint32_t kAttr_Script         = 0x9178C4CBu;
constexpr uint32_t kAttr_MinRange       = 0xDE1DA62Bu;
constexpr uint32_t kAttr_MaxRange       = 0x47FC4259u;
constexpr uint32_t kAttr_MinAngle       = 0xDD33E5C1u;
constexpr uint32_t kAttr_MaxAngle       = 0x471281EFu;
constexpr uint32_t kAttr_Strength       = 0x996B4041u;
constexpr uint32_t kAttr_Score          = 0x04B14672u;
constexpr uint32_t kAttr_EndPos         = 0x7A29AB99u;
constexpr uint32_t kAttr_StartVel       = 0x7D22FFFBu;
constexpr uint32_t kAttr_Damp           = 0x001FE6E0u;
constexpr uint32_t kAttr_Speed          = 0x04B70827u;
constexpr uint32_t kAttr_Cracker18      = 0xCC2ABC8Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_Damping        = 0x807548E2u;
constexpr uint32_t kAttr_OnDuration     = 0x476DF873u;
constexpr uint32_t kAttr_OffDuration    = 0x0148AFE3u;
constexpr uint32_t kAttr_TimeOffset     = 0x7B2F95A0u;
constexpr uint32_t kAttr_BoneName       = 0xA424A34Fu;
constexpr uint32_t kAttr_EntityName     = 0xC490A0EEu;
constexpr uint32_t kAttr_Grow           = 0x00218413u;
constexpr uint32_t kAttr_BallRoll28     = 0x5D1A6CBAu;   // NOTE: name unknown
constexpr uint32_t kAttr_BallRoll2C     = 0x2BCD00F1u;   // NOTE: name unknown
constexpr uint32_t kAttr_DamageSeq18    = 0xBF260CF3u;   // NOTE: name unknown
constexpr uint32_t kAttr_InitTime       = 0x3EB49FDDu;
constexpr uint32_t kAttr_EnabledTime    = 0x2B6FE96Eu;
constexpr uint32_t kAttr_DisabledTime   = 0x4FA1D749u;
constexpr uint32_t kAttr_DamageSeq29    = 0x173A6338u;   // NOTE: name unknown

constexpr uint32_t kAttr_MaxDepth        = 0x4738D3DFu;
constexpr uint32_t kAttr_FloatSpeed      = 0xE165E36Bu;
constexpr uint32_t kAttr_SinkSpeed       = 0x543E5574u;
constexpr uint32_t kAttr_Buoyancy20      = 0xFFF7B927u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy24      = 0x7F154347u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy28      = 0x827EF485u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy3C      = 0x4801705Eu;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy40      = 0xCD0D2AF5u;   // NOTE: name unknown
constexpr uint32_t kAttr_Buoyancy44      = 0x708A4E60u;   // NOTE: name unknown
constexpr uint32_t kAttr_TiltSpeed       = 0x8E11EB8Au;
constexpr uint32_t kAttr_Buoyancy4C      = 0xC4F41CF6u;   // NOTE: name unknown
constexpr uint32_t kAttr_MaxRotationX    = 0x735FF3D6u;
constexpr uint32_t kAttr_MaxRotationY    = 0x735FF3D7u;
constexpr uint32_t kAttr_MaxRotationZ    = 0x735FF3D8u;
constexpr uint32_t kAttr_ContactScript   = 0x143774CBu;
constexpr uint32_t kAttr_ContactInterval = 0x1E7884C5u;
constexpr uint32_t kAttr_GrabAngleH      = 0xB5292F41u;
constexpr uint32_t kAttr_MaxAngleV       = 0x9B3DBC47u;
constexpr uint32_t kAttr_MaxAngleH       = 0x9B3DBC39u;
constexpr uint32_t kAttr_GrabDistance    = 0x53A7F241u;
constexpr uint32_t kAttr_MaxDistance     = 0x06CD2139u;
constexpr uint32_t kAttr_StickThreshold  = 0x82E789DBu;
constexpr uint32_t kAttr_WallClimb50     = 0x977C8AB2u;   // NOTE: name unknown
constexpr uint32_t kAttr_WallClimb54     = 0xB081C3B4u;   // NOTE: name unknown
constexpr uint32_t kAttr_WallClimb58     = 0x01B279B3u;   // NOTE: name unknown
constexpr uint32_t kAttr_WallClimb60     = 0x009DAD78u;   // NOTE: name unknown

constexpr uint32_t kAttr_Size            = 0x0026D801u;
constexpr uint32_t kAttr_Offset          = 0x8ACA5893u;
constexpr uint32_t kAttr_Enabled         = 0xCADB1721u;
constexpr uint32_t kAttr_Physics91       = 0x8BB0F765u;   // NOTE: name unknown

constexpr uint32_t kAttr_Marker14        = 0x457D8076u;   // NOTE: name unknown
constexpr uint32_t kAttr_Marker18        = 0x4EA1C7E6u;   // NOTE: name unknown
constexpr uint32_t kAttr_MarkerRange     = 0x4852D5AFu;   // NOTE: name unknown
constexpr uint32_t kAttr_OnscreenColor   = 0x7087CB18u;
constexpr uint32_t kAttr_OffscreenColor  = 0x83D09DA8u;
constexpr uint32_t kAttr_Marker28        = 0xF6DB9816u;   // NOTE: name unknown

constexpr uint32_t kTag_ClosetSpawn      = 0x7480D3BAu;   // NOTE: element name unknown
constexpr uint32_t kAttr_Def             = 0x000107E5u;
constexpr uint32_t kAttr_SpawnPoint      = 0x6D4342F5u;
constexpr uint32_t kAttr_RespawnCooldown = 0x7307C873u;
constexpr uint32_t kAttr_RespawnEnabled  = 0x83CA0519u;
// Spawn kinds by "Type" (0x00AD538C): none, "Mine".
constexpr uint32_t kClosetSpawnTypes[2] = {0u, 0x00241C53u};

constexpr uint32_t kAttr_Name            = 0x0024728Bu;
constexpr uint32_t kTag_JointTwoSolver   = 0x75215015u;
constexpr uint32_t kTag_JointThreeSolver = 0x6AD11467u;
constexpr uint32_t kTag_FootSolver       = 0x895ACE01u;
constexpr uint32_t kTag_LookAtSolver     = 0x5BDDD9A5u;

constexpr uint32_t kAttr_AttachRot      = 0x898B7952u;
constexpr uint32_t kAttr_AttachOffset   = 0x3818B318u;
constexpr uint32_t kAttr_AttachCameraOffset = 0xE8B802BDu;
constexpr uint32_t kAttr_AttachPart     = 0xA7E293B8u;
constexpr uint32_t kAttr_GeomDef        = 0x26161B29u;
constexpr uint32_t kAttr_ScaleMin       = 0xF97D90C8u;
constexpr uint32_t kAttr_ScaleMax       = 0xF97D8FDAu;
constexpr uint32_t kAttr_ScaleDistMin   = 0x0F8DA662u;
constexpr uint32_t kAttr_ScaleDistMax   = 0x0F8DA574u;
constexpr uint32_t kAttr_ScaleFactorFOV = 0xFDF98B14u;
constexpr uint32_t kAttr_ApplyRotation  = 0xBC175CCCu;

constexpr uint32_t kAttr_ProcAnim31      = 0x002480E8u;   // NOTE: name unknown
constexpr uint32_t kAttr_ProcAnim30      = 0x724F4DDAu;   // NOTE: name unknown
constexpr uint32_t kAttr_Iterate         = 0xA8F0945Au;
constexpr uint32_t kAttr_Spring          = 0x922FF60Du;
constexpr uint32_t kAttr_DampingWorld    = 0xE42E17D0u;
constexpr uint32_t kAttr_DampingLocal    = 0xE392DBA9u;
constexpr uint32_t kAttr_AngleMax        = 0xFBCA07B1u;
constexpr uint32_t kAttr_Gravity         = 0x3B83FE2Eu;
constexpr uint32_t kAttr_ProcAnim50      = 0x34BF0B12u;   // NOTE: name unknown
constexpr uint32_t kTag_ProcAnimBone     = 0xA0702B13u;   // NOTE: element name unknown

constexpr uint32_t kAttr_PathType        = 0x784848DFu;
constexpr uint32_t kAttr_MotionType      = 0xF2006FF0u;
constexpr uint32_t kAttr_PauseDelayMin   = 0x7689C1C5u;
constexpr uint32_t kAttr_PauseDelayMax   = 0x7689C0D7u;
constexpr uint32_t kAttr_Mover28         = 0x86BE6BD3u;   // NOTE: name unknown
constexpr uint32_t kAttr_Mover29         = 0xE9B00400u;   // NOTE: name unknown

constexpr uint32_t kAttr_CombatHashes[6] = {0xFB92D658u, 0xFB92E002u, 0xFB929D5Au,
                                            0xBF173D65u, 0xBF17470Fu, 0xBF170467u};   // NOTE: names unknown
constexpr uint32_t kAttr_Combat2C        = 0x7CDB801Cu;   // NOTE: name unknown
constexpr uint32_t kTag_Reaction         = 0xFAF0CA29u;
constexpr uint32_t kReactionDefault      = 0x86DF6221u;   // "default"

constexpr uint32_t kAttr_Interact20      = 0x3B6C1F30u;   // NOTE: name unknown
constexpr uint32_t kAttr_Interact30      = 0x98DB3873u;   // NOTE: name unknown
constexpr uint32_t kAttr_InteractDist    = 0xD98B80DCu;
constexpr uint32_t kAttr_AngleThreshold  = 0x7D2B8CF8u;
constexpr uint32_t kAttr_Interact48      = 0x17A28676u;   // NOTE: name unknown
constexpr uint32_t kAttr_ScriptLifetime  = 0xF89FDD94u;
constexpr uint32_t kAttr_Interact50      = 0x3A20269Eu;   // NOTE: name unknown
constexpr uint32_t kAttr_StartAvailable  = 0xC5B578A7u;
constexpr uint32_t kAttr_InteractFlags[7] = {0xA9991533u, 0x9F79B1C1u, 0x2EC1E492u, 0xD499961Cu,
                                             0x7EC7743Bu, 0x10823996u, 0xEFDCC30Cu};   // NOTE: names unknown
constexpr uint32_t kAttr_ActiveNoCull    = 0x0CD89539u;
constexpr uint32_t kTag_PhysicsState     = 0x51609AAAu;
constexpr uint32_t kAttr_Enable          = 0x7A281983u;
constexpr uint32_t kAttr_InteractText    = 0x6E2EE341u;   // NOTE: name unknown
constexpr uint32_t kPhysicsStateNames[7] = {0x0024A738u /* None */, 0x7DD0CD87u /* Ground */,
                                            0x0000FD2Au /* Air */, 0x03D3CE95u /* Climb */,
                                            0x03D66C07u /* Crawl */, 0x9EA8BD98u /* PlayDead */,
                                            0x419905DFu /* ClimbWall */};

constexpr uint32_t kAttr_Simple          = 0x91CB25B2u;
constexpr uint32_t kAttr_Raycast         = 0x65AF9A49u;
constexpr uint32_t kAttr_PartName        = 0x75849CFEu;
constexpr uint32_t kAttr_OffsetPos       = 0x313AB0C1u;
constexpr uint32_t kAttr_TargetingC0     = 0xC333F453u;   // NOTE: name unknown
constexpr uint32_t kAttr_TargetingD0     = 0x69EC8F92u;   // NOTE: name unknown
constexpr uint32_t kAttr_UseCamera       = 0xA0D5174Cu;
constexpr uint32_t kAttr_MaxTargets      = 0xA3F9EF1Eu;
constexpr uint32_t kAttr_Angle           = 0x03B880B3u;
constexpr uint32_t kAttr_HeightRange     = 0x9C819064u;   // NOTE: name unknown
constexpr uint32_t kAttr_Targeting14     = 0xF5DFB24Au;   // NOTE: name unknown
// Targeting kinds by child element name (0x00AD4D34).
constexpr uint32_t kTargetingKinds[6] = {0x96EC7B40u, 0x036166E2u, 0x04C1AEC6u /* Throw */,
                                         0x50A58636u /* Interact */, 0x03CEF0FBu /* Carry */,
                                         0x6362ED0Au /* Collect */};

constexpr uint32_t kAttr_ProcEyes11      = 0x2A9B2CB7u;   // NOTE: name unknown
constexpr uint32_t kTag_Eye              = 0x00010E11u;
constexpr uint32_t kAttr_Material        = 0x40795527u;
constexpr uint32_t kAttr_Eye08           = 0xE79D4302u;   // NOTE: name unknown
constexpr uint32_t kAttr_Bone            = 0x001F32C4u;
constexpr uint32_t kAttr_EyelidBone      = 0x980FFA7Au;
constexpr uint32_t kAttr_Eye14           = 0x1A1D3D81u;   // NOTE: name unknown
constexpr uint32_t kAttr_Eye18           = 0x98247D0Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_EnableBlink     = 0x2B501799u;
constexpr uint32_t kAttr_BlinkMin        = 0xD9DE2696u;   // NOTE: name unknown
constexpr uint32_t kAttr_BlinkMax        = 0xD9DE25A8u;   // NOTE: name unknown
constexpr uint32_t kAttr_Blink28         = 0xD211F8D0u;   // NOTE: name unknown
constexpr uint32_t kAttr_UpperRotPivotU  = 0xC4E89AA8u;
constexpr uint32_t kAttr_UpperRotPivotV  = 0xC4E89AA9u;
constexpr uint32_t kAttr_LowerRotPivotU  = 0x1B750329u;
constexpr uint32_t kAttr_LowerRotPivotV  = 0x1B75032Au;

constexpr uint32_t kTag_Cloak            = 0x03D3E3B0u;
constexpr uint32_t kTag_Electricity      = 0xB96F8651u;
constexpr uint32_t kTag_Fire             = 0x0020EE36u;
constexpr uint32_t kTag_Poison           = 0x8CFF6308u;
constexpr uint32_t kTag_Ice              = 0x00011A6Bu;
constexpr uint32_t kTag_FxGlow           = 0x654ADD4Fu;   // NOTE: element name unknown
constexpr uint32_t kTag_SlowTime         = 0xEE1B444Eu;
constexpr uint32_t kAttr_ObjName         = 0xC7E35FE2u;
constexpr uint32_t kAttr_DefName         = 0x86E4FFF0u;
constexpr uint32_t kAttr_Timeout         = 0xDC7AD941u;
constexpr uint32_t kAttr_PowerCost       = 0xD87F3E32u;
constexpr uint32_t kAttr_Glow84          = 0x71D1F169u;   // NOTE: name unknown
constexpr uint32_t kAttr_GlowScale       = 0x1BAD0B40u;   // NOTE: name unknown
constexpr uint32_t kAttr_ColorScale[4]   = {0xF0828D6Bu, 0xF0828D60u, 0xF0828D5Bu, 0xF0828D5Au};   // R, G, B, A
constexpr uint32_t kAttr_GlowB0          = 0xA621B465u;   // NOTE: name unknown
constexpr uint32_t kAttr_GlowB1          = 0x6E828D95u;   // NOTE: name unknown
constexpr uint32_t kAttr_TimeScale       = 0xA9597E7Du;
constexpr uint32_t kAttr_SlowTimeD0      = 0x4F113DAEu;   // NOTE: name unknown
constexpr uint32_t kAttr_SlowTimeD4      = 0x173BA787u;   // NOTE: name unknown
constexpr uint32_t kAttr_SlowTimeD8      = 0x4E1FBF3Au;   // NOTE: name unknown

constexpr uint32_t kAttr_ChairType       = 0xF4EF8B5Fu;
constexpr uint32_t kAttr_ChairBone       = 0xF4E736E9u;
constexpr uint32_t kAttr_WheelBoneL      = 0x7F6B2FADu;
constexpr uint32_t kAttr_WheelBoneR      = 0x7F6B2FB3u;
constexpr uint32_t kAttr_Wheelchair24    = 0xCF9ADB4Cu;   // NOTE: name unknown
constexpr uint32_t kAttr_Parent          = 0x8C3DFCAAu;
constexpr uint32_t kTag_Arm              = 0x0000FE3Cu;
constexpr uint32_t kTag_Slave            = 0x04B529B7u;
constexpr uint32_t kTag_Root             = 0x002678E2u;
constexpr uint32_t kTag_Bone             = 0x001F32C4u;

constexpr uint32_t kAttr_InitScript      = 0x620515DBu;
constexpr uint32_t kAttr_OnActivateScript = 0x5D6F9F1Du;
constexpr uint32_t kAttr_Radar44         = 0x7C4EBDF7u;   // NOTE: name unknown
constexpr uint32_t kAttr_CompassScale    = 0xDEB00F78u;
constexpr uint32_t kAttr_Radar4C         = 0xC9796122u;   // NOTE: name unknown
constexpr uint32_t kAttr_Radar50         = 0xA7FC59B2u;   // NOTE: name unknown
constexpr uint32_t kAttr_ActiveDuration  = 0x80244A9Au;
constexpr uint32_t kAttr_RefreshDuration = 0x1867714Fu;
constexpr uint32_t kAttr_IgnoreEntities  = 0xC09B3793u;
constexpr uint32_t kAttr_RadarDefValue   = 0x7BA7CDEFu;   // NOTE: name unknown
constexpr uint32_t kTag_RadarDef         = 0xDE886EE4u;   // NOTE: element name unknown
constexpr uint32_t kTag_MarkerType       = 0x5F746274u;
constexpr uint32_t kMarkerTypeDefault    = 0x7DDC7E2Du;   // NOTE: name unknown
// Radar modes, read by name from <MarkerType> (0x00F64BF0).
const char* const kRadarModeNames[9] = {"location", "entity", "binoPhoto", "binoRecord", "hacking",
                                        "vehicle", "blueprint", "follow", "switch"};

constexpr uint32_t kTag_ReceptacleSlot   = 0xA7E419C2u;   // NOTE: element name unknown
constexpr uint32_t kTag_ReceptacleDef    = 0xD5B4E11Bu;   // NOTE: element name unknown
constexpr uint32_t kAttr_Pos             = 0x00013634u;
constexpr uint32_t kAttr_Rot             = 0x00013DB7u;
constexpr uint32_t kAttr_EntityDef       = 0x79F40122u;
constexpr uint32_t kAttr_LockEntities    = 0x4BEF99ECu;

constexpr float kDegToRad = 0.017453292f;

// Shared start of every parser: the description named by the element's
// "name" attribute, or null (after the compiled-out error) without one.
template <class Desc>
Desc* NewNamedDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    Desc* desc = new Desc(ape::HashNameNoCase(name));
    desc->scope = scope;
    return desc;
}

// "Name" of up to `count` children with tag `tag`, in order.
void ReadChildNames(XmlElement* elem, uint32_t tag, uint32_t* out, uint32_t count)
{
    XmlElement* child = elem->FirstChildElement(tag);
    for (uint32_t i = 0; child && i < count; ++i) {
        uint32_t value = 0;
        child->GetHash(kAttr_Name, &value, 0);
        out[i] = value;
        child = child->NextSiblingElement(tag);
    }
}

void ReadPowerFx(XmlElement* child, PowerFx& fx)
{
    child->GetHash(kAttr_Script, &fx.script, 0);
    child->GetHash(kAttr_ObjName, &fx.objName, 0);
    child->GetHash(kAttr_DefName, &fx.defName, 0);
    child->GetFloat(kAttr_Timeout, &fx.timeout, 0.0f);
}

void ReadPulseField(XmlElement* elem, PulseFieldComponentDesc& desc)
{
    elem->GetFloat(kAttr_Radius, &desc.radius, desc.radius);
    elem->GetFloat(kAttr_Strength, &desc.strength, desc.strength);
    elem->GetFloat(kAttr_Damping, &desc.damping, desc.damping);
    elem->GetFloat(kAttr_OnDuration, &desc.onDuration, desc.onDuration);
    elem->GetFloat(kAttr_OffDuration, &desc.offDuration, desc.offDuration);
    elem->GetFloat(kAttr_TimeOffset, &desc.timeOffset, desc.timeOffset);
}

} // namespace

// Radar defaults. NOTE: these belong to the component static initialiser
// 0x002D6A40 (not ported), which registers them with the other component tuning.
ape::TuningFloat g_radarDefaultActiveDuration("EntityComp.Radar.DefaultActiveDuration", 8.0f);
ape::TuningFloat g_radarDefaultRefreshDuration("EntityComp.Radar.DefaultRefreshDuration", 8.0f);

// @func 0x0026EFD4 ParsePhysicsComponentDesc
ComponentDesc* ParsePhysicsComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    PhysicsComponentDesc* desc = NewNamedDesc<PhysicsComponentDesc>(root, elem, scope);
    if (desc) {
        desc->world = LevelPhysicsWorld();
        elem->GetFloat(kAttr_Mass, &desc->mass, desc->mass);
        elem->GetVec3(kAttr_Size, &desc->size.x, &desc->size.x);
        elem->GetVec3(kAttr_Offset, &desc->offset.x, &desc->offset.x);
        elem->GetBool(kAttr_Enabled, &desc->enabled, desc->enabled);
        elem->GetBool(kAttr_Physics91, &desc->flag91, desc->flag91);
    }
    return desc;
}

// @func 0x0026F2D0 ParseTargetingComponentDesc
ComponentDesc* ParseTargetingComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    TargetingComponentDesc* desc = NewNamedDesc<TargetingComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    elem->GetBool(kAttr_Simple, &desc->simple, desc->simple);
    elem->GetBool(kAttr_Raycast, &desc->raycast, desc->raycast);
    elem->GetHash(kAttr_BoneName, &desc->boneName, desc->boneName);
    elem->GetHash(kAttr_PartName, &desc->partName, desc->partName);
    elem->GetVec3(kAttr_OffsetPos, &desc->offsetPos.x, &desc->offsetPos.x);
    elem->GetVec3(kAttr_TargetingC0, &desc->vecC0.x, &desc->vecC0.x);
    elem->GetBool(kAttr_TargetingD0, &desc->flagD0, desc->flagD0);
    elem->GetBool(kAttr_UseCamera, &desc->useCamera, desc->useCamera);
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        uint32_t kind = 0;
        while (kind < 6 && child->GetNameHash() != kTargetingKinds[kind])
            ++kind;
        if (kind >= 6) {
            (void)root->GetDocumentPath();   // compiled-out error report
            continue;
        }
        TargetingRange& range = desc->ranges[kind];
        child->GetInt(kAttr_MaxTargets, &range.maxTargets, range.maxTargets);
        child->GetFloat(kAttr_Radius, &range.radius, range.radius);
        child->GetFloat(kAttr_Angle, &range.angle, range.angle);
        child->GetVec2(kAttr_HeightRange, range.heightRange, range.heightRange);
        child->GetFloat(kAttr_Targeting14, &range.value14, range.value14);
    }
    return desc;
}

// @func 0x002975F4 ParseAnimComponentDesc
ComponentDesc* ParseAnimComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    AnimComponentDesc* desc = NewNamedDesc<AnimComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Set, &desc->animSet, desc->animSet);
        elem->GetHash(kAttr_Tree, &desc->animTree, desc->animTree);
    }
    return desc;
}

// @func 0x002745B8 ParseCombatComponentDesc
ComponentDesc* ParseCombatComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    CombatComponentDesc* desc = new CombatComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    for (int i = 0; i < 6; ++i)
        elem->GetHash(kAttr_CombatHashes[i], &desc->hashes[i], desc->hashes[i]);
    elem->GetFloat(kAttr_Combat2C, &desc->value2C, desc->value2C);
    for (XmlElement* child = elem->FirstChildElement(kTag_Reaction); child;
         child = child->NextSiblingElement(kTag_Reaction)) {
        uint32_t type = 0, anim = 0;
        if (child->GetHash(kAttr_Type, &type, 0) != ape::xml::kQuery_Ok ||
            child->GetHash(kAttr_Name, &anim, 0) != ape::xml::kQuery_Ok) {
            (void)root->GetDocumentPath();   // compiled-out error report
            continue;
        }
        if (type == kReactionDefault)
            type = 0;
        if (*desc->reactions.Find(type) != 0xFFFFFFFFu) {
            // NOTE: the compiled data carries no line numbers; the console
            // prints the element row + 1.
            ape::TempFormat("%s(%ld): Combat component '%s': duplicate Reaction type '%s' specifiers.",
                            root->GetDocumentPath(), 0L, name, ape::TempHexString(type));
        }
        desc->reactions.Add(type, anim);
    }
    return desc;
}

// @func 0x0027B648 ParseContextualMarkerComponentDesc
ComponentDesc* ParseContextualMarkerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    ContextualMarkerComponentDesc* desc = new ContextualMarkerComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    const float zero[3] = {0.0f, 0.0f, 0.0f};
    ape::Vec3 euler = {0.0f, 0.0f, 0.0f, 0.0f};
    if (elem->GetVec3(kAttr_AttachRot, &euler.x, zero) == ape::xml::kQuery_Ok) {
        const ape::Vec3 radians = {euler.x * kDegToRad, euler.y * kDegToRad, euler.z * kDegToRad, 0.0f};
        desc->attachRot = objects::QuaternionFromEuler(radians);
    }
    elem->GetVec3(kAttr_AttachOffset, &desc->attachOffset.x, &desc->attachOffset.x);
    elem->GetVec3(kAttr_AttachCameraOffset, &desc->attachCameraOffset.x, &desc->attachCameraOffset.x);
    elem->GetHash(kAttr_AttachPart, &desc->attachPart, desc->attachPart);
    elem->GetHash(kAttr_GeomDef, &desc->geomDef, desc->geomDef);
    elem->GetFloat(kAttr_ScaleMin, &desc->scaleMin, desc->scaleMin);
    elem->GetFloat(kAttr_ScaleMax, &desc->scaleMax, desc->scaleMax);
    elem->GetFloat(kAttr_ScaleDistMin, &desc->scaleDistMin, desc->scaleDistMin);
    elem->GetFloat(kAttr_ScaleDistMax, &desc->scaleDistMax, desc->scaleDistMax);
    desc->hasScaleFactorFOV =
        elem->GetFloat(kAttr_ScaleFactorFOV, &desc->scaleFactorFOV, desc->scaleFactorFOV) == ape::xml::kQuery_Ok;
    elem->GetBool(kAttr_ApplyRotation, &desc->applyRotation, desc->applyRotation);
    elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, desc->startEnabled);
    return desc;
}

// @func 0x0027CA10 ParseControlAIComponentDesc
ComponentDesc* ParseControlAIComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ControlAIComponentDesc* desc = NewNamedDesc<ControlAIComponentDesc>(root, elem, scope);
    if (desc)
        elem->GetHash(kAttr_Behavior, &desc->behavior, desc->behavior);
    return desc;
}

// @func 0x002879AC ParseAnimMapComponentDesc
ComponentDesc* ParseAnimMapComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    AnimMapComponentDesc* desc = NewNamedDesc<AnimMapComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Map, &desc->map, desc->map);
        elem->GetHash(kAttr_Tree, &desc->tree, desc->tree);
        elem->GetHash(kAttr_BlendMap, &desc->blendMap, desc->blendMap);
        elem->GetHash(kAttr_Set, &desc->animSet, desc->animSet);
    }
    return desc;
}

// @func 0x00283D58 ParseProcAnimComponentDesc
ComponentDesc* ParseProcAnimComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ProcAnimComponentDesc* desc = NewNamedDesc<ProcAnimComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    elem->GetBool(kAttr_ProcAnim31, &desc->flag31, desc->flag31);
    elem->GetBool(kAttr_ProcAnim30, &desc->flag30, desc->flag30);
    int32_t iterate = desc->iterate;
    elem->GetInt(kAttr_Iterate, &iterate, iterate);
    desc->iterate = (uint8_t)iterate;
    elem->GetFloat(kAttr_Spring, &desc->spring, desc->spring);
    elem->GetFloat(kAttr_DampingWorld, &desc->dampingWorld, desc->dampingWorld);
    elem->GetFloat(kAttr_DampingLocal, &desc->dampingLocal, desc->dampingLocal);
    elem->GetFloat(kAttr_AngleMax, &desc->angleMax, desc->angleMax);
    elem->GetVec3(kAttr_Gravity, &desc->gravity.x, &desc->gravity.x);
    elem->GetVec3(kAttr_ProcAnim50, &desc->vec50.x, &desc->vec50.x);
    for (XmlElement* child = elem->FirstChildElement(kTag_ProcAnimBone); child;
         child = child->NextSiblingElement(kTag_ProcAnimBone)) {
        uint32_t bone = 0;
        if (child->GetHash(kAttr_BoneName, &bone, 0) == ape::xml::kQuery_Ok)
            desc->bones.PushBack(bone);
        else
            (void)root->GetDocumentPath();   // compiled-out error report
    }
    return desc;
}

// @func 0x00286378 ParseProcEyesComponentDesc
ComponentDesc* ParseProcEyesComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ProcEyesComponentDesc* desc = NewNamedDesc<ProcEyesComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    elem->GetBool(kAttr_ProcEyes11, &desc->flag11, false);
    // Missing required attributes only trigger the compiled-out error report.
    auto require = [root](int result) {
        if (result != ape::xml::kQuery_Ok)
            (void)root->GetDocumentPath();
    };
    for (XmlElement* child = elem->FirstChildElement(kTag_Eye); child;
         child = child->NextSiblingElement(kTag_Eye)) {
        ProcEye eye;
        require(child->GetHash(kAttr_Name, &eye.name, 0));
        require(child->GetHash(kAttr_Material, &eye.material, 0));
        child->GetHash(kAttr_Eye08, &eye.hash08, 0);
        require(child->GetHash(kAttr_Bone, &eye.bone, 0));
        require(child->GetHash(kAttr_Eye14, &eye.hash14, 0));
        require(child->GetHash(kAttr_EyelidBone, &eye.eyelidBone, 0));
        require(child->GetHash(kAttr_Eye18, &eye.hash18, 0));
        require(child->GetBool(kAttr_EnableBlink, &eye.enableBlink, false));
        if (eye.enableBlink) {
            require(child->GetFloat(kAttr_BlinkMin, &eye.blinkMin, 0.0f));
            require(child->GetFloat(kAttr_BlinkMax, &eye.blinkMax, 0.0f));
            require(child->GetFloat(kAttr_Blink28, &eye.blink28, 0.0f));
        }
        child->GetFloat(kAttr_UpperRotPivotU, &eye.upperRotPivot[0], eye.upperRotPivot[0]);
        child->GetFloat(kAttr_UpperRotPivotV, &eye.upperRotPivot[1], eye.upperRotPivot[1]);
        child->GetFloat(kAttr_LowerRotPivotU, &eye.lowerRotPivot[0], eye.lowerRotPivot[0]);
        child->GetFloat(kAttr_LowerRotPivotV, &eye.lowerRotPivot[1], eye.lowerRotPivot[1]);
        desc->eyes.PushBack(eye);
    }
    return desc;
}

// @func 0x00291AD0 ParseIKComponentDesc
ComponentDesc* ParseIKComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    IKComponentDesc* desc = NewNamedDesc<IKComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    ReadChildNames(elem, kTag_JointTwoSolver, desc->jointTwoSolvers, 8);
    ReadChildNames(elem, kTag_JointThreeSolver, desc->jointThreeSolvers, 8);
    ReadChildNames(elem, kTag_FootSolver, desc->footSolvers, 2);
    if (XmlElement* lookAt = elem->FirstChildElement(kTag_LookAtSolver)) {
        uint32_t value = 0;
        lookAt->GetHash(kAttr_Name, &value, 0);
        desc->lookAtSolver = value;
    }
    return desc;
}

// @func 0x0028F080 ParseFacePlayerComponentDesc
ComponentDesc* ParseFacePlayerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    FacePlayerComponentDesc* desc = NewNamedDesc<FacePlayerComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Part, &desc->part, desc->part);
        if (elem->GetFloat(kAttr_RotSpeed, &desc->rotSpeed, desc->rotSpeed) == ape::xml::kQuery_Ok)
            desc->rotSpeed *= kDegToRad;
        elem->GetBool(kAttr_LockY, &desc->lockY, desc->lockY);
        elem->GetBool(kAttr_FacePlayer1D, &desc->flag1D, desc->flag1D);
    }
    return desc;
}

// @func 0x00290928 ParseFxComponentDesc
ComponentDesc* ParseFxComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    FxComponentDesc* desc = NewNamedDesc<FxComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        switch (child->GetNameHash()) {
        case kTag_FxGlow: {
            const float defaultScale[3] = {1.05f, 1.05f, 1.05f};
            child->GetHash(kAttr_ObjName, &desc->glowObjName, 0);
            child->GetFloat(kAttr_Glow84, &desc->glow84, 0.8f);
            child->GetVec3(kAttr_GlowScale, &desc->glowScale.x, defaultScale);
            child->GetFloat(kAttr_ColorScale[0], &desc->colorScale[0], 1.1f);
            child->GetFloat(kAttr_ColorScale[1], &desc->colorScale[1], 1.1f);
            child->GetFloat(kAttr_ColorScale[2], &desc->colorScale[2], 1.1f);
            child->GetFloat(kAttr_ColorScale[3], &desc->colorScale[3], 1.5f);
            child->GetBool(kAttr_GlowB0, &desc->glowB0, false);
            child->GetBool(kAttr_GlowB1, &desc->glowB1, false);
            break;
        }
        case kTag_Cloak:
            child->GetHash(kAttr_Script, &desc->cloak.script, 0);
            child->GetHash(kAttr_ObjName, &desc->cloak.objName, 0);
            child->GetHash(kAttr_DefName, &desc->cloak.defName, 0);
            child->GetFloat(kAttr_Timeout, &desc->cloak.timeout, 0.0f);
            child->GetFloat(kAttr_PowerCost, &desc->cloakPowerCost, 0.0f);
            break;
        case kTag_Fire:
            ReadPowerFx(child, desc->fire);
            break;
        case kTag_Ice:
            ReadPowerFx(child, desc->ice);
            break;
        case kTag_SlowTime:
            child->GetFloat(kAttr_Radius, &desc->slowTimeRadius, 0.0f);
            child->GetFloat(kAttr_Timeout, &desc->slowTimeTimeout, 0.0f);
            child->GetFloat(kAttr_TimeScale, &desc->slowTimeScale, 0.0f);
            child->GetFloat(kAttr_PowerCost, &desc->slowTimePowerCost, 0.0f);
            child->GetHash(kAttr_SlowTimeD0, &desc->slowTimeHashD0, 0);
            child->GetFloat(kAttr_SlowTimeD4, &desc->slowTimeD4, 1.0f);
            child->GetFloat(kAttr_SlowTimeD8, &desc->slowTimeD8, 0.0f);
            break;
        case kTag_Electricity:
            ReadPowerFx(child, desc->electricity);
            break;
        case kTag_Poison:
            ReadPowerFx(child, desc->poison);
            break;
        default:
            (void)root->GetDocumentPath();   // compiled-out error report
            break;
        }
    }
    return desc;
}

// @func 0x00299A60 ParseRadarComponentDesc
ComponentDesc* ParseRadarComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    RadarComponentDesc* desc = new RadarComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    desc->activeDuration = g_radarDefaultActiveDuration.Get();
    desc->refreshDuration = g_radarDefaultRefreshDuration.Get();
    elem->GetHash(kAttr_InitScript, &desc->initScript, desc->initScript);
    elem->GetHash(kAttr_OnActivateScript, &desc->onActivateScript, desc->onActivateScript);
    elem->GetHash(kAttr_Radar44, &desc->hash44, desc->hash44);
    elem->GetFloat(kAttr_CompassScale, &desc->compassScale, desc->compassScale);
    elem->GetHash(kAttr_Radar4C, &desc->hash4C, desc->hash4C);
    elem->GetHash(kAttr_Radar50, &desc->hash50, desc->hash50);
    elem->GetFloat(kAttr_ActiveDuration, &desc->activeDuration, desc->activeDuration);
    elem->GetFloat(kAttr_RefreshDuration, &desc->refreshDuration, desc->refreshDuration);
    bool ignore = false;
    elem->GetBool(kAttr_IgnoreEntities, &ignore, false);
    if (ignore)
        desc->ignoreEntities = 1;
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t tag = child->GetNameHash();
        if (tag == kTag_RadarDef) {
            RadarDefEntry entry;
            child->GetHash(kAttr_Def, &entry.def, 0);
            child->GetHash(kAttr_RadarDefValue, &entry.value, entry.value);
            desc->defs.PushBack(entry);
        } else if (tag == kTag_MarkerType && desc->markerTypes.Count() == 0) {
            // Only the first <MarkerType> counts.
            for (const char* mode : kRadarModeNames) {
                uint32_t type = kMarkerTypeDefault;
                child->GetHash(mode, &type, kMarkerTypeDefault);
                desc->markerTypes.PushBack(type);
            }
        }
    }
    return desc;
}

// @func 0x0029FAA0 ParseRadarBlipComponentDesc
ComponentDesc* ParseRadarBlipComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name) {
        // NOTE: the console still reads "Type" into the null description.
        return nullptr;
    }
    RadarBlipComponentDesc* desc = new RadarBlipComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetHash(kAttr_Type, &desc->blipType, desc->blipType);
    return desc;
}

// @func 0x002B6740 ParseInteractComponentDesc
ComponentDesc* ParseInteractComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    InteractComponentDesc* desc = new InteractComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetVec3(kAttr_Interact20, &desc->vec20.x, &desc->vec20.x);
    if (elem->GetVec3(kAttr_Interact30, &desc->rotation.x, &desc->rotation.x) == ape::xml::kQuery_Ok) {
        desc->rotation.x *= kDegToRad;
        desc->rotation.y *= kDegToRad;
        desc->rotation.z *= kDegToRad;
    }
    if (elem->GetFloat(kAttr_InteractDist, &desc->interactDistSq, desc->interactDistSq) == ape::xml::kQuery_Ok &&
        desc->interactDistSq > 0.0f)
        desc->interactDistSq *= desc->interactDistSq;
    if (elem->GetFloat(kAttr_AngleThreshold, &desc->halfAngleThreshold, desc->halfAngleThreshold) ==
        ape::xml::kQuery_Ok)
        desc->halfAngleThreshold = desc->halfAngleThreshold * kDegToRad * 0.5f;
    if (elem->GetFloat(kAttr_Interact48, &desc->halfAngle48, desc->halfAngle48) == ape::xml::kQuery_Ok)
        desc->halfAngle48 = desc->halfAngle48 * kDegToRad * 0.5f;
    elem->GetHash(kAttr_ScriptLifetime, &desc->scriptLifetime, 0);
    elem->GetFloat(kAttr_Interact50, &desc->value50, desc->value50);
    elem->GetBool(kAttr_StartAvailable, &desc->startAvailable, desc->startAvailable);
    for (int i = 0; i < 7; ++i)
        elem->GetBool(kAttr_InteractFlags[i], &desc->flags[i], desc->flags[i]);
    elem->GetBool(kAttr_ActiveNoCull, &desc->activeNoCull, desc->activeNoCull);

    // <PhysicsState Name=... Enable=...> edit the default mask (Ground).
    desc->physicsStates = 1u << kPhysState_Ground;
    for (XmlElement* child = elem->FirstChildElement(kTag_PhysicsState); child;
         child = child->NextSiblingElement(kTag_PhysicsState)) {
        uint32_t stateName = 0;
        if (child->GetHash(kAttr_Name, &stateName, 0) != ape::xml::kQuery_Ok)
            continue;
        for (uint32_t i = 0; i < 7; ++i) {
            if (kPhysicsStateNames[i] != stateName)
                continue;
            bool enable = true;
            child->GetBool(kAttr_Enable, &enable, true);
            if (enable)
                desc->physicsStates |= 1u << i;
            else
                desc->physicsStates &= ~(1u << i);
            break;
        }
    }
    if (const char* text = elem->GetString(kAttr_InteractText))
        desc->text = ape::StrDup(text);
    return desc;
}

// @func 0x002B8328 ParseReceptacleComponentDesc
ComponentDesc* ParseReceptacleComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ReceptacleComponentDesc* desc = NewNamedDesc<ReceptacleComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;   // NOTE: the console still reads LockEntities into the null description
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t tag = child->GetNameHash();
        if (tag == kTag_ReceptacleSlot) {
            ReceptacleSlot slot;
            child->GetHash(kAttr_Name, &slot.name, 0);
            child->GetHash(kAttr_Part, &slot.part, slot.part);
            child->GetVec3(kAttr_Pos, &slot.pos.x, &slot.pos.x);
            child->GetVec3(kAttr_Rot, &slot.rot.x, &slot.rot.x);
            slot.rot.x *= kDegToRad;
            slot.rot.y *= kDegToRad;
            slot.rot.z *= kDegToRad;
            desc->slots.PushBack(slot);
        } else if (tag == kTag_ReceptacleDef) {
            uint32_t def = 0;
            child->GetHash(kAttr_EntityDef, &def, 0);
            if (def != 0 && desc->acceptedDefs.Count() < 0xC)
                desc->acceptedDefs.PushBack(def);
        }
    }
    elem->GetBool(kAttr_LockEntities, &desc->lockEntities, desc->lockEntities);
    return desc;
}

// @func 0x002B9C74 ParseMoverComponentDesc
ComponentDesc* ParseMoverComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    MoverComponentDesc* desc = NewNamedDesc<MoverComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    desc->pointCount = 0;
    elem->GetHash(kAttr_PathType, &desc->pathType, desc->pathType);
    elem->GetHash(kAttr_MotionType, &desc->motionType, desc->motionType);
    elem->GetFloat(kAttr_Speed, &desc->speed, desc->speed);
    elem->GetFloat(kAttr_PauseDelayMin, &desc->pauseDelayMin, desc->pauseDelayMin);
    elem->GetFloat(kAttr_PauseDelayMax, &desc->pauseDelayMax, desc->pauseDelayMax);
    elem->GetBool(kAttr_Mover28, &desc->flag28, desc->flag28);
    elem->GetBool(kAttr_Mover29, &desc->flag29, desc->flag29);
    const float zero[3] = {0.0f, 0.0f, 0.0f};
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (desc->pointCount >= 8)
            continue;
        MoverPoint& point = desc->points[desc->pointCount];
        point.tag = child->GetNameHash();
        child->GetHash(kAttr_Name, &point.name, 0);
        child->GetVec3(kAttr_Offset, &point.offset.x, zero);
        ++desc->pointCount;
    }
    return desc;
}

// @func 0x002BA548 ParseOffscreenMarkerComponentDesc
ComponentDesc* ParseOffscreenMarkerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    OffscreenMarkerComponentDesc* desc = new OffscreenMarkerComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, true);
    elem->GetHash(kAttr_Marker14, &desc->hash14, desc->hash14);
    elem->GetHash(kAttr_Marker18, &desc->hash18, desc->hash18);
    elem->GetFloat(kAttr_MarkerRange, &desc->rangeSq, desc->rangeSq);
    desc->hasOnscreenColor =
        elem->GetColor(kAttr_OnscreenColor, &desc->onscreenColor, &desc->onscreenColor) == ape::xml::kQuery_Ok;
    desc->hasOffscreenColor =
        elem->GetColor(kAttr_OffscreenColor, &desc->offscreenColor, &desc->offscreenColor) == ape::xml::kQuery_Ok;
    elem->GetBool(kAttr_Marker28, &desc->flag28, false);
    if (desc->rangeSq > 0.0f)
        desc->rangeSq *= desc->rangeSq;
    return desc;
}

// @func 0x002BB9FC ParseSimplePhysicsComponentDesc
ComponentDesc* ParseSimplePhysicsComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    SimplePhysicsComponentDesc* desc = NewNamedDesc<SimplePhysicsComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_Radius, &desc->radius, desc->radius);
        elem->GetFloat(kAttr_Height, &desc->height, desc->height);
        elem->GetFloat(kAttr_Mass, &desc->mass, desc->mass);
        elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, desc->startEnabled);
    }
    return desc;
}

// @func 0x002AF0D4 ParseJumpRopeComponentDesc
ComponentDesc* ParseJumpRopeComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    JumpRopeComponentDesc* desc = NewNamedDesc<JumpRopeComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_MaterialObject, &desc->materialObject, desc->materialObject);
        elem->GetHash(kAttr_AnchorEntity, &desc->anchorEntity, desc->anchorEntity);
    }
    return desc;
}

// @func 0x00280614 ParseAeSplineComponentDesc
ComponentDesc* ParseAeSplineComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    return NewNamedDesc<AeSplineComponentDesc>(root, elem, scope);
}

// @func 0x002BC208 ParseWheelchairComponentDesc
ComponentDesc* ParseWheelchairComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    WheelchairComponentDesc* desc = new WheelchairComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetInt(kAttr_ChairType, &desc->chairType, desc->chairType);
    elem->GetHash(kAttr_ChairBone, &desc->chairBone, 0);
    elem->GetHash(kAttr_WheelBoneL, &desc->wheelBoneL, 0);
    elem->GetHash(kAttr_WheelBoneR, &desc->wheelBoneR, 0);
    elem->GetHash(kAttr_Wheelchair24, &desc->hash24, 0);
    uint32_t arm = 0;
    for (XmlElement* armElem = elem->FirstChildElement(kTag_Arm); armElem;
         armElem = armElem->NextSiblingElement(kTag_Arm), ++arm) {
        if (arm > 1)
            break;   // a third arm ends the parse
        uint32_t boneCount = 0, slaveCount = 0;
        for (XmlElement* child = armElem->FirstChildElement(); child; child = child->NextSiblingElement()) {
            switch (child->GetNameHash()) {
            case kTag_Slave:
                if (slaveCount < 8) {
                    WheelchairSlave& slave = desc->armSlaves[arm][slaveCount++];
                    child->GetHash(kAttr_Name, &slave.name, 0);
                    child->GetHash(kAttr_Parent, &slave.parent, 0);
                } else {
                    ape::TempFormat("EntityCompWheelchair '%s': max # slaves is %d, extra definitions ignored.",
                                    name, 8);
                }
                break;
            case kTag_Root:
                if (desc->armRoot[arm] != 0)
                    ape::TempFormat("EntityCompWheelchair '%s': multiple Arm Root definitions.", name);
                child->GetHash(kAttr_Name, &desc->armRoot[arm], 0);
                break;
            case kTag_Bone:
                if (boneCount < 16) {
                    child->GetHash(kAttr_Name, &desc->armIkBones[arm][boneCount++], 0);
                } else {
                    ape::TempFormat("EntityCompWheelchair '%s': max # IK bones is %d, extra definitions ignored.",
                                    name, 16);
                }
                break;
            default:
                break;
            }
        }
    }
    return desc;
}

// @func 0x002CBB40 ParseTrackerComponentDesc
ComponentDesc* ParseTrackerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    TrackerComponentDesc* desc = new TrackerComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetHash(kAttr_Script, &desc->script, desc->script);
    elem->GetFloat(kAttr_MinRange, &desc->minRange, desc->minRange);
    elem->GetFloat(kAttr_MaxRange, &desc->maxRange, desc->maxRange);
    elem->GetFloat(kAttr_MinAngle, &desc->minAngle, desc->minAngle);
    elem->GetFloat(kAttr_MaxAngle, &desc->maxAngle, desc->maxAngle);
    elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, desc->startEnabled);
    return desc;
}

// @func 0x002B06D4 ParseVortexComponentDesc
ComponentDesc* ParseVortexComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    VortexComponentDesc* desc = NewNamedDesc<VortexComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_Radius, &desc->radius, desc->radius);
        elem->GetFloat(kAttr_Strength, &desc->strength, desc->strength);
    }
    return desc;
}

// @func 0x002B1D18 ParseScoringComponentDesc
ComponentDesc* ParseScoringComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ScoringComponentDesc* desc = NewNamedDesc<ScoringComponentDesc>(root, elem, scope);
    if (desc) {
        const float zero[3] = {0.0f, 0.0f, 0.0f};
        elem->GetFloat(kAttr_Score, &desc->score, desc->score);
        elem->GetVec3(kAttr_EndPos, &desc->endPos.x, zero);
        elem->GetFloat(kAttr_StartVel, &desc->startVel, 0.0f);
        elem->GetFloat(kAttr_Damp, &desc->damp, 5.0f);
        elem->GetFloat(kAttr_Speed, &desc->speed, 20.0f);
    }
    return desc;
}

// @func 0x002B1628 ParseSystemCrackerComponentDesc
ComponentDesc* ParseSystemCrackerComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    SystemCrackerComponentDesc* desc = NewNamedDesc<SystemCrackerComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_Script, &desc->script, desc->script);
        elem->GetBool(kAttr_Cracker18, &desc->flag18, false);
    }
    return desc;
}

// @func 0x002B2814 ParseMagNetComponentDesc
ComponentDesc* ParseMagNetComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    MagNetComponentDesc* desc = NewNamedDesc<MagNetComponentDesc>(root, elem, scope);
    if (desc)
        ReadPulseField(elem, *desc);
    return desc;
}

// @func 0x002B4E78 ParseSpeedBoostComponentDesc
ComponentDesc* ParseSpeedBoostComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    SpeedBoostComponentDesc* desc = NewNamedDesc<SpeedBoostComponentDesc>(root, elem, scope);
    if (desc)
        ReadPulseField(elem, *desc);
    return desc;
}

// @func 0x002D08C0 ParseBallRollComponentDesc
ComponentDesc* ParseBallRollComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    BallRollComponentDesc* desc = NewNamedDesc<BallRollComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetHash(kAttr_BoneName, &desc->boneName, 0);
        elem->GetHash(kAttr_EntityName, &desc->entityName, 0);
        // NOTE: both reads target the same field with a default of 0, so
        // "Radius" only survives when "RefRadius" is present too (console bug).
        elem->GetFloat(kAttr_Radius, &desc->radius, 0.0f);
        elem->GetFloat(kAttr_RefRadius, &desc->radius, 0.0f);
        elem->GetBool(kAttr_Grow, &desc->grow, false);
        elem->GetHash(kAttr_BallRoll28, &desc->hash28, 0);
        elem->GetHash(kAttr_BallRoll2C, &desc->hash2C, 0);
    }
    return desc;
}

// @func 0x002ACBF8 ParseBuoyancyComponentDesc
ComponentDesc* ParseBuoyancyComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    BuoyancyComponentDesc* desc = NewNamedDesc<BuoyancyComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_MaxDepth, &desc->maxDepth, desc->maxDepth);
        elem->GetFloat(kAttr_FloatSpeed, &desc->floatSpeed, desc->floatSpeed);
        elem->GetFloat(kAttr_SinkSpeed, &desc->sinkSpeed, desc->sinkSpeed);
        elem->GetFloat(kAttr_Buoyancy20, &desc->value20, desc->value20);
        elem->GetFloat(kAttr_Buoyancy40, &desc->value40, desc->value40);
        elem->GetFloat(kAttr_Buoyancy3C, &desc->value3C, desc->value3C);
        elem->GetFloat(kAttr_Buoyancy24, &desc->value24, desc->value24);
        elem->GetFloat(kAttr_Buoyancy44, &desc->value44, desc->value44);
        elem->GetFloat(kAttr_Buoyancy28, &desc->value28, desc->value28);
        elem->GetFloat(kAttr_MaxRotationX, &desc->maxRotationX, desc->maxRotationX);
        elem->GetFloat(kAttr_MaxRotationY, &desc->maxRotationY, desc->maxRotationY);
        elem->GetFloat(kAttr_MaxRotationZ, &desc->maxRotationZ, desc->maxRotationZ);
        elem->GetFloat(kAttr_TiltSpeed, &desc->tiltSpeed, desc->tiltSpeed);
        elem->GetFloat(kAttr_Buoyancy4C, &desc->value4C, desc->value4C);
        elem->GetHash(kAttr_ContactScript, &desc->contactScript, desc->contactScript);
        elem->GetFloat(kAttr_ContactInterval, &desc->contactInterval, desc->contactInterval);
    }
    return desc;
}

// @func 0x002CC470 ParseWallClimbComponentDesc
ComponentDesc* ParseWallClimbComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    WallClimbComponentDesc* desc = NewNamedDesc<WallClimbComponentDesc>(root, elem, scope);
    if (desc) {
        elem->GetFloat(kAttr_GrabAngleH, &desc->grabAngleH, desc->grabAngleH);
        elem->GetFloat(kAttr_MaxAngleV, &desc->maxAngleV, desc->maxAngleV);
        elem->GetFloat(kAttr_MaxAngleH, &desc->maxAngleH, desc->maxAngleH);
        desc->grabAngleH *= kDegToRad;
        desc->maxAngleV *= kDegToRad;
        desc->maxAngleH *= kDegToRad;
        elem->GetFloat(kAttr_GrabDistance, &desc->grabDistance, desc->grabDistance);
        elem->GetFloat(kAttr_MaxDistance, &desc->maxDistance, desc->maxDistance);
        elem->GetFloat(kAttr_StickThreshold, &desc->stickThreshold, desc->stickThreshold);
        elem->GetFloat(kAttr_WallClimb50, &desc->value50, desc->value50);
        elem->GetFloat(kAttr_WallClimb54, &desc->value54, desc->value54);
        elem->GetFloat(kAttr_WallClimb58, &desc->value58, desc->value58);
        elem->GetFloat(kAttr_WallClimb60, &desc->value60, desc->value60);
    }
    return desc;
}

// @func 0x002B44AC ParseMonsterClosetComponentDesc
ComponentDesc* ParseMonsterClosetComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    MonsterClosetComponentDesc* desc = NewNamedDesc<MonsterClosetComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    for (XmlElement* child = elem->FirstChildElement(kTag_ClosetSpawn); child;
         child = child->NextSiblingElement(kTag_ClosetSpawn)) {
        MonsterClosetSpawn spawn;
        uint32_t typeName = 0;
        child->GetHash(kAttr_Def, &spawn.def, 0);
        child->GetHash(kAttr_SpawnPoint, &spawn.spawnPoint, spawn.spawnPoint);
        child->GetFloat(kAttr_RespawnCooldown, &spawn.respawnCooldown, spawn.respawnCooldown);
        child->GetBool(kAttr_RespawnEnabled, &spawn.respawnEnabled, spawn.respawnEnabled);
        child->GetHash(kAttr_Type, &typeName, 0);
        // No early exit: the last matching kind wins; unknown kinds become 0.
        uint32_t type = 0;
        for (uint32_t i = 0; i < 2; ++i)
            if (typeName == kClosetSpawnTypes[i])
                type = i;
        spawn.type = type;
        if (spawn.def != 0)
            desc->spawns.PushBack(spawn);
    }
    elem->GetFloat(kAttr_RespawnCooldown, &desc->respawnCooldown, desc->respawnCooldown);
    return desc;
}

// @func 0x002D2F3C ParseDamageSequenceComponentDesc
ComponentDesc* ParseDamageSequenceComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    // This parser skips the missing-name report.
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;
    DamageSequenceComponentDesc* desc = new DamageSequenceComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetHash(kAttr_Script, &desc->script, desc->script);
    elem->GetInt(kAttr_DamageSeq18, &desc->int18, desc->int18);
    elem->GetFloat(kAttr_InitTime, &desc->initTime, desc->initTime);
    elem->GetFloat(kAttr_EnabledTime, &desc->enabledTime, desc->enabledTime);
    elem->GetFloat(kAttr_DisabledTime, &desc->disabledTime, desc->disabledTime);
    elem->GetBool(kAttr_StartEnabled, &desc->startEnabled, desc->startEnabled);
    elem->GetBool(kAttr_DamageSeq29, &desc->flag29, desc->flag29);
    return desc;
}

} // namespace components
} // namespace sly
