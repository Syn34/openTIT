// openTIT - Component descriptions parsed from entity XML (second part).
#include "sly/components/ComponentDescs2.h"

#include "sly/components/ComponentDescs.h"
#include "sly/components/ComponentHooks.h"
#include "sly/components/ComponentTuning.h"
#include "sly/components/InventoryDescs.h"
#include "sly/entity/Abilities.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMem.h"

#include <algorithm>
#include <cstring>

namespace sly {
namespace components {

namespace {

using ape::xml::XmlElement;

constexpr float kDegToRad = 0.017453292f;
const float kZero3[3] = {0.0f, 0.0f, 0.0f};   // 0x01164B40

// -- Projector -------------------------------------------------------------
constexpr uint32_t kAttr_Part            = 0x00255BF3u;
constexpr uint32_t kAttr_OffsetPos       = 0x313AB0C1u;
constexpr uint32_t kAttr_Projector30     = 0xC333F453u;   // NOTE: name unknown
constexpr uint32_t kAttr_LightPart       = 0xCF032DE9u;
constexpr uint32_t kAttr_Projector50     = 0x6D1FA88Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_Projector60     = 0x7AB1927Au;   // NOTE: name unknown
constexpr uint32_t kAttr_Projector70     = 0x7A3E769Au;   // NOTE: name unknown
constexpr uint32_t kAttr_PivotOffset     = 0xF4390B35u;
constexpr uint32_t kAttr_Scale           = 0x04B1112Au;
constexpr uint32_t kAttr_Projector8C     = 0x69EC8F92u;   // NOTE: name unknown
constexpr uint32_t kAttr_CheckAlert      = 0xFE1D7554u;
constexpr uint32_t kAttr_TextureName     = 0xE08EDE06u;
constexpr uint32_t kAttr_RenderOption    = 0xE5E6A98Bu;
constexpr uint32_t kAttr_AlertColor      = 0x8F930B87u;
constexpr uint32_t kAttr_NormColor       = 0xB4500507u;
constexpr uint32_t kAttr_CullDist        = 0x465FBA18u;
constexpr uint32_t kAttr_CullRange       = 0x86591AEBu;
constexpr uint32_t kAttr_ProjectorC4     = 0x66C871E2u;   // NOTE: name unknown
constexpr uint32_t kAttr_FlickerEnable   = 0xC0A5255Bu;
constexpr uint32_t kAttr_Flicker10C      = 0xCE0BA767u;   // NOTE: name unknown
constexpr uint32_t kAttr_Flicker110      = 0xCD9F7715u;   // NOTE: name unknown
constexpr uint32_t kAttr_FlickerPeriod   = 0xD2F37599u;
constexpr uint32_t kTag_Perspective      = 0xBBD2B6A2u;   // NOTE: element name unknown
constexpr uint32_t kTag_Orthographic     = 0xF5A8C1B6u;   // NOTE: element name unknown
constexpr uint32_t kAttr_NearZ           = 0x046B8532u;   // "nearz"
constexpr uint32_t kAttr_FarZ            = 0x0020D043u;   // "farz"
constexpr uint32_t kAttr_FOV             = 0x000110ADu;
constexpr uint32_t kAttr_AspectRatio     = 0xC32FC693u;
constexpr uint32_t kAttr_Width           = 0x04EC35A6u;
constexpr uint32_t kAttr_Height          = 0x7ECB8527u;
constexpr uint32_t kAttr_ProjectionBC    = 0x3ED04CF9u;   // NOTE: name unknown
constexpr uint32_t kAttr_ProjectionC0    = 0x9A95F70Au;   // NOTE: name unknown
constexpr uint32_t kTag_SourcePart       = 0x671ED2BAu;   // NOTE: element name unknown
constexpr uint32_t kTag_Source2          = 0x580A9337u;   // NOTE: element name unknown
constexpr uint32_t kTag_Source1          = 0x328B6165u;   // NOTE: element name unknown
constexpr uint32_t kAttr_FadeDist        = 0x4DFD3622u;
constexpr uint32_t kAttr_FadeRange       = 0x726B2021u;   // "fadeRange"
constexpr uint32_t kAttr_SourceE0        = 0x72BD3841u;   // NOTE: name unknown
constexpr uint32_t kAttr_TargetPart      = 0x39F79104u;
constexpr uint32_t kAttr_SourceVec       = 0xD883FE50u;   // NOTE: name unknown
constexpr uint32_t kAttr_Radius          = 0x8FA16092u;

// -- CustomModel -------------------------------------------------------------
constexpr uint32_t kTag_Group            = 0x040EFE5Fu;   // "group"
constexpr uint32_t kTag_Option           = 0x8B5D7C75u;   // "Option"
constexpr uint32_t kAttr_ObjName         = 0xC7E35FE2u;

// -- Weapon --------------------------------------------------------------------
constexpr uint32_t kAttr_WeaponRate      = 0xBA5F6FB1u;   // NOTE: name unknown (sets both intervals)
constexpr uint32_t kAttr_WeaponRateMin   = 0x6807D0E1u;   // NOTE: name unknown ("...Min")
constexpr uint32_t kAttr_WeaponRateMax   = 0x6807CFF3u;   // NOTE: name unknown ("...Max")
constexpr uint32_t kAttr_Weapon404       = 0x51D7D9E6u;   // NOTE: name unknown
constexpr uint32_t kAttr_Weapon40C       = 0x9941979Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_ScatterRange    = 0x0C0DE13Fu;
constexpr uint32_t kAttr_FireOrder       = 0x677FEFF1u;   // NOTE: name unknown
constexpr uint32_t kAttr_Weapon408       = 0x887349FFu;   // NOTE: name unknown
// Fire orders (0x00AD5040): "Sequential", "Random", unknown.
constexpr uint32_t kFireOrders[3] = {0x13234CFFu, 0x8FA5D8C3u, 0xE7EB824Fu};
constexpr uint32_t kAttr_PointOffset     = 0x04849810u;   // NOTE: name unknown
constexpr uint32_t kAttr_PointRot10      = 0x25DA775Au;   // NOTE: name unknown
constexpr uint32_t kAttr_PointRot20      = 0x0A283BAAu;   // NOTE: name unknown
constexpr uint32_t kAttr_Point30         = 0x3C395DA0u;   // NOTE: name unknown
constexpr uint32_t kAttr_Point40         = 0xEE3D09AEu;   // NOTE: name unknown
constexpr uint32_t kAttr_Point50         = 0x69ACF13Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_Point54         = 0xE3D1D420u;   // NOTE: name unknown
constexpr uint32_t kAttr_Point58         = 0x3B25897Au;   // NOTE: name unknown
constexpr uint32_t kAttr_ProjDefName     = 0x161851F3u;
constexpr uint32_t kAttr_SpeedOverride   = 0x65D8FCF3u;
constexpr uint32_t kAttr_Point8C         = 0xD97C7082u;   // NOTE: name unknown
constexpr uint32_t kAttr_Point84Min      = 0xC59F0A85u;   // NOTE: name unknown
constexpr uint32_t kAttr_Point84Max      = 0xC59F0997u;   // NOTE: name unknown
constexpr uint32_t kAttr_LaunchType      = 0x3D8A8D6Du;
constexpr uint32_t kAttr_LaunchAngle     = 0x72B640E0u;
constexpr uint32_t kAttr_MaxRange        = 0x47FC4259u;
constexpr uint32_t kAttr_PredictPos      = 0x62210EDBu;
constexpr uint32_t kAttr_PointDirect1    = 0xA65E6A8Eu;   // NOTE: name unknown
constexpr uint32_t kAttr_LeadTime        = 0x8BB7D429u;
constexpr uint32_t kAttr_Point78         = 0x52A63446u;   // NOTE: name unknown
constexpr uint32_t kAttr_Point7C         = 0x2C394DE0u;   // NOTE: name unknown
constexpr uint32_t kAttr_Point80         = 0xE776A159u;   // NOTE: name unknown
constexpr uint32_t kAttr_PointFlag31     = 0x9A9BAF78u;   // NOTE: name unknown
constexpr uint32_t kAttr_PointFlag30     = 0xED0B55B1u;   // NOTE: name unknown
constexpr uint32_t kAttr_RequireTarget   = 0x145C2B16u;
constexpr uint32_t kAttr_FlattenMatrix   = 0xC2277EA5u;
// Launch types (0x00AD504C).
constexpr uint32_t kLaunchTypes[4] = {0x0027E3CBu, 0x5B0F39FAu, 0x5C0DC16Eu, 0x5D8AAF89u};

// -- Binocucom -----------------------------------------------------------------
constexpr uint32_t kAttr_Binocucom14     = 0x1914C72Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_ShowSFX         = 0xA6043048u;
constexpr uint32_t kAttr_HideSFX         = 0x61314BE3u;
constexpr uint32_t kAttr_AmbientSFX      = 0xF7208EEDu;
constexpr uint32_t kAttr_ZoomInSFX       = 0x0926B92Du;
constexpr uint32_t kAttr_ZoomOutSFX      = 0x2650C80Au;
constexpr uint32_t kAttr_PhotoSFX        = 0xE2B26913u;
constexpr uint32_t kAttr_3DPhotoAnimGeo  = 0xB125DA1Fu;
constexpr uint32_t kAttr_3DPhotoAnim     = 0x00A44FD2u;
constexpr uint32_t kAttr_UIParentGroup   = 0x206FACE1u;
constexpr uint32_t kAttr_NGReticle       = 0x688C8C3Bu;
constexpr uint32_t kAttr_PartialReticle  = 0x07EF5393u;
constexpr uint32_t kAttr_FullReticle     = 0x1A54FEA5u;
constexpr uint32_t kAttr_ReticleTitle    = 0x66F0CEA4u;
constexpr uint32_t kAttr_PhotoGroup      = 0xFF18B38Du;
constexpr uint32_t kAttr_PhotoImage      = 0xFF326669u;
constexpr uint32_t kAttr_PhotoTitle      = 0xFFCBDF66u;
constexpr uint32_t kAttr_PhotoDate       = 0x7393D4C0u;
constexpr uint32_t kAttr_TargetList      = 0x39F5DDAFu;
constexpr uint32_t kAttr_CheckMark       = 0xBDE537B5u;
constexpr uint32_t kAttr_CheckBox        = 0x8A410BC3u;
constexpr uint32_t kAttr_2DPhotoStartAnim = 0x5BA89253u;
constexpr uint32_t kAttr_2DPhotoEndAnim  = 0xA2952E4Cu;
constexpr uint32_t kAttr_PhotoDisplayTime = 0x001D691Du;
constexpr uint32_t kAttr_HelpText        = 0xFFA17BEEu;
constexpr uint32_t kAttr_HelpTextSecondary = 0xE07C9126u;
constexpr uint32_t kAttr_InGameShowScript = 0xDA8FC7DFu;
constexpr uint32_t kAttr_CinematicShowScript = 0x8F5661A5u;
constexpr uint32_t kAttr_TutorialShowScript = 0x0C81A3E6u;
constexpr uint32_t kAttr_InGameHideScript = 0xC0CF4264u;
constexpr uint32_t kAttr_CinematicHideScript = 0x7595DC2Au;
constexpr uint32_t kAttr_TutorialHideScript = 0xF2C11E6Bu;
constexpr uint32_t kAttr_ShowLCharacterScript = 0xF3117C45u;
constexpr uint32_t kAttr_ShowRCharacterScript = 0x3C7CAF7Fu;
constexpr uint32_t kAttr_ShowStaticScript = 0x39A6BC36u;
constexpr uint32_t kAttr_StaticScript    = 0x0FAD7CB9u;
constexpr uint32_t kAttr_AlertScript     = 0x7D757907u;
constexpr uint32_t kAttr_FullReticleScript = 0x23FB73B0u;
constexpr uint32_t kAttr_PartialReticleScript = 0xAF91DE1Eu;
constexpr uint32_t kAttr_CenteredNGReticleScript = 0x0D661292u;
constexpr uint32_t kAttr_Binocucom130    = 0x2B4ED536u;   // NOTE: name unknown
constexpr uint32_t kAttr_Binocucom134    = 0xADD86E25u;   // NOTE: name unknown
constexpr uint32_t kAttr_Binocucom138    = 0xD084310Cu;   // NOTE: name unknown
constexpr uint32_t kTag_Projectile       = 0xA77EB5E9u;
constexpr uint32_t kAttr_Def             = 0x000107E5u;
constexpr uint32_t kAttr_ReloadTime      = 0xD2AE4BE6u;
constexpr uint32_t kAttr_Ability         = 0xE33D410Au;
constexpr uint32_t kAttr_IconID          = 0x80670354u;
constexpr uint32_t kTag_Vision           = 0x96EC4FE8u;
constexpr uint32_t kAttr_Type            = 0x0027873Au;
constexpr uint32_t kAttr_ColorTint       = 0x1C3928BEu;
constexpr uint32_t kAttr_VisionOffset    = 0x3DA51A7Bu;
constexpr uint32_t kAttr_ScanLineFreq    = 0x1F7A8DE9u;
constexpr uint32_t kAttr_Mono            = 0x002432E3u;
constexpr uint32_t kAttr_Bloom           = 0x03C5CDE3u;
constexpr uint32_t kTag_VisionMaterial   = 0xA7AB3D0Fu;
constexpr uint32_t kAttr_Name            = 0x0024728Bu;
constexpr uint32_t kAttr_VisionMaterialReplacement = 0x49AA0D0Bu;   // NOTE: name unknown
// Vision types (0x00F64C14).
constexpr uint32_t kVisionTypes[2] = {0x8999B0E7u /* Normal */, 0xDA5B2377u /* Thermal */};

// -- Shooting ------------------------------------------------------------------
// Anim types (0x00AD5568): "default", "CHARGE", two unknown.
constexpr uint32_t kShootingAnimTypes[4] = {0x86DF6221u, 0x766A16B4u, 0x838099B5u, 0x838099B6u};

template <class Desc>
Desc* NewDesc(XmlElement* root, XmlElement* elem, uint8_t scope, const char** nameOut = nullptr)
{
    const char* name = elem->GetString("name");
    if (nameOut)
        *nameOut = name;
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    Desc* desc = new Desc(ape::HashNameNoCase(name));
    desc->scope = scope;
    return desc;
}

// Index of `value` in `table`, or -1.
int FindIndex(const uint32_t* table, int count, uint32_t value)
{
    for (int i = 0; i < count; ++i)
        if (table[i] == value)
            return i;
    return -1;
}

void ReadDegrees(XmlElement* elem, uint32_t attr, float* out, float defaultDegrees)
{
    elem->GetFloat(attr, out, defaultDegrees);
    *out *= kDegToRad;
}

// Upper bound first, then lower bound (the console's order).
float ClampUpperLower(float value, float lo, float hi)
{
    if (hi - value < 0.0f)
        value = hi;
    if (value - lo < 0.0f)
        value = lo;
    return value;
}

// Owned copy of a string attribute (or of the default).
char* DupStringAttr(XmlElement* elem, uint32_t attr, const char* defaultValue)
{
    const char* value = nullptr;
    elem->GetStringOrNumber(attr, &value, defaultValue);
    return ape::StrDup(value);
}

} // namespace

// ===========================================================================
// Projector
// ===========================================================================

ProjectorComponentDesc::ProjectorComponentDesc(uint32_t nameHash)
    : ComponentDesc(nameHash, kCompDesc_Projector),
      alertColor(g_projectorAlertColor.Get()),
      normColor(g_projectorNormColor.Get()),
      sourceAlertColor(g_projectorAlertColor.Get()),
      sourceNormColor(g_projectorNormColor.Get()),
      sourcePart(0)
{
}

// @func 0x00288C3C ProjectorComponentDesc::ReadPartSource
bool ProjectorComponentDesc::ReadPartSource(XmlElement* elem)
{
    targetPart = 0;
    elem->GetColor(kAttr_AlertColor, &sourceAlertColor, &sourceAlertColor);
    elem->GetColor(kAttr_NormColor, &sourceNormColor, &sourceNormColor);
    elem->GetHash(kAttr_TextureName, &sourceTexture, sourceTexture);
    elem->GetFloat(kAttr_FadeDist, &fadeDist, fadeDist);
    elem->GetFloat(kAttr_FadeRange, &fadeRange, fadeRange);
    elem->GetInt(kAttr_SourceE0, &valueE0, valueE0);
    elem->GetHash(kAttr_TargetPart, &targetPart, targetPart);
    ape::Vec3 vec = {0.0f, 0.0f, 0.0f, 0.0f};
    elem->GetVec3(kAttr_SourceVec, &vec.x, &vec.x);
    sourceVec = vec;
    sourceRadius = 0.0f;
    sourceKind = kProjectorSource_Part;
    elem->GetHash(kAttr_Part, &sourcePart, 0);
    ape::Vec3 offset = {0.0f, 0.0f, 0.0f, 0.0f};
    elem->GetVec3(kAttr_OffsetPos, &offset.x, &offset.x);
    elem->GetFloat(kAttr_Radius, &sourceRadius, sourceRadius);
    sourceOffsetPos = offset;
    return true;
}

// @func 0x00288E7C ParseProjectorComponentDesc
ComponentDesc* ParseProjectorComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ProjectorComponentDesc* desc = NewDesc<ProjectorComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    elem->GetHash(kAttr_Part, &desc->part, desc->part);
    elem->GetVec3(kAttr_OffsetPos, &desc->offsetPos.x, &desc->offsetPos.x);
    elem->GetVec3(kAttr_Projector30, &desc->vec30.x, &desc->vec30.x);
    elem->GetHash(kAttr_LightPart, &desc->lightPart, desc->lightPart);
    elem->GetVec3(kAttr_Projector50, &desc->vec50.x, &desc->vec50.x);
    elem->GetHash(kAttr_Projector60, &desc->hash60, desc->hash60);
    elem->GetVec3(kAttr_Projector70, &desc->vec70.x, &desc->vec70.x);
    elem->GetVec2(kAttr_PivotOffset, desc->pivotOffset, desc->pivotOffset);
    elem->GetFloat(kAttr_Scale, &desc->scale, desc->scale);
    elem->GetBool(kAttr_Projector8C, &desc->flag8C, desc->flag8C);
    elem->GetBool(kAttr_CheckAlert, &desc->checkAlert, desc->checkAlert);
    elem->GetHash(kAttr_TextureName, &desc->textureName, desc->textureName);
    elem->GetHash(kAttr_RenderOption, &desc->renderOption, desc->renderOption);
    elem->GetColor(kAttr_AlertColor, &desc->alertColor, &desc->alertColor);
    elem->GetColor(kAttr_NormColor, &desc->normColor, &desc->normColor);
    elem->GetFloat(kAttr_CullDist, &desc->cullDist, desc->cullDist);
    elem->GetFloat(kAttr_CullRange, &desc->cullRange, desc->cullRange);
    elem->GetFloat(kAttr_ProjectorC4, &desc->valueC4, desc->valueC4);
    elem->GetBool(kAttr_FlickerEnable, &desc->flickerEnable, desc->flickerEnable);
    elem->GetFloat(kAttr_Flicker10C, &desc->flicker10C, desc->flicker10C);
    elem->GetFloat(kAttr_Flicker110, &desc->flicker110, desc->flicker110);
    elem->GetFloat(kAttr_FlickerPeriod, &desc->flickerPeriod, desc->flickerPeriod);

    // The first projection block decides the projection.
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t tag = child->GetNameHash();
        if (tag != kTag_Perspective && tag != kTag_Orthographic)
            continue;
        const bool perspective = tag == kTag_Perspective;
        desc->projection = perspective ? kProjection_Perspective : kProjection_Orthographic;
        child->GetFloat(kAttr_NearZ, &desc->nearZ, desc->nearZ);
        child->GetFloat(kAttr_FarZ, &desc->farZ, desc->farZ);
        child->GetFloat(perspective ? kAttr_FOV : kAttr_Width, &desc->fovOrWidth, desc->fovOrWidth);
        child->GetFloat(perspective ? kAttr_AspectRatio : kAttr_Height, &desc->aspectOrHeight, desc->aspectOrHeight);
        child->GetFloat(kAttr_ProjectionBC, &desc->valueBC, desc->valueBC);
        child->GetFloat(kAttr_ProjectionC0, &desc->valueC0, desc->valueC0);
        break;
    }

    // The first source block decides the light source.
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t tag = child->GetNameHash();
        if (tag == kTag_SourcePart) {
            if (desc->ReadPartSource(child))
                break;
            continue;
        }
        if (tag != kTag_Source1 && tag != kTag_Source2)
            continue;
        desc->sourceKind = tag == kTag_Source1 ? kProjectorSource_1 : kProjectorSource_2;
        desc->sourceScale = 1.0f;
        desc->targetPart = 0;
        child->GetColor(kAttr_AlertColor, &desc->sourceAlertColor, &desc->sourceAlertColor);
        child->GetColor(kAttr_NormColor, &desc->sourceNormColor, &desc->sourceNormColor);
        child->GetHash(kAttr_TextureName, &desc->sourceTexture, desc->sourceTexture);
        child->GetFloat(kAttr_FadeDist, &desc->fadeDist, desc->fadeDist);
        child->GetFloat(kAttr_FadeRange, &desc->fadeRange, desc->fadeRange);
        child->GetInt(kAttr_SourceE0, &desc->valueE0, desc->valueE0);
        child->GetHash(kAttr_TargetPart, &desc->targetPart, desc->targetPart);
        ape::Vec3 vec = {0.0f, 0.0f, 0.0f, 0.0f};
        child->GetVec3(kAttr_SourceVec, &vec.x, &vec.x);
        desc->sourceVec = vec;
        child->GetFloat(kAttr_Scale, &desc->sourceScale, desc->sourceScale);
        break;
    }
    return desc;
}

// ===========================================================================
// CustomModel
// ===========================================================================

CustomModelComponentDesc::CustomModelComponentDesc(uint32_t nameHash)
    : ComponentDesc(nameHash, kCompDesc_CustomModel)
{
    std::memset(groups, 0, sizeof(groups));
}

// @func 0x0028E624 ParseCustomModelComponentDesc
ComponentDesc* ParseCustomModelComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    CustomModelComponentDesc* desc = NewDesc<CustomModelComponentDesc>(root, elem, scope);
    if (!desc)
        return nullptr;
    uint32_t groupCount = 0;
    for (XmlElement* groupElem = elem->FirstChildElement(kTag_Group); groupElem;
         groupElem = groupElem->NextSiblingElement(kTag_Group)) {
        const char* groupName = groupCount < CustomModelComponentDesc::kMaxGroups ? groupElem->GetString("name")
                                                                                    : nullptr;
        if (!groupName) {
            (void)root->GetDocumentPath();   // too many groups or no name (compiled-out error)
            continue;
        }
        CustomModelGroup& group = desc->groups[groupCount++];
        ape::StrNCopy(group.name, groupName, sizeof(group.name) - 1);
        group.nameHash = ape::HashNameNoCase(groupName);
        uint32_t optionCount = 0;
        for (XmlElement* optionElem = groupElem->FirstChildElement(kTag_Option); optionElem;
             optionElem = optionElem->NextSiblingElement(kTag_Option)) {
            const char* optionName = optionCount < CustomModelComponentDesc::kMaxOptions
                                         ? optionElem->GetString("name")
                                         : nullptr;
            if (!optionName) {
                (void)root->GetDocumentPath();
                continue;
            }
            CustomModelOption& option = group.options[optionCount++];
            ape::StrNCopy(option.name, optionName, sizeof(option.name) - 1);
            option.nameHash = ape::HashNameNoCase(optionName);
            optionElem->GetHash(kAttr_ObjName, &option.objName, 0);
        }
        group.optionCount = optionCount;
    }
    desc->groupCount = groupCount;
    return desc;
}

// ===========================================================================
// Weapon
// ===========================================================================

WeaponComponentDesc::WeaponComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Weapon) {}

// @func 0x00293998 ParseWeaponComponentDesc
ComponentDesc* ParseWeaponComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name)
        return nullptr;   // this parser skips the error report
    auto* desc = new WeaponComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    (void)root;

    float rate = 0.0f;
    if (elem->GetFloat(kAttr_WeaponRate, &rate, 0.0f) == ape::xml::kQuery_Ok && rate > 0.0f)
        desc->intervalMin = desc->intervalMax = 1.0f / rate;
    if (elem->GetFloat(kAttr_WeaponRateMin, &rate, 0.0f) == ape::xml::kQuery_Ok && rate > 0.0f)
        desc->intervalMin = 1.0f / rate;
    if (elem->GetFloat(kAttr_WeaponRateMax, &rate, 0.0f) == ape::xml::kQuery_Ok && rate > 0.0f)
        desc->intervalMax = 1.0f / rate;
    elem->GetHash(kAttr_Weapon404, &desc->hash404, desc->hash404);
    elem->GetBool(kAttr_Weapon40C, &desc->flag40C, desc->flag40C);
    elem->GetVec3(kAttr_ScatterRange, &desc->scatterRange.x, kZero3);
    desc->scatterRange.x *= kDegToRad;
    desc->scatterRange.y *= kDegToRad;
    desc->scatterRange.z *= kDegToRad;
    uint32_t order = 0;
    if (elem->GetHash(kAttr_FireOrder, &order, 0) == ape::xml::kQuery_Ok) {
        const int index = FindIndex(kFireOrders, 3, order);
        if (index >= 0)
            desc->fireOrder = uint32_t(index);
    }
    elem->GetInt(kAttr_Weapon408, &desc->value408, desc->value408);

    for (XmlElement* part = elem->FirstChildElement(kAttr_Part); part; part = part->NextSiblingElement(kAttr_Part)) {
        // NOTE: the console does not bound the count; more than six parts
        // would overrun the array, so extra ones are ignored here.
        if (desc->firePointCount >= WeaponComponentDesc::kMaxFirePoints)
            break;
        WeaponFirePoint point = {};
        point.value30 = -1.0f;
        point.vec40 = {1.0f, 0.0f, 0.0f, 0.0f};   // console global 0x01164B50 (first row of the identity matrix)
        point.speedOverride = -1.0f;
        point.value7C = 0.1f;
        point.value80 = -10.0f;
        if (part->GetVec3(kAttr_PointRot10, &point.rot10.x, &point.rot10.x) == ape::xml::kQuery_Ok) {
            point.rot10.x *= kDegToRad;
            point.rot10.y *= kDegToRad;
            point.rot10.z *= kDegToRad;
        }
        if (part->GetVec3(kAttr_PointRot20, &point.rot20.x, &point.rot20.x) == ape::xml::kQuery_Ok) {
            point.rot20.x *= kDegToRad;
            point.rot20.y *= kDegToRad;
            point.rot20.z *= kDegToRad;
        }
        part->GetFloat(kAttr_Point30, &point.value30, point.value30);
        part->GetVec3(kAttr_PointOffset, &point.offset.x, &point.offset.x);
        part->GetHash(kAttr_Point50, &point.hash50, 0);
        part->GetHash(kAttr_ProjDefName, &point.projDefName, 0);
        part->GetFloat(kAttr_SpeedOverride, &point.speedOverride, point.speedOverride);
        part->GetHash(kAttr_Point54, &point.hash54, 0);
        part->GetHash(kAttr_Point58, &point.hash58, 0);
        part->GetInt(kAttr_Point8C, &point.value8C, 0);
        part->GetFloat(kAttr_Point84Min, &point.range84[0], 0.0f);
        part->GetFloat(kAttr_Point84Max, &point.range84[1], 0.0f);
        part->GetVec3(kAttr_Point40, &point.vec40.x, &point.vec40.x);
        uint32_t launch = 0;
        if (part->GetHash(kAttr_LaunchType, &launch, 0) == ape::xml::kQuery_Ok) {
            const int index = FindIndex(kLaunchTypes, 4, launch);
            if (index >= 0)
                point.launchType = index;
        }
        if (point.launchType == kWeaponLaunch_1) {
            if (part->GetFloat(kAttr_LaunchAngle, &point.aimed.launchAngle, 45.0f) == ape::xml::kQuery_Ok)
                point.aimed.launchAngle *= kDegToRad;
            part->GetFloat(kAttr_MaxRange, &point.aimed.maxRange, 100.0f);
            part->GetBool(kAttr_PredictPos, &point.aimed.predictPos, false);
        } else if (point.launchType == kWeaponLaunch_2) {
            part->GetBool(kAttr_PredictPos, &point.direct.predictPos, false);
            part->GetBool(kAttr_PointDirect1, &point.direct.flag1, false);
        }
        part->GetFloat(kAttr_LeadTime, &point.leadTime, point.leadTime);
        part->GetFloat(kAttr_Point78, &point.value78, point.value78);
        part->GetFloat(kAttr_Point7C, &point.value7C, point.value7C);
        part->GetFloat(kAttr_Point80, &point.value80, point.value80);
        part->GetBool(kAttr_PointFlag31, &point.flag31, false);
        part->GetBool(kAttr_PointFlag30, &point.flag30, false);
        part->GetBool(kAttr_RequireTarget, &point.requireTarget, false);
        part->GetBool(kAttr_FlattenMatrix, &point.flattenMatrix, false);
        desc->firePoints[desc->firePointCount++] = point;
    }
    return desc;
}

// ===========================================================================
// Binocucom
// ===========================================================================

BinocucomComponentDesc::BinocucomComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Binocucom)
{
    for (BinocucomVision& vision : visions) {
        vision.colorTint = {0, 0x80, 0xFF, 0xFF};
        vision.visionOffset = 0.05f;
        vision.scanLineFreq = 840.0f;
        vision.mono = true;
        vision.bloom = false;
    }
}

// NOTE: the console destructor lives at 0x0076xxxx (outside this range).
BinocucomComponentDesc::~BinocucomComponentDesc()
{
    char* strings[] = {uiParentGroup, ngReticle, partialReticle, fullReticle, reticleTitle, photoParentGroup,
                       photoImage, photoTitle, photoDate, targetList, checkMark, checkBox, helpText,
                       helpTextSecondary};
    for (char* s : strings)
        ape::MemFree(s);
}

// @func 0x0029FE68 ParseBinocucomComponentDesc
ComponentDesc* ParseBinocucomComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = nullptr;
    BinocucomComponentDesc* desc = NewDesc<BinocucomComponentDesc>(root, elem, scope, &name);
    if (!desc)
        return nullptr;
    elem->GetHash(kAttr_ShowSFX, &desc->showSFX, g_binocucomDefaultShowSFX.Get());
    elem->GetHash(kAttr_HideSFX, &desc->hideSFX, g_binocucomDefaultHideSFX.Get());
    elem->GetHash(kAttr_AmbientSFX, &desc->ambientSFX, g_binocucomDefaultAmbientSFX.Get());
    elem->GetHash(kAttr_ZoomInSFX, &desc->zoomInSFX, g_binocucomDefaultZoomInSFX.Get());
    elem->GetHash(kAttr_ZoomOutSFX, &desc->zoomOutSFX, g_binocucomDefaultZoomOutSFX.Get());
    elem->GetHash(kAttr_PhotoSFX, &desc->photoSFX, g_binocucomDefaultPhotoSFX.Get());
    elem->GetHash(kAttr_3DPhotoAnimGeo, &desc->photo3DAnimGeo, g_binocucomDefault3DPhotoAnimGeo.Get());
    elem->GetHash(kAttr_3DPhotoAnim, &desc->photo3DAnim, g_binocucomDefault3DPhotoAnim.Get());
    desc->uiParentGroup = DupStringAttr(elem, kAttr_UIParentGroup, g_binocucomDefaultUIParentGroup.Get());
    desc->ngReticle = DupStringAttr(elem, kAttr_NGReticle, g_binocucomDefaultNGReticul.Get());
    desc->partialReticle = DupStringAttr(elem, kAttr_PartialReticle, g_binocucomDefaultPartialReticle.Get());
    desc->fullReticle = DupStringAttr(elem, kAttr_FullReticle, g_binocucomDefaultFullReticle.Get());
    desc->reticleTitle = DupStringAttr(elem, kAttr_ReticleTitle, g_binocucomDefaultReticleTitle.Get());
    desc->photoParentGroup = DupStringAttr(elem, kAttr_PhotoGroup, g_binocucomDefaultPhotoParentGroup.Get());
    desc->photoImage = DupStringAttr(elem, kAttr_PhotoImage, g_binocucomDefaultPhotoImage.Get());
    desc->photoTitle = DupStringAttr(elem, kAttr_PhotoTitle, g_binocucomDefaultPhotoTitle.Get());
    desc->photoDate = DupStringAttr(elem, kAttr_PhotoDate, g_binocucomDefaultPhotoDate.Get());
    desc->targetList = DupStringAttr(elem, kAttr_TargetList, g_binocucomDefaultTarget.Get());
    desc->checkMark = DupStringAttr(elem, kAttr_CheckMark, g_binocucomDefaultCheckMark.Get());
    desc->checkBox = DupStringAttr(elem, kAttr_CheckBox, g_binocucomDefaultCheckBox.Get());
    elem->GetHash(kAttr_2DPhotoStartAnim, &desc->photo2DStartAnim, g_binocucomDefault2DPhotoStartAnim.Get());
    elem->GetHash(kAttr_2DPhotoEndAnim, &desc->photo2DEndAnim, g_binocucomDefault2DPhotoEndAnim.Get());
    elem->GetFloat(kAttr_PhotoDisplayTime, &desc->photoDisplayTime, g_binocucomDefaultPhotoDisplayTime.Get());
    desc->helpText = DupStringAttr(elem, kAttr_HelpText, g_binocucomDefaultHelpText.Get());
    desc->helpTextSecondary = DupStringAttr(elem, kAttr_HelpTextSecondary, g_binocucomDefaultHelpTextSecondary.Get());
    elem->GetHash(kAttr_InGameShowScript, &desc->inGameShowScript, g_binocucomDefaultInGameShowScript.Get());
    elem->GetHash(kAttr_CinematicShowScript, &desc->cinematicShowScript, g_binocucomDefaultCinematicShowScript.Get());
    elem->GetHash(kAttr_TutorialShowScript, &desc->tutorialShowScript, g_binocucomDefaultTutorialShowScript.Get());
    elem->GetHash(kAttr_InGameHideScript, &desc->inGameHideScript, g_binocucomDefaultInGameHideScript.Get());
    elem->GetHash(kAttr_CinematicHideScript, &desc->cinematicHideScript, g_binocucomDefaultCinematicHideScript.Get());
    elem->GetHash(kAttr_TutorialHideScript, &desc->tutorialHideScript, g_binocucomDefaultTutorialHideScript.Get());
    elem->GetHash(kAttr_ShowLCharacterScript, &desc->showLCharScript, g_binocucomDefaultShowLCharScript.Get());
    elem->GetHash(kAttr_ShowRCharacterScript, &desc->showRCharScript, g_binocucomDefaultShowRCharScript.Get());
    elem->GetHash(kAttr_ShowStaticScript, &desc->showStaticScript, g_binocucomDefaultShowStaticScript.Get());
    elem->GetHash(kAttr_StaticScript, &desc->staticScript, g_binocucomDefaultStaticScript.Get());
    elem->GetHash(kAttr_AlertScript, &desc->alertScript, g_binocucomDefaultAlertScript.Get());
    elem->GetHash(kAttr_FullReticleScript, &desc->fullReticleScript, g_binocucomDefaultFullReticleScript.Get());
    elem->GetHash(kAttr_PartialReticleScript, &desc->partialReticleScript, g_binocucomDefaultPartialReticleScript.Get());
    elem->GetHash(kAttr_CenteredNGReticleScript, &desc->centeredNGReticleScript,
                  g_binocucomDefaultCenteredNGReticleScript.Get());
    elem->GetHash(kAttr_Binocucom14, &desc->hash14, desc->hash14);

    // Material, animation and text names built from the character prefix.
    const char* prefix = elem->GetString("prefixName") ? elem->GetString("prefixName") : "Sly";
    auto prefixed = [prefix](const char* format) { return ape::HashNameNoCase(ape::TempFormat(format, prefix)); };
    desc->meterMaterial = prefixed("Mat_%sBicomMeter");
    desc->reticleMaterial = prefixed("Mat_%sBicomReticle");
    desc->compassMaterial = prefixed("Mat_%sBicomCompass");
    desc->dbStatic = prefixed("STATIC_%sBicomDBStatic");
    desc->introAnim = prefixed("%sBicom_Intro");
    desc->idleAnim = prefixed("%sBicom_Idle");
    desc->outroAnim = prefixed("%sBicom_Outro");
    desc->introIGCAnim = prefixed("%sBicom_IntroIGC");
    desc->idleIGCAnim = prefixed("%sBicom_IdleIGC");
    desc->outroIGCAnim = prefixed("%sBicom_OutroIGC");
    desc->igcActorLMaterial = prefixed("Mat_%sBicomIGCActorL");
    desc->igcActorRMaterial = prefixed("Mat_%sBicomIGCActorR");
    desc->bicomText = prefixed("STATIC_%sBicomText");

    elem->GetFloat(kAttr_Binocucom130, &desc->value130, -1.0f);
    ReadDegrees(elem, kAttr_Binocucom134, &desc->angle134, 20.0f);
    ReadDegrees(elem, kAttr_Binocucom138, &desc->angle138, -1.0f);

    const ape::TuningColor* tints[2] = {&g_binocucomVisionNormalColorTint, &g_binocucomVisionThermalColorTint};
    const ape::TuningFloat* offsets[2] = {&g_binocucomVisionNormalVisionOffset, &g_binocucomVisionThermalVisionOffset};
    const ape::TuningFloat* freqs[2] = {&g_binocucomVisionNormalScanLineFreq, &g_binocucomVisionThermalScanLineFreq};
    const ape::TuningBool* monos[2] = {&g_binocucomVisionNormalMono, &g_binocucomVisionThermalMono};
    const ape::TuningBool* blooms[2] = {&g_binocucomVisionNormalBloom, &g_binocucomVisionThermalBloom};
    for (int i = 0; i < 2; ++i) {
        desc->visions[i].colorTint = tints[i]->Get();
        desc->visions[i].visionOffset = offsets[i]->Get();
        desc->visions[i].scanLineFreq = freqs[i]->Get();
        desc->visions[i].mono = monos[i]->Get();
        desc->visions[i].bloom = blooms[i]->Get();
    }

    // NOTE: the console does not bound the projectile count (eight slots).
    BinocucomProjectile* projectile = desc->projectiles;
    for (XmlElement* child = elem->FirstChildElement(kTag_Projectile); child;
         child = child->NextSiblingElement(kTag_Projectile)) {
        if (projectile == desc->projectiles + 8)
            break;
        child->GetHash(kAttr_Def, &projectile->def, 0);
        child->GetFloat(kAttr_ReloadTime, &projectile->reloadTime, -1.0f);
        uint32_t abilityName = 0;
        child->GetHash(kAttr_Ability, &abilityName, 0);
        child->GetHash(kAttr_IconID, &projectile->iconId, 0);
        projectile->ability = entity::FindAbility(abilityName);
        if (projectile->ability < 0 && abilityName != 0) {
            // NOTE: the compiled data carries no line numbers; the console
            // prints the element row + 1.
            ape::TempFormat("%s(%ld): Binocucom %s: unrecognized ability flag '%s'.", root->GetDocumentPath(), 0L,
                            name, child->GetString(kAttr_Ability));
        }
        ++projectile;
    }

    for (XmlElement* child = elem->FirstChildElement(kTag_Vision); child;
         child = child->NextSiblingElement(kTag_Vision)) {
        uint32_t type = 0;
        if (child->GetHash(kAttr_Type, &type, 0) != ape::xml::kQuery_Ok)
            continue;
        const int index = FindIndex(kVisionTypes, 2, type);
        if (index < 0)
            continue;
        BinocucomVision& vision = desc->visions[index];
        child->GetColor(kAttr_ColorTint, &vision.colorTint, &vision.colorTint);
        child->GetFloat(kAttr_VisionOffset, &vision.visionOffset, vision.visionOffset);
        child->GetFloat(kAttr_ScanLineFreq, &vision.scanLineFreq, vision.scanLineFreq);
        child->GetBool(kAttr_Mono, &vision.mono, vision.mono);
        child->GetBool(kAttr_Bloom, &vision.bloom, vision.bloom);
    }

    uint32_t materialCount = 0;
    for (XmlElement* child = elem->FirstChildElement(kTag_VisionMaterial); child;
         child = child->NextSiblingElement(kTag_VisionMaterial)) {
        if (materialCount >= 4)
            continue;
        BinocucomVisionMaterial& material = desc->visionMaterials[materialCount++];
        if (child->GetHash(kAttr_Name, &material.name, material.name) == ape::xml::kQuery_Ok)
            child->GetHash(kAttr_VisionMaterialReplacement, &material.replacement, material.replacement);
    }
    return desc;
}

// ===========================================================================
// Shooting
// ===========================================================================

ShootingComponentDesc::ShootingComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Shooting) {}

// @func 0x002C1340 ParseShootingComponentDesc
ComponentDesc* ParseShootingComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    ShootingComponentDesc* d = NewDesc<ShootingComponentDesc>(root, elem, scope);
    if (!d)
        return nullptr;
    // Attack animation sets per physics state.
    elem->GetHash(0xC9D1D9CFu, &d->attackGround, d->attackGround);   // "AttackGround"
    elem->GetHash(0xFB929CF6u, &d->hash18, d->hash18);
    elem->GetHash(0x0BE3E3E6u, &d->hash1C, d->hash1C);
    elem->GetHash(0x898C95E2u, &d->attackAir, d->attackAir);         // "AttackAir"
    elem->GetHash(0xBF170403u, &d->hash24, d->hash24);
    elem->GetHash(0x6DF00D33u, &d->hash28, d->hash28);
    elem->GetHash(0x58DC194Du, &d->attackClimb, d->attackClimb);     // "AttackClimb"
    elem->GetHash(0xB52544B8u, &d->hash30, d->hash30);
    elem->GetHash(0x17163228u, &d->hash34, d->hash34);

    // NOTE: these four defaults are pi/4 used as a value in degrees.
    constexpr float kQuarterPi = 0.7853982f;
    elem->GetFloat(0x64C9BAEAu, &d->angle38, kQuarterPi);
    elem->GetFloat(0xA9FE7C7Bu, &d->angle3C, kQuarterPi);
    elem->GetFloat(0xC039F34Eu, &d->aimSticky, 0.7f);                // "AimSticky"
    elem->GetFloat(0x2EC61337u, &d->pitchRange[0], kQuarterPi);
    elem->GetFloat(0x2EC61249u, &d->pitchRange[1], kQuarterPi);
    d->angle38 *= kDegToRad;
    d->angle3C *= kDegToRad;
    const float pitchA = std::max(-89.9f, std::min(d->pitchRange[0], 89.9f)) * kDegToRad;
    const float pitchB = std::max(-89.9f, std::min(d->pitchRange[1], 89.9f)) * kDegToRad;
    d->pitchRange[0] = std::min(pitchA, pitchB);
    d->pitchRange[1] = std::max(pitchA, pitchB);

    elem->GetFloat(0x566B3A00u, &d->value4C, 0.1f);
    elem->GetFloat(0xBFC64510u, &d->value50, 0.1f);
    elem->GetFloat(0x6684969Du, &d->value54, -1.0f);
    elem->GetFloat(0x6A0E54B6u, &d->value58, -1.0f);
    elem->GetHash(0x6350BDB4u, &d->hash64, d->hash64);
    ReadDegrees(elem, 0x6326647Fu, &d->angle5C, 15.0f);
    ReadDegrees(elem, 0x5EBE4369u, &d->angle60, 60.0f);
    ReadDegrees(elem, 0x184920D6u, &d->angle68, 1.0f);
    ReadDegrees(elem, 0x99DA72E5u, &d->angle6C, 8.0f);
    elem->GetHash(0xC06D2016u, &d->aimBone, 0xB3521F36u);            // default "Spine_2"
    elem->GetFloat(0x9323107Eu, &d->angle74, 60.0f);
    d->angle74 = ClampUpperLower(d->angle74, 0.0f, 90.0f) * kDegToRad;
    elem->GetHash(0xB510DF00u, &d->hash78, d->hash78);
    ReadDegrees(elem, 0x93BC69F6u, &d->angle7C, 5.0f);
    elem->GetFloat(0x7142F22Eu, &d->value80, 0.2f);
    elem->GetBool(0x85F93E26u, &d->flag84, d->flag84);
    elem->GetFloat(0x1499581Cu, &d->value8C, d->value8C);
    d->value8C = ClampUpperLower(d->value8C, 0.2f, 60.0f);
    elem->GetHash(0x811B7C68u, &d->hash90, d->hash90);
    ReadDegrees(elem, 0x456D1135u, &d->angle98, d->angle98);
    ReadDegrees(elem, 0x0743D16Fu, &d->angle94, d->angle94);
    elem->GetBool(0x0AA54FF4u, &d->flag9C, d->flag9C);
    elem->GetBool(0x09AED71Du, &d->flag9D, d->flag9D);
    elem->GetBool(0xEB1C4F07u, &d->flag9E, d->flag9E);
    ReadDegrees(elem, 0xA459F408u, &d->angleA0, 5.0f);
    int32_t count = 10;
    elem->GetInt(0xEAF4CDB3u, &count, 10);
    d->countA4 = std::min<uint32_t>(uint32_t(count), 16u);
    elem->GetFloat(0x11220C44u, &d->valueAC, 0.05f);
    d->valueAC = ClampUpperLower(d->valueAC, 0.001f, 1000.0f);
    elem->GetFloat(0x9D761FB7u, &d->valueB0, 0.05f);
    d->valueB0 = ClampUpperLower(d->valueB0, 0.001f, 1000.0f);
    elem->GetFloat(0x0C617229u, &d->valueB4, 2.0f);
    d->valueB4 = ClampUpperLower(d->valueB4, 0.001f, 1000.0f);
    elem->GetFloat(0x013F745Eu, &d->valueB8, 5.0f);
    d->valueB8 = ClampUpperLower(d->valueB8, 0.1f, 60.0f);
    elem->GetFloat(0x1DF5AEF3u, &d->angleBC, 60.0f);
    d->angleBC = ClampUpperLower(d->angleBC, 1.0f, 89.0f) * kDegToRad;
    elem->GetFloat(0xF2D1211Bu, &d->valueC0, 1.0f);
    d->valueC0 = ClampUpperLower(d->valueC0, 0.0f, 1.0f);
    elem->GetHash(0x651DE034u, &d->hashC4, d->hashC4);
    elem->GetHash(0x320E57BAu, &d->hashC8, d->hashC8);
    elem->GetHash(0xAE60D046u, &d->hashCC, d->hashCC);
    elem->GetFloat(0x6B64F0FCu, &d->valueD0, -1.0f);
    elem->GetFloat(0xE9ED1023u, &d->valueD4, -1.0f);
    elem->GetFloat(0xE9ED1023u, &d->valueD8, d->valueD8);   // NOTE: the console reads the same name twice
    ReadDegrees(elem, 0x32B2FC7Eu, &d->angleDC, d->angleDC);
    ReadDegrees(elem, 0xA29DA90Fu, &d->angleE0, d->angleE0);
    elem->GetFloat(0xFBBD7BFBu, &d->angleE4, d->angleE4);
    d->angleE4 = ClampUpperLower(d->angleE4, 45.0f, 720.0f) * kDegToRad;
    elem->GetHash(0x23136842u, &d->hashE8, d->hashE8);
    elem->GetHash(0x236B23E1u, &d->hashEC, d->hashEC);
    elem->GetFloat(0x05AD2DE4u, &d->valueF0, d->valueF0);
    elem->GetFloat(0x56700203u, &d->valueF4, d->valueF4);
    elem->GetFloat(0x7B3B035Bu, &d->angle12C, d->angle12C);
    d->angle12C = ClampUpperLower(d->angle12C, 0.0f, 89.9f) * kDegToRad;

    // Per-type projectile overrides (<... Type=default|CHARGE|...>).
    for (XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        uint32_t type = 0;
        child->GetHash(kAttr_Type, &type, 0);
        const int index = FindIndex(kShootingAnimTypes, 4, type);
        if (index < 0)
            continue;
        ShootingAnim& anim = d->anims[index];
        child->GetHash(kAttr_ProjDefName, &anim.projDefName, anim.projDefName);
        child->GetHash(0xE3D1D420u, &anim.hash04, anim.hash04);
        child->GetFloat(0xC7A1516Du, &anim.value08, anim.value08);
    }
    elem->GetFloat(0x8833485Du, &d->value130, d->value130);
    elem->GetHash(0x78727291u, &d->hash134, d->hash134);
    return d;
}

// ===========================================================================
// Null parser and dispatcher
// ===========================================================================

// @func 0x0028363C ParseNullComponentDesc
ComponentDesc* ParseNullComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    (void)root;
    (void)elem;
    (void)scope;
    return nullptr;
}

namespace {

using ParseFn = ComponentDesc* (*)(XmlElement*, XmlElement*, uint8_t);

struct ComponentParserEntry {
    uint32_t elementName;
    ParseFn parse;
};

// Element name -> parser (0x00AD4000). NOTE: the console table points the
// ControlPlayer/ControlNet/ControlExt entries at the dispatcher itself (which
// would recurse forever); they build nothing here.
const ComponentParserEntry kComponentParsers[] = {
    {0x07DC7227u, ParsePhysicsComponentDesc},          // "Physics"
    {0x757B8091u, ParseTargetingComponentDesc},        // "Targeting"
    {0x001EBA11u, ParseAnimComponentDesc},             // "Anim"
    {0x76D1F2F4u, ParseCombatComponentDesc},           // "Combat"
    {0x186AEECBu, ParseContextualMarkerComponentDesc}, // "ContextualMarker"
    {0x1F77DE5Eu, nullptr},                            // "ControlPlayer"
    {0x8E8B9E25u, ParseControlAIComponentDesc},        // "ControlAI"
    {0x42E85720u, nullptr},                            // "ControlNet"
    {0x42E837A4u, nullptr},                            // "ControlExt"
    {0x00000828u, ParseAIComponentDescHook},           // "AI"
    {0xF7B7E98Bu, ParseAnimMapComponentDesc},          // "AnimMap"
    {0xF2BABA67u, ParseProcAnimComponentDesc},         // "ProcAnim"
    {0xF2BCB4B8u, ParseProcEyesComponentDesc},         // "ProcEyes"
    {0x00000922u, ParseIKComponentDesc},               // "IK"
    {0x2DA8293Cu, ParseInventoryComponentDesc},        // "Inventory"
    {0x9860B61Cu, ParseWeaponComponentDesc},           // "Weapon"
    {0x70C2069Cu, ParseProjectorComponentDesc},        // "Projector"
    {0x17238238u, ParseCustomModelComponentDesc},      // "CustomModel"
    {0x7043637Eu, ParseFacePlayerComponentDesc},       // "FacePlayer"
    {0x000008D2u, ParseFxComponentDesc},               // "FX"
    {0x04A21AE6u, ParseRadarComponentDesc},            // "Radar"
    {0x5ECD5ED7u, ParseRadarBlipComponentDesc},        // "RadarBlip"
    {0x81A72067u, ParseBinocucomComponentDesc},        // "Binocucom"
    {0x50A58636u, ParseInteractComponentDesc},         // "Interact"
    {0x3536D734u, ParseReceptacleComponentDesc},       // "Receptacle"
    {0x046246A1u, ParseMoverComponentDesc},            // "Mover"
    {0x0690EE75u, ParseOffscreenMarkerComponentDesc},  // "OffscreenMarker"
    {0xB1BEE035u, ParseSimplePhysicsComponentDesc},    // "SimplePhysics"
    {0xE452FC60u, ParseBuoyancyComponentDesc},         // "Buoyancy"
    {0x1F7B9E20u, ParseJumpRopeComponentDesc},         // "JumpRope"
    {0x974090AEu, ParseVortexComponentDesc},           // Vortex (NOTE: element name unknown)
    {0x9D798AAFu, ParseScoringComponentDesc},          // "Scoring"
    {0x0C8B8AB8u, ParseSystemCrackerComponentDesc},    // "SystemCracker"
    {0x871A936Au, ParseMagNetComponentDesc},           // "MagNet"
    {0x42C223BCu, ParseSpeedBoostComponentDesc},       // "SpeedBoost"
    {0x346798F5u, ParseAeSplineComponentDesc},         // "AeSpline"
    {0xCE95B536u, ParseNullComponentDesc},             // NOTE: element name unknown
    {0x7F75E84Au, ParseWheelchairComponentDesc},       // "Wheelchair"
    {0x1A119B63u, ParseShootingComponentDesc},         // "Shooting"
    {0xEB2C6318u, ParseTrackerComponentDesc},          // "tracker"
    {0xFD4BD30Bu, ParseWallClimbComponentDesc},        // "WallClimb"
    {0xBC885EFCu, ParseBallRollComponentDesc},         // "BallRoll"
    {0xBD4675B6u, ParseMonsterClosetComponentDesc},    // "MonsterCloset"
    {0xE2950830u, ParseDamageSequenceComponentDesc},   // "DamageSequence"
    {0x0077D131u, ParseBossGrizzComponentDescHook},    // "BossGrizz"
};

} // namespace

// @func 0x00250190 ParseComponentDesc
ComponentDesc* ParseComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const uint32_t elementName = elem->GetNameHash();
    for (const ComponentParserEntry& entry : kComponentParsers) {
        if (entry.elementName == elementName)
            return entry.parse ? entry.parse(root, elem, scope) : nullptr;
    }
    // Unknown component element (compiled-out error with the hex name).
    (void)root->GetDocumentPath();
    (void)ape::TempHexString(elementName);
    return nullptr;
}

} // namespace components
} // namespace sly
