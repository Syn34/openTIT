// openTIT - Destructors of the component descriptions, the control
// component defaults and the inventory slot defaults.
//
// Each description class has its own destructor (they leave the named
// template table through the ComponentDesc base); most have nothing else
// to release. Complete and deleting variants are tagged together.
#include "sly/components/ComponentDescs.h"
#include "sly/components/ComponentDescs2.h"
#include "sly/components/InventoryDescs.h"
#include "sly/components/Components.h"

#include "ape/memory/ApeMem.h"

#include <cstring>

namespace sly {
namespace components {

// @func 0x00762E64 PhysicsComponentDesc::~PhysicsComponentDesc
// @func 0x00762EA8 PhysicsComponentDesc::~PhysicsComponentDesc (deleting)
PhysicsComponentDesc::~PhysicsComponentDesc() = default;

// @func 0x00749668 TargetingComponentDesc::~TargetingComponentDesc
// @func 0x007496AC TargetingComponentDesc::~TargetingComponentDesc (deleting)
TargetingComponentDesc::~TargetingComponentDesc() = default;

// @func 0x0074EC00 AnimComponentDesc::~AnimComponentDesc
// @func 0x0074EE34 AnimComponentDesc::~AnimComponentDesc (deleting)
AnimComponentDesc::~AnimComponentDesc() = default;

// @func 0x00762F10 CombatComponentDesc::~CombatComponentDesc
// @func 0x00762FC0 CombatComponentDesc::~CombatComponentDesc (deleting)
CombatComponentDesc::~CombatComponentDesc() = default;

// @func 0x007630C8 ContextualMarkerComponentDesc::~ContextualMarkerComponentDesc
// @func 0x0076310C ContextualMarkerComponentDesc::~ContextualMarkerComponentDesc (deleting)
ContextualMarkerComponentDesc::~ContextualMarkerComponentDesc() = default;

// @func 0x0074EC44 ControlAIComponentDesc::~ControlAIComponentDesc
// @func 0x0074ED64 ControlAIComponentDesc::~ControlAIComponentDesc (deleting)
ControlAIComponentDesc::~ControlAIComponentDesc() = default;

// @func 0x00764140 AnimMapComponentDesc::~AnimMapComponentDesc
// @func 0x00764184 AnimMapComponentDesc::~AnimMapComponentDesc (deleting)
AnimMapComponentDesc::~AnimMapComponentDesc() = default;

// @func 0x00763F10 ProcAnimComponentDesc::~ProcAnimComponentDesc
// @func 0x00763F90 ProcAnimComponentDesc::~ProcAnimComponentDesc (deleting)
ProcAnimComponentDesc::~ProcAnimComponentDesc() = default;

// @func 0x00764028 ProcEyesComponentDesc::~ProcEyesComponentDesc
// @func 0x007640A8 ProcEyesComponentDesc::~ProcEyesComponentDesc (deleting)
ProcEyesComponentDesc::~ProcEyesComponentDesc() = default;

// @func 0x007644EC IKComponentDesc::~IKComponentDesc
// @func 0x00764530 IKComponentDesc::~IKComponentDesc (deleting)
IKComponentDesc::~IKComponentDesc() = default;

// @func 0x00764394 FacePlayerComponentDesc::~FacePlayerComponentDesc
// @func 0x007643D8 FacePlayerComponentDesc::~FacePlayerComponentDesc (deleting)
FacePlayerComponentDesc::~FacePlayerComponentDesc() = default;

// @func 0x00764440 FxComponentDesc::~FxComponentDesc
// @func 0x00764484 FxComponentDesc::~FxComponentDesc (deleting)
FxComponentDesc::~FxComponentDesc() = default;

// @func 0x00764644 RadarComponentDesc::~RadarComponentDesc
// @func 0x007646E4 RadarComponentDesc::~RadarComponentDesc (deleting)
RadarComponentDesc::~RadarComponentDesc() = default;

// @func 0x0076479C RadarBlipComponentDesc::~RadarBlipComponentDesc
// @func 0x007647E0 RadarBlipComponentDesc::~RadarBlipComponentDesc (deleting)
RadarBlipComponentDesc::~RadarBlipComponentDesc() = default;

// @func 0x0076528C InteractComponentDesc::~InteractComponentDesc
// @func 0x00765308 InteractComponentDesc::~InteractComponentDesc (deleting)
InteractComponentDesc::~InteractComponentDesc()
{
    ape::MemFree(text);
    text = nullptr;
}

// @func 0x0076539C ReceptacleComponentDesc::~ReceptacleComponentDesc
// @func 0x0076543C ReceptacleComponentDesc::~ReceptacleComponentDesc (deleting)
ReceptacleComponentDesc::~ReceptacleComponentDesc() = default;

// @func 0x007654F4 MoverComponentDesc::~MoverComponentDesc
// @func 0x00765538 MoverComponentDesc::~MoverComponentDesc (deleting)
MoverComponentDesc::~MoverComponentDesc() = default;

// @func 0x007655A0 OffscreenMarkerComponentDesc::~OffscreenMarkerComponentDesc
// @func 0x007655E4 OffscreenMarkerComponentDesc::~OffscreenMarkerComponentDesc (deleting)
OffscreenMarkerComponentDesc::~OffscreenMarkerComponentDesc() = default;

// @func 0x0076564C SimplePhysicsComponentDesc::~SimplePhysicsComponentDesc
// @func 0x00765690 SimplePhysicsComponentDesc::~SimplePhysicsComponentDesc (deleting)
SimplePhysicsComponentDesc::~SimplePhysicsComponentDesc() = default;

// @func 0x00764D6C JumpRopeComponentDesc::~JumpRopeComponentDesc
// @func 0x00764DB0 JumpRopeComponentDesc::~JumpRopeComponentDesc (deleting)
JumpRopeComponentDesc::~JumpRopeComponentDesc() = default;

// @func 0x00738DB0 AeSplineComponentDesc::~AeSplineComponentDesc
// @func 0x0073993C AeSplineComponentDesc::~AeSplineComponentDesc (deleting)
AeSplineComponentDesc::~AeSplineComponentDesc() = default;

// @func 0x007656F8 WheelchairComponentDesc::~WheelchairComponentDesc
// @func 0x0076573C WheelchairComponentDesc::~WheelchairComponentDesc (deleting)
WheelchairComponentDesc::~WheelchairComponentDesc() = default;

// @func 0x007658B4 TrackerComponentDesc::~TrackerComponentDesc
// @func 0x007658F8 TrackerComponentDesc::~TrackerComponentDesc (deleting)
TrackerComponentDesc::~TrackerComponentDesc() = default;

// @func 0x00764E18 VortexComponentDesc::~VortexComponentDesc
// @func 0x00764E5C VortexComponentDesc::~VortexComponentDesc (deleting)
VortexComponentDesc::~VortexComponentDesc() = default;

// @func 0x00764F70 ScoringComponentDesc::~ScoringComponentDesc
// @func 0x00764FB4 ScoringComponentDesc::~ScoringComponentDesc (deleting)
ScoringComponentDesc::~ScoringComponentDesc() = default;

// @func 0x00764EC4 SystemCrackerComponentDesc::~SystemCrackerComponentDesc
// @func 0x00764F08 SystemCrackerComponentDesc::~SystemCrackerComponentDesc (deleting)
SystemCrackerComponentDesc::~SystemCrackerComponentDesc() = default;

// @func 0x0076501C MagNetComponentDesc::~MagNetComponentDesc
// @func 0x00765060 MagNetComponentDesc::~MagNetComponentDesc (deleting)
MagNetComponentDesc::~MagNetComponentDesc() = default;

// @func 0x007651E0 SpeedBoostComponentDesc::~SpeedBoostComponentDesc
// @func 0x00765224 SpeedBoostComponentDesc::~SpeedBoostComponentDesc (deleting)
SpeedBoostComponentDesc::~SpeedBoostComponentDesc() = default;

// @func 0x00765A0C BallRollComponentDesc::~BallRollComponentDesc
// @func 0x00765A50 BallRollComponentDesc::~BallRollComponentDesc (deleting)
BallRollComponentDesc::~BallRollComponentDesc() = default;

// @func 0x007650C8 MonsterClosetComponentDesc::~MonsterClosetComponentDesc
// @func 0x00765148 MonsterClosetComponentDesc::~MonsterClosetComponentDesc (deleting)
MonsterClosetComponentDesc::~MonsterClosetComponentDesc() = default;

// @func 0x00765AB8 DamageSequenceComponentDesc::~DamageSequenceComponentDesc
// @func 0x00765AFC DamageSequenceComponentDesc::~DamageSequenceComponentDesc (deleting)
DamageSequenceComponentDesc::~DamageSequenceComponentDesc() = default;

// @func 0x00764CC0 BuoyancyComponentDesc::~BuoyancyComponentDesc
// @func 0x00764D04 BuoyancyComponentDesc::~BuoyancyComponentDesc (deleting)
BuoyancyComponentDesc::~BuoyancyComponentDesc() = default;

// @func 0x00765960 WallClimbComponentDesc::~WallClimbComponentDesc
// @func 0x007659A4 WallClimbComponentDesc::~WallClimbComponentDesc (deleting)
WallClimbComponentDesc::~WallClimbComponentDesc() = default;

// @func 0x007641EC ProjectorComponentDesc::~ProjectorComponentDesc
// @func 0x00764230 ProjectorComponentDesc::~ProjectorComponentDesc (deleting)
ProjectorComponentDesc::~ProjectorComponentDesc() = default;

// @func 0x007642E8 CustomModelComponentDesc::~CustomModelComponentDesc
// @func 0x0076432C CustomModelComponentDesc::~CustomModelComponentDesc (deleting)
CustomModelComponentDesc::~CustomModelComponentDesc() = default;

// @func 0x00764598 WeaponComponentDesc::~WeaponComponentDesc
// @func 0x007645DC WeaponComponentDesc::~WeaponComponentDesc (deleting)
WeaponComponentDesc::~WeaponComponentDesc() = default;

// @func 0x007657A4 ShootingComponentDesc::~ShootingComponentDesc
// @func 0x007657E8 ShootingComponentDesc::~ShootingComponentDesc (deleting)
ShootingComponentDesc::~ShootingComponentDesc() = default;

// @func 0x00762D34 InventoryComponentDesc::~InventoryComponentDesc
// @func 0x00762DB4 InventoryComponentDesc::~InventoryComponentDesc (deleting)
InventoryComponentDesc::~InventoryComponentDesc() = default;

// ---------------------------------------------------------------------------
// Control component base (vtable 0x00B1C118): defaults shared by the
// control components.

// @func 0x00763174 ControlComponent::~ControlComponent
// @func 0x00763270 ControlComponent::~ControlComponent (deleting)
ControlComponent::~ControlComponent() = default;

// @func 0x00763390 ControlComponent::DataBlockA
void* ControlComponent::DataBlockA() { return &pressed; }

// @func 0x0076339C ControlComponent::DataBlockB
void* ControlComponent::DataBlockB() { return &input; }

// @func 0x007633A8 ControlComponent::ResetState
void ControlComponent::ResetState() { input = Input(); }

// @func 0x007633CC ControlComponent::ButtonForAction
uint32_t ControlComponent::ButtonForAction(uint32_t) const { return 0; }

// ---------------------------------------------------------------------------
// Inventory slot descriptions (vtables 0x00B1BE78 base, 0x00B1BEA0 entity,
// 0x00B1BEC8 loot table).

// @func 0x00762AB8 InventorySlotDesc::~InventorySlotDesc
// @func 0x00762AC8 InventorySlotDesc::~InventorySlotDesc (deleting)
InventorySlotDesc::~InventorySlotDesc() = default;

// @func 0x00762B0C InventorySlotDesc::IsLootTable
bool InventorySlotDesc::IsLootTable() const { return false; }

// @func 0x00762B14 InventorySlotDesc::GetAttachName
uint32_t InventorySlotDesc::GetAttachName(int) const { return attachName; }

// @func 0x00762B1C InventorySlotDesc::GetOffset
ape::Vec3 InventorySlotDesc::GetOffset(int) const { return offset; }

// @func 0x00762B38 InventorySlotDesc::GetRotation
ape::Vec4 InventorySlotDesc::GetRotation(int) const { return rotation; }

// @func 0x00762B5C InventorySlotDesc::GetInPocketNameOverride
const char* InventorySlotDesc::GetInPocketNameOverride() const { return nullptr; }

// @func 0x00762B64 InventoryEntitySlotDesc::~InventoryEntitySlotDesc
// @func 0x00762B74 InventoryEntitySlotDesc::~InventoryEntitySlotDesc (deleting)
InventoryEntitySlotDesc::~InventoryEntitySlotDesc() = default;

// @func 0x00762BB8 InventoryEntitySlotDesc::GetEntityDefName
uint32_t InventoryEntitySlotDesc::GetEntityDefName(int) const { return entityDefName; }

// @func 0x00762BC4 InventoryEntitySlotDesc::GetInPocketNameOverride
const char* InventoryEntitySlotDesc::GetInPocketNameOverride() const { return entityInPocketNameOverride; }

// @func 0x00762BD0 InventoryLootSlotDesc::~InventoryLootSlotDesc
// @func 0x00762C38 InventoryLootSlotDesc::~InventoryLootSlotDesc (deleting)
InventoryLootSlotDesc::~InventoryLootSlotDesc() = default;

// @func 0x00762CB8 InventoryLootSlotDesc::IsLootTable
bool InventoryLootSlotDesc::IsLootTable() const { return true; }

} // namespace components
} // namespace sly
