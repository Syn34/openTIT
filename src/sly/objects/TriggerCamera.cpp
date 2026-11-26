// openTIT - Camera trigger volumes.
#include "sly/objects/TriggerCamera.h"

#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

namespace sly {
namespace objects {

namespace {
constexpr float kDegToRad = 0.017453292f;

uint16_t CameraStateBit(uint32_t nameHash)
{
    switch (nameHash) {
    case 0x7DD0CD87: return kCamState_Ground;   // "Ground"
    case 0x03D66C07: return kCamState_Crawl;    // "Crawl"
    case 0x03D3CE95: return kCamState_Climb;    // "Climb"
    case 0x0000FD2A: return kCamState_Air;      // "Air"
    default:         return 0;
    }
}
} // namespace

void ParseCameraStates(ape::xml::XmlElement* elem, uint16_t& allowed, uint16_t& excluded)
{
    uint16_t listed = 0;
    for (ape::xml::XmlElement* child = elem->FirstChildElement(kAttr_PhysicsState); child;
         child = child->NextSiblingElement(kAttr_PhysicsState)) {
        uint32_t name = 0;
        child->GetHash(kAttr_Name, &name, 0);
        listed |= CameraStateBit(name);
    }
    uint32_t single = 0;
    elem->GetHash(kAttr_PhysicsState, &single, 0);
    uint16_t mask;
    if (single == 0 && listed == 0)
        mask = kCamState_All;
    else
        mask = single ? CameraStateBit(single) : 0;
    allowed = (uint16_t)(mask | listed);
    excluded = (uint16_t)~allowed;
    // Climbing volumes also keep the camera while jumping off the wall.
    if (allowed & kCamState_Climb)
        excluded &= (uint16_t)~kCamState_Air;
}

// ===========================================================================
// Definition
// ===========================================================================

CameraTriggerDefDesc::CameraTriggerDefDesc(const EntityDefDesc& loader)
    : TriggerDefDesc(0x00800000u | kDefClass_Trigger, loader, "TriggerCamera_")
{
    playersOnly = true;
}

// @func 0x00214C28 CameraTriggerDefDesc::Parse
void CameraTriggerDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc::Parse(root, elem, loader, parent);
    field_20 = 1;
    ParseCameraStates(elem, allowedStates, excludedStates);
}

CameraTriggerDef::CameraTriggerDef(const CameraTriggerDefDesc& desc)
    : TriggerDef(desc),
      allowedStates(desc.allowedStates),
      excludedStates(desc.excludedStates),
      field_B4(desc.field_58)
{
    // Camera volumes only ever react to players.
    playersOnly = true;
}

// @func 0x00214E1C CreateCameraTriggerDef
EntityDef* CreateCameraTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                  const EntityDefDesc& loader, const EntityDef* parent)
{
    CameraTriggerDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    return new CameraTriggerDef(desc);
}

CameraTriggerInstanceDesc::CameraTriggerInstanceDesc(const char* instanceName, CameraTriggerDef* ownerDef)
    : TriggerInstanceDesc(instanceName, ownerDef)
{
    if (ownerDef) {
        allowedStates = ownerDef->allowedStates;
        excludedStates = ownerDef->excludedStates;
    }
}

// @func 0x00215128 CameraTriggerDef::Create
Entity* CameraTriggerDef::Spawn(EntityPlacement* placement)
{
    bool typed = (placement->flags & kPlace_Camera) != 0;
    if (TriggerPlacementSkips(placement, typed))
        return nullptr;
    CameraTriggerInstanceDesc desc(InstanceName(placement), this);
    const char* objectName = TriggerPlacementObjectName(placement);
    placement->FillInstanceDesc(desc);
    // Spline cameras default to "Spline_Camera_Pos<suffix>"/"Spline_Camera_Tgt<suffix>".
    if (desc.behavior == kCameraBehavior_Spline) {
        const char* suffix = TriggerNameSuffix(objectName);
        if (desc.splinePos == 0)
            desc.splinePos = ape::HashNameNoCase(ape::TempFormat("Spline_Camera_Pos%s", suffix));
        if (desc.splineTgt == 0)
            desc.splineTgt = ape::HashNameNoCase(ape::TempFormat("Spline_Camera_Tgt%s", suffix));
    }
    CameraTrigger* trigger = new CameraTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

// ===========================================================================
// Placement
// ===========================================================================

CameraTriggerPlacement::CameraTriggerPlacement(const char* placementName)
    : TriggerPlacement(placementName, nullptr, nullptr, true, false, true, false, false, false)
{
    flags |= kPlace_Camera;
    playersOnly = true;
}

// @func 0x00215518 ReadCameraTriggerPlacement
static void ReadCameraTriggerPlacement(ape::xml::XmlElement* elem, CameraTriggerPlacement* p)
{
    ParseTriggerPlacementFlags(elem, *p);
    ParseCameraStates(elem, p->allowedStates, p->excludedStates);
    elem->GetFloat(kAttr_EaseIn, &p->easeIn, p->easeIn);
    elem->GetFloat(kAttr_EaseOut, &p->easeOut, p->easeOut);
    elem->GetHash(kAttr_Behavior, &p->behavior, p->behavior);
    if (p->behavior == kCameraBehavior_Spline) {
        elem->GetHash(kAttr_SplinePos, &p->splinePos, p->splinePos);
        elem->GetHash(kAttr_SplineTgt, &p->splineTgt, p->splineTgt);
        elem->GetHash(kAttr_SplineParent, &p->splineParent, p->splineParent);
    } else if (p->behavior == kCameraBehavior_Track) {
        float pos[3] = { p->offsetPos.x, p->offsetPos.y, p->offsetPos.z };
        elem->GetVec3(kAttr_OffsetPos, pos, pos);
        p->offsetPos = { pos[0], pos[1], pos[2], 0.0f };
        elem->GetVec2(kAttr_OffsetRot, p->offsetRot, p->offsetRot);
        p->offsetRot[0] *= kDegToRad;
        p->offsetRot[1] *= kDegToRad;
        elem->GetFloat(kAttr_Fov, &p->fov, p->fov);
        elem->GetBool(kAttr_InvertDir, &p->invertDir, p->invertDir);
    }
}

// @func 0x002159AC ParseCameraTriggerPlacement
EntityPlacement* ParseCameraTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                             const char* name)
{
    (void)root;
    CameraTriggerPlacement* p = new CameraTriggerPlacement(name);
    ReadCameraTriggerPlacement(elem, p);
    return p;
}

// @func 0x00215ACC CameraTriggerPlacement::FillInstanceDesc
void CameraTriggerPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    TriggerPlacement::FillInstanceDesc(baseDesc);
    CameraTriggerInstanceDesc& desc = static_cast<CameraTriggerInstanceDesc&>(baseDesc);
    desc.field_9C = field_BC;
    desc.allowedStates = allowedStates;
    desc.excludedStates = excludedStates;
    desc.behavior = behavior;
    desc.easeIn = easeIn;
    desc.easeOut = easeOut;
    desc.splinePos = splinePos;
    desc.splineTgt = splineTgt;
    desc.splineParent = splineParent;
    desc.offsetPos = offsetPos;
    desc.offsetRot[0] = offsetRot[0];
    desc.offsetRot[1] = offsetRot[1];
    desc.fov = fov;
    desc.invertDir = invertDir;
}

// ===========================================================================
// Entity
// ===========================================================================

// @func 0x00214FAC CameraTrigger::CameraTrigger
CameraTrigger::CameraTrigger(const CameraTriggerInstanceDesc& desc)
    : TriggerBase(desc, 0x20, true),
      m_allowedStates(desc.allowedStates),
      m_excludedStates(desc.excludedStates),
      m_behavior(desc.behavior),
      m_easeIn(desc.easeIn),
      m_easeOut(desc.easeOut)
{
    if (m_behavior == kCameraBehavior_Spline) {
        // The target spline is only looked up when the position spline exists
        // (a missing one is reported as a warning, stripped).
        m_splinePos = FindSplineDef(desc.splinePos);
        if (m_splinePos)
            m_splineTgt = FindSplineDef(desc.splineTgt);
        m_splineParent = desc.splineParent;
    } else if (m_behavior == kCameraBehavior_Track) {
        m_offsetPos = desc.offsetPos;
        m_offsetRot[0] = desc.offsetRot[0];
        m_offsetRot[1] = desc.offsetRot[1];
        m_fov = desc.fov;
        m_invertDir = desc.invertDir;
    }
}

// @func 0x00215D40 CameraTrigger::~CameraTrigger (deleting)
CameraTrigger::~CameraTrigger() = default;

// @func 0x0021686C CameraTrigger::OnEnter
void CameraTrigger::OnEnter(Entity* entity)
{
    TriggerBase::OnEnter(entity);
    ActivateCamera(entity);
}

// @func 0x0021690C CameraTrigger::OnExit
void CameraTrigger::OnExit(Entity* entity)
{
    // With "NoDeadExit" a dead character keeps the camera of the volume it died in.
    if (g_triggerCameraNoDeadExit.Get() && entity && entity->HasTypeBit(kType_Character) && EntityIsDead(entity))
        return;
    DeactivateCamera(entity);
    TriggerBase::OnExit(entity);
}

// @func 0x002169C0 CameraTrigger::ReleaseCameraMode
void CameraTrigger::ReleaseCameraMode()
{
    if (!m_cameraMode)
        return;
    CameraRemoveMode(m_cameraMode, -1.0f);
    CameraModeDelete(m_cameraMode);
    m_cameraMode = nullptr;
}

// @func 0x00216A64 CameraTrigger::IsDebugDisplayed
bool CameraTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerCamera.Get());
}

} // namespace objects
} // namespace sly
