// openTIT - "SimpleWheeledVehicle" (0x00244980-0x0024EEDC), see
// SimpleWheeledVehicle.h.
#include "sly/objects/SimpleWheeledVehicle.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeObj.h"
#include "sly/components/Components.h"
#include "sly/components/ControlComponents.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/SlyGame.h"
#include "sly/game/hud/HudManager.h"
#include "sly/objects/VehicleHooks.h"
#include "sly/objects/VehiclePhysics.h"
#include "sly/objects/VehicleTuning.h"

#include <cmath>
#include <cstring>

namespace sly {
namespace objects {

using entity::Entity;

namespace {

constexpr int kState_Destroyed = 1;

constexpr uint32_t kScriptVar_This = 0x0027469Eu;        // "this"
constexpr uint32_t kGunState_On = 0x000009DFu;           // "On"
constexpr uint32_t kGunState_End = 0x00010CBBu;          // "End"

// Script states that follow the motion (entity script and turbo script).
// NOTE: the two airborne states are only known by hash.
constexpr uint32_t kMotion_Idle = 0x002237D4u;                // "Idle"
constexpr uint32_t kMotion_StartDriveForward = 0xCE2E6B3Du;   // "StartDriveForward"
constexpr uint32_t kMotion_StartDriveBackward = 0x594B024Bu;  // "StartDriveBackward"
constexpr uint32_t kMotion_EndDriveForward = 0x9B04BE36u;     // "EndDriveForward"
constexpr uint32_t kMotion_EndDriveBackward = 0x273F0E72u;    // "EndDriveBackward"
constexpr uint32_t kMotion_Air = 0xE15D38C8u;
constexpr uint32_t kMotion_AirEnd = 0x4090D96Fu;
constexpr uint32_t kMotion_AirBackward = 0xAB6ADA7Au;
constexpr uint32_t kMotion_AirBackwardEnd = 0x32AB4EB3u;
constexpr uint32_t kTurbo_StartOff = 0x7D22E5CDu;             // "StartOff"
constexpr uint32_t kTurbo_EndOff = 0x7A29A6B4u;               // "EndOff"

// The "End" state that leaves a motion state, 0 for states without one.
uint32_t MotionEndState(uint32_t state)
{
    switch (state) {
    case kMotion_StartDriveForward: return kMotion_EndDriveForward;
    case kMotion_StartDriveBackward: return kMotion_EndDriveBackward;
    case kMotion_Air: return kMotion_AirEnd;
    case kMotion_AirBackward: return kMotion_AirBackwardEnd;
    default: return 0;
    }
}

// Controllers built by code.
constexpr uint32_t kControlType_VehiclePad = 0x09;
constexpr uint32_t kControlType_External = 0x0A;

// Action bits of the controller's input record (word +0x24). NOTE: two bits
// per action; the actions are named from the help texts.
constexpr uint32_t kAction_Shoot = 0x00001000u;
constexpr uint32_t kAction_Costume = 0x00008000u;
constexpr uint32_t kAction_Horn = 0x00080000u;

// The gun only fires this long after a reset.
constexpr float kGunDelay = 0.25f;

// Collision layers of the probe phantom (players' vehicles use their own).
constexpr uint32_t kLayer_PlayerProbe = 10;
constexpr uint32_t kLayer_Probe = 11;

// Material technique of the wheel/body materials that take constant 7.
constexpr uint32_t kTechnique_Vehicle = 0xA359CDE9u;

// The three HUD prompt slots the help text writes to.
constexpr uint32_t kPrompt_Left = 0x15817C86u;     // NOTE: names unknown
constexpr uint32_t kPrompt_Middle = 0xF8274AF8u;
constexpr uint32_t kPrompt_Right = 0x9A436A2Eu;

uint32_t ControllerActions(entity::EntityComponent* controller)
{
    const auto* input = static_cast<const components::ControlComponent::Input*>(controller->DataBlockB());
    return input ? input->actions[0] : 0;
}

ape::ApeAnimScriptInstance* StartBoundScript(Entity* entity, ape::ApeAnimScriptLibrary* library, uint32_t scriptHash)
{
    ape::ApeAnimScript* script = library->FindScript(scriptHash);
    if (!script)
        return nullptr;
    auto* instance = new ape::ApeAnimScriptInstance(script);
    ape::ApeAnimScriptVar* self = instance->FindVar(kScriptVar_This);
    if (self && self->GetType() == ape::AnimScriptVarType::Object)
        static_cast<ape::ApeAnimScriptObjectVar*>(self)->SetObject(entity::EntityWorldObjectRef(entity));
    return instance;
}

void ShowPrompt(uint32_t prompt, const char* text)
{
    // NOTE: the console passes a colour constant (0x01164CA4) as well.
    hud::ShowTextByName(-1.0f, prompt, text, true);
}

} // namespace

// ===========================================================================
// Definition

// @func 0x00244BEC SimpleWheeledVehicleDef::~SimpleWheeledVehicleDef
// @func 0x00244C54 SimpleWheeledVehicleDef::~SimpleWheeledVehicleDef (deleting)
SimpleWheeledVehicleDef::~SimpleWheeledVehicleDef() = default;

// @func 0x00245DD0 SimpleWheeledVehicleDef::ResolveBodyTemplate
void SimpleWheeledVehicleDef::ResolveBodyTemplate()
{
    if (bodyTemplate)
        return;
    PhysicsBody* source = FindWorldBody(bodyName);
    if (!source)
        return;
    RemoveBodyFromWorld(source);
    bodyTemplate = CloneBody(source, BodyName(source));
}

// @func 0x00247EE4 SimpleWheeledVehicleDef::Spawn
// NOTE: as for the hover vehicle, the console builds the scripted spawn
// description first (0x000DFDF8) and lets the parameters complete it.
Entity* SimpleWheeledVehicleDef::Spawn(entity::EntityPlacement* placement)
{
    ResolveBodyTemplate();
    auto* vehicle = new SimpleWheeledVehicle(this, *placement);
    vehicle->OnSpawn();
    return vehicle;
}

// ===========================================================================
// Help text

void VehicleHelpText::HidePrompts()
{
    hud::ShowByName(kPrompt_Left, false, true, false);
    hud::ShowByName(kPrompt_Middle, false, true, false);
    hud::ShowByName(kPrompt_Right, false, true, false);
}

// @func 0x0024E7E8 VehicleHelpText::OnMessage
bool VehicleHelpText::OnMessage(const Message& message)
{
    if (!owner || !owner->def || message.type != kMsg_Update)
        return true;
    const SimpleWheeledVehicleSettings& s = owner->WheeledDef()->settings;
    const uint32_t needed = VehicleEntity::kVehicle_Enabled | VehicleEntity::kVehicle_ShowHelp;
    const uint8_t state = GetState(0);
    if (state != kHelp_Idle && (owner->vehicleFlags & needed) != needed) {
        SetState(0, kHelp_Idle);
        return false;
    }
    const bool entering = FramesInState(0) == 0;
    const float time = TimeInState(0);

    switch (state) {
    case kHelp_Shoot:
        if (entering) {
            if (shootPagesShown >= g_vehicleSimpleWheeledHelpTextShootMax.Get()) {
                SetState(0, kHelp_Idle);
                return false;
            }
            ++shootPagesShown;
            HidePrompts();
            if (s.disguiseDef)
                ShowPrompt(kPrompt_Right, "$BUTTON_LTRIG2 $STR_COSTUME");
            if (owner->skin)
                ShowPrompt(kPrompt_Middle, "$BUTTON_RTRIG1 $STR_SHOOT");
            if (s.turboEnabled && s.turboMaxSpeed > 0.0f)
                ShowPrompt(kPrompt_Left, "$BUTTON_LTRIG1 $STR_BOOST");
        }
        if (time >= g_vehicleSimpleWheeledHelpTextShootDuration.Get()) {
            SetState(0, kHelp_Idle);
            return false;
        }
        break;
    case kHelp_WaitShoot:
    case kHelp_WaitDrive:
        if (entering)
            HidePrompts();
        if (time > 1.0f) {
            AdvanceState(0);
            return false;
        }
        break;
    case kHelp_Drive:
        if (entering) {
            if (drivePagesShown >= g_vehicleSimpleWheeledHelpTextDriveMax.Get()) {
                SetState(0, kHelp_Shoot);
                return false;
            }
            ++drivePagesShown;
            HidePrompts();
            if (owner->hornScript)
                ShowPrompt(kPrompt_Right, "$BUTTON_CIRCLE $STR_HORN");
            ShowPrompt(kPrompt_Middle, "$BUTTON_FIRE2 $STR_STOPREVERSE");
            ShowPrompt(kPrompt_Left, "$BUTTON_FIRE4 $STR_DRIVE");
        }
        if (time >= g_vehicleSimpleWheeledHelpTextDriveDuration.Get()) {
            AdvanceState(0);
            return false;
        }
        break;
    case kHelp_Idle:
        if (entering)
            HidePrompts();
        break;
    default:
        break;
    }
    return true;
}

// ===========================================================================
// Entity

// @func 0x00247AB8 SimpleWheeledVehicle::SimpleWheeledVehicle
SimpleWheeledVehicle::SimpleWheeledVehicle(SimpleWheeledVehicleDef* wheeledDef, const entity::EntityPlacement& placement)
    : VehicleEntity(wheeledDef)
{
    (void)placement;
    // NOTE: the console copies word +0xC8 of the spawn description into
    // field_1A8; the port's spawn parameters do not carry it yet.
    helpText.owner = nullptr;
    fadeActive = false;
    fadeVisible = true;
    std::memset(fadeColor, 0, sizeof fadeColor);

    CreateBody();
    CreateControllers();
    StartScripts();
    if (def) {
        // The gun shoots for the body's collision group.
        const uint32_t group = (body && BodyInWorld(body)) ? BodyCollisionGroup(body) : 0;
        skin = entity::EntitySkin(this);
        if (skin)
            SkinSetValue4D8(skin, group);
        fireTimer = 0.0f;
    }
    CreateProbePhantom();
    if (def)
        hudElement = hud::Find(WheeledDef()->settings.hudElement);
    ResetState();
    MarkWheelMaterials();
}

// @func 0x00248118 SimpleWheeledVehicle::~SimpleWheeledVehicle
// @func 0x00248210 SimpleWheeledVehicle::~SimpleWheeledVehicle (deleting)
// @func 0x00248258 SimpleWheeledVehicle::~SimpleWheeledVehicle (contact listener thunk, deleting)
// @func 0x002482A4 SimpleWheeledVehicle::~SimpleWheeledVehicle (contact listener thunk)
SimpleWheeledVehicle::~SimpleWheeledVehicle()
{
    entity::EntityShutdown(this);   // virtual slot 3 (StateMachine::Stop)
    if (g_game)
        g_game->RemoveCallback(0, this, this);
    if (body) {
        RemoveBodyFromWorld(body);
        body = nullptr;
    }
    if (shape) {
        PhysicsRemoveReference(shape);
        shape = nullptr;
    }
}

// @func 0x00246D5C SimpleWheeledVehicle::CreateControllers
void SimpleWheeledVehicle::CreateControllers()
{
    if (classBits & 1) {
        components::ControlComponentDesc desc;
        desc.name = ape::HashNameNoCase("TmpControl");
        desc.type = kControlType_VehiclePad;
        desc.value = PlayerPadIndex(PlayerSlot());
        entity::RegisterComponentTemplate(desc.name, &desc);
        padController = AddComponent(&desc, false);
        padController->OnOwnerSpawned(this);
    }
    controller = padController;

    components::CodeComponentDesc ext;
    ext.name = ape::HashNameNoCase("TmpExtControl");
    ext.type = kControlType_External;
    entity::RegisterComponentTemplate(ext.name, &ext);
    extController = AddComponent(&ext, false);
    if (!controller)
        controller = extController;
}

// @func 0x002464F0 SimpleWheeledVehicle::StartScripts
void SimpleWheeledVehicle::StartScripts()
{
    if (!def)
        return;
    const SimpleWheeledVehicleSettings& s = WheeledDef()->settings;
    if (animScript) {
        ape::ApeAnimScriptState* state = animScript->GetState();
        scriptState = state ? state->GetNameHash() : 0;
    }
    ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0);
    if (!library)
        return;

    if (!gunScript) {
        gunScript = StartBoundScript(this, library, s.gunScript);
        if (gunScript) {
            ape::ApeAnimScript* script = gunScript->GetScript();
            gunOnState = script ? script->FindState(kGunState_On) : nullptr;
            gunEndState = script ? script->FindState(kGunState_End) : nullptr;
            gunScript->SetRemoveWhenFinished(false);
            ape::ApeAnimScriptInstance::Register(gunScript);
        }
    }
    if (!turboScript) {
        turboScript = StartBoundScript(this, library, s.turboScript);
        if (turboScript) {
            turboScript->SetRemoveWhenFinished(false);
            ape::ApeAnimScriptInstance::Register(turboScript);
            turboScript->Update(1.0f / 60.0f);
        }
    }
    if (!hornScript) {
        hornScript = StartBoundScript(this, library, s.hornScript);
        if (hornScript) {
            hornScript->SetRemoveWhenFinished(false);
            ape::ApeAnimScriptInstance::Register(hornScript);
            hornScript->Update(1.0f / 60.0f);
        }
    }
}

// @func 0x002462D8 SimpleWheeledVehicle::CreateProbePhantom
// A sphere as wide as the body's box, in the body's collision group, on the
// probe layer (the player's own layer for a player's vehicle).
void SimpleWheeledVehicle::CreateProbePhantom()
{
    if (phantom || !def)
        return;
    const ape::Vec3& size = WheeledDef()->settings.colSize;
    phantomRadius = std::sqrt(size.x * size.x + size.y * size.y + size.z * size.z) * 0.5f;
    const uint32_t groupBits = body ? (BodyCollisionGroup(body) << 16) : 0;
    const uint32_t layer = (classBits & 1) ? kLayer_PlayerProbe : kLayer_Probe;
    phantom = CreateSpherePhantom(phantomRadius, groupBits | layer);
}

// @func 0x00245F70 SimpleWheeledVehicle::MarkWheelMaterials
void SimpleWheeledVehicle::MarkWheelMaterials()
{
    if (object)
        ObjectSetTechniqueConstant(object, kTechnique_Vehicle, 7, 1);
}

// TODO(0x00247068 SimpleWheeledVehicle::CreateBody): builds the body from
// the definition's template with a compound shape (a sphere of half the
// smaller of the box's width/depth, two boxes, a list shape); Havok shape
// construction with bad instruction data in the disassembly. Only the mass
// is taken here.
void SimpleWheeledVehicle::CreateBody()
{
    if (!def)
        return;
    mass = WheeledDef()->settings.mass;
}

// @func 0x00246080 SimpleWheeledVehicle::ResetState
void SimpleWheeledVehicle::ResetState()
{
    field_1AC = 0.0f;
    field_1B0 = 0.0f;
    chassis = transform;
    spawnDelay = 0.0f;
    std::fill(std::begin(field_1B4), std::end(field_1B4), 0.0f);
    std::fill(std::begin(field_1C4), std::end(field_1C4), 0.0f);
    field_1E0 = {0.0f, 1.0f, 0.0f, 0.0f};
    field_1F0 = 0.0f;
    field_1F8[0] = field_1F8[1] = 0.0f;
    field_200 = 90.0f;
    velocity = {0.0f, 0.0f, 0.0f, 0.0f};
    field_4A0 = {0.0f, 0.0f, 0.0f, 0.0f};
    impulse = {0.0f, 0.0f, 0.0f, 0.0f};
    driveMode = kDrive_Idle;
    wallNormal = {0.0f, 0.0f, 0.0f, 0.0f};
    field_4E0 = {0.0f, 0.0f, 0.0f, 0.0f};
    field_500 = kVehicleIdentity;
    grounded[0] = grounded[1] = false;
    turbo = false;
    target = nullptr;
    field_4F8 = 0.0f;
    helpText.owner = this;
    helpText.SetState(0, VehicleHelpText::kHelp_WaitDrive);
    for (Wheel& wheel : wheels)
        wheel.spin = 0.0f;
    fireTimer = 0.0f;
    fadeActive = false;
    fadeVisible = true;
    std::memset(fadeColor, 0, sizeof fadeColor);
}

// @func 0x002482D4 SimpleWheeledVehicle::Release
bool SimpleWheeledVehicle::Release(bool kill)
{
    if (kill) {
        PlayerResetPower(PlayerSlot());
        entity::EntityShutdown(this);   // virtual slot 3 (StateMachine::Stop)
        if (phantom) {
            ReleasePhantom(phantom);
            phantom = nullptr;
        }
        skin = nullptr;
        if (gunScript) {
            gunScript->SetRemoveWhenFinished(true);
            gunScript = nullptr;
            gunOnState = nullptr;
            gunEndState = nullptr;
        }
        if (hornScript) {
            hornScript->SetRemoveWhenFinished(true);
            hornScript = nullptr;
        }
        if (turboScript) {
            turboScript->SetRemoveWhenFinished(true);
            turboScript = nullptr;
        }
    }
    return ScriptedEntity::Release(kill);
}

uint32_t SimpleWheeledVehicle::MotionState() const
{
    static constexpr uint32_t kGrounded[3] = {kMotion_Idle, kMotion_StartDriveForward, kMotion_StartDriveBackward};
    static constexpr uint32_t kAirborne[3] = {kMotion_Air, kMotion_Air, kMotion_AirBackward};
    const bool onGround = grounded[0] || grounded[1];
    return (onGround ? kGrounded : kAirborne)[driveMode];
}

// @func 0x00248958 SimpleWheeledVehicle::UpdateTurboScript
void SimpleWheeledVehicle::UpdateTurboScript()
{
    if (!turboScript)
        return;
    const uint32_t wanted = turbo ? MotionState() : kTurbo_StartOff;
    if (wanted != turboScriptState) {
        const uint32_t end = turboScriptState == kTurbo_StartOff ? kTurbo_EndOff : MotionEndState(turboScriptState);
        if (end) {
            turboScriptState = end;
            turboScript->SetStateAndUpdate(end);
        }
    }
    if (turboScriptState != wanted) {
        turboScriptState = wanted;
        turboScript->SetStateAndUpdate(wanted);
    }
}

// @func 0x00248BB0 SimpleWheeledVehicle::UpdateScriptState
void SimpleWheeledVehicle::UpdateScriptState()
{
    const uint32_t wanted = MotionState();
    if (wanted != scriptState) {
        if (const uint32_t end = MotionEndState(scriptState)) {
            scriptState = end;
            ScriptedEntity::SetTimeScale(end, true);
        }
    }
    if (scriptState != wanted) {
        scriptState = wanted;
        ScriptedEntity::SetTimeScale(wanted, true);
    }
}

// @func 0x0024A2E0 SimpleWheeledVehicle::UpdateGun
void SimpleWheeledVehicle::UpdateGun(float dt)
{
    target = nullptr;
    if (!(vehicleFlags & kVehicle_Enabled))
        return;
    fireTimer += dt;
    const bool firing = (ControllerActions(controller) & kAction_Shoot) && fireTimer >= kGunDelay;
    if (skin) {
        if (entity::EntityComponent* list = entity::EntityComponent01(this)) {
            for (Entity* e = Component01Next(list, nullptr); e; e = Component01Next(list, e)) {
                if (e != this) {
                    target = e;
                    break;
                }
            }
        }
        SkinSetFiring(skin, firing);
        SkinSetTarget(skin, target);
    }
    if (gunScript) {
        ape::ApeAnimScriptState* state = firing ? gunOnState : gunEndState;
        if (state != gunScript->GetState())
            gunScript->SetState(state);
    }
}

// @func 0x0024B064 SimpleWheeledVehicle::SyncFromBody
void SimpleWheeledVehicle::SyncFromBody()
{
    if (!body)
        return;
    const ape::Matrix34 m = BodyTransform(body);
    transform.xAxis = {m.xAxis.x, m.xAxis.y, m.xAxis.z, transform.xAxis.pad};
    transform.yAxis = {m.yAxis.x, m.yAxis.y, m.yAxis.z, transform.yAxis.pad};
    transform.zAxis = {m.zAxis.x, m.zAxis.y, m.zAxis.z, transform.zAxis.pad};
    transform.translation = {m.translation.x, m.translation.y, m.translation.z, transform.translation.pad};
}

// TODO(0x0024B1E8 SimpleWheeledVehicle::ChangeDefinition): rebuilds the
// entity from another definition (components, skin, scripts, body, HUD
// element, team bits); needs the entity module's re-initialisation
// (0x000CDD28, 0x000D2F04, 0x000E0000).
void SimpleWheeledVehicle::ChangeDefinition(SimpleWheeledVehicleDef* newDef)
{
    (void)newDef;
}

// @func 0x0024BA28 SimpleWheeledVehicle::Update
bool SimpleWheeledVehicle::Update(float dt)
{
    if (!VehicleEntity::Update(dt))
        return false;
    if (fadeActive)
        entity::RegisterEntity(entity::kList_B, this);   // the instance render list (0x000D64F4)
    if (controller)
        controller->Update(dt);

    if (spawnDelay > 0.0f) {
        spawnDelay -= dt;
    } else if (entity::EntityGetState(this, 0) != kState_Destroyed && (vehicleFlags & kVehicle_Enabled)) {
        // The costume button swaps to the definition named by DisguiseDef.
        const SimpleWheeledVehicleSettings& s = WheeledDef()->settings;
        if ((ControllerActions(controller) & kAction_Costume) && s.disguiseDef) {
            entity::EntityDef* other = entity::FindEntityDef(s.disguiseDef);
            auto* otherDef = dynamic_cast<SimpleWheeledVehicleDef*>(other);
            if (otherDef) {
                otherDef->ResolveBodyTemplate();
                ChangeDefinition(otherDef);
                helpText.SetState(0, VehicleHelpText::kHelp_WaitDrive);
            }
        }
        UpdateDriving(dt);
        UpdateGun(dt);
        UpdateWheels(dt);
        if ((vehicleFlags & kVehicle_Enabled) && gunScript && (ControllerActions(controller) & kAction_Horn)) {
            ape::ApeAnimScriptState* state = gunScript->GetState();
            if (!state || state->GetNameHash() != kGunState_On)
                gunScript->SetStateAndUpdate(kGunState_On);
        }
        ProbeAhead(dt);
    }
    UpdateScriptState();
    UpdateTurboScript();
    UpdateSounds(dt);
    helpText.Update(dt);
    if (g_game)
        g_game->AddCallback(0, reinterpret_cast<void*>(&SimpleWheeledVehicle::QueuedUpdateTransform), this, this);
    return true;
}

// TODO(0x0024A4D0 SimpleWheeledVehicle::UpdateDriving): throttle, steering,
// turbo and power use from the controller input (2.9 KB of vector code).
void SimpleWheeledVehicle::UpdateDriving(float dt) { (void)dt; }

// TODO(0x00249C18 SimpleWheeledVehicle::UpdateWheels): wheel ground probes,
// suspension and the grounded flags (bad instruction data).
void SimpleWheeledVehicle::UpdateWheels(float dt) { (void)dt; }

// TODO(0x00248E00 SimpleWheeledVehicle::ProbeAhead): sweeps the probe
// phantom ahead of the vehicle when it is faster than probeMinSpeed
// (Ghidra drops the result handling as unreachable).
void SimpleWheeledVehicle::ProbeAhead(float dt) { (void)dt; }

// TODO(0x00248470 SimpleWheeledVehicle::UpdateSounds): engine sounds and
// collision rumble (bad instruction data).
void SimpleWheeledVehicle::UpdateSounds(float dt) { (void)dt; }

// TODO(0x0024C860 SimpleWheeledVehicle::PlaceWheels): wheel bone
// transforms (bad instruction data).
void SimpleWheeledVehicle::PlaceWheels() {}

// @func 0x0024C784 SimpleWheeledVehicle::QueuedUpdateTransform
void SimpleWheeledVehicle::QueuedUpdateTransform(void* unused, void* context)
{
    (void)unused;
    if (auto* vehicle = static_cast<SimpleWheeledVehicle*>(context))
        vehicle->UpdateTransform();
}

// @func 0x0024D6B0 SimpleWheeledVehicle::UpdateTransform
void SimpleWheeledVehicle::UpdateTransform(float dt)
{
    Entity::UpdateTransform(dt);
    PlaceWheels();
}

// @func 0x0024D744 SimpleWheeledVehicle::UpdateFade
void SimpleWheeledVehicle::UpdateFade()
{
    Entity::UpdateFade();
    if (!fadeActive)
        return;
    const bool visible = (flags & entity::kEnt_Visible) != 0;
    if (visible)
        entity::EntityApplyFade(this, fadeColor, true);
    fadeVisible = visible;
    SetVisible(false, true, true, false);
}

// @func 0x0024D804 SimpleWheeledVehicle::GetVelocity
ape::Vec3 SimpleWheeledVehicle::GetVelocity()
{
    if (body && BodyInWorld(body))
        return BodyLinearVelocity(body);
    return velocity;
}

// @func 0x0024D6F8 SimpleWheeledVehicle::GetUpVector
ape::Vec3 SimpleWheeledVehicle::GetUpVector()
{
    if (!def)
        return {0.0f, 0.0f, 0.0f, 0.0f};
    return WheeledDef()->settings.colSize;
}

// @func 0x0024D9C0 SimpleWheeledVehicle::GetMass
float SimpleWheeledVehicle::GetMass()
{
    return (body && BodyInWorld(body)) ? BodyMass(body) : 0.0f;
}

// @func 0x0024DC5C SimpleWheeledVehicle::ApplyPointImpulse
// The impulses are collected and applied by the driving model; the point
// is not used.
void SimpleWheeledVehicle::ApplyPointImpulse(const ape::Vec3& push, const ape::Vec3& point)
{
    (void)point;
    impulse.x += push.x;
    impulse.y += push.y;
    impulse.z += push.z;
}

// @func 0x0024DC98 SimpleWheeledVehicle::SetEnabled
void SimpleWheeledVehicle::SetEnabled(bool on)
{
    const bool wasOn = (vehicleFlags & kVehicle_Enabled) != 0;
    if (on != wasOn) {
        field_1AC = 0.0f;
        field_1B0 = 0.0f;
        const ape::Vec3 zero{0.0f, 0.0f, 0.0f, 0.0f};
        SetVelocity(zero);
        SetAngularVelocity(zero);
        field_1B4[0] = 0.0f;
        velocity = zero;
        field_4A0 = zero;
        impulse = zero;
        turbo = false;
        driveMode = kDrive_Idle;
        UpdateScriptState();
    }
    VehicleEntity::SetEnabled(on);
}

// @func 0x0024DDD0 SimpleWheeledVehicle::OnContact
// @func 0x0024DEC4 SimpleWheeledVehicle::OnContact (contact listener thunk)
// Steep contacts (normals within 30 degrees of horizontal) are summed up as
// walls for the driving model.
void SimpleWheeledVehicle::OnContact(const void* event)
{
    if (!body)
        return;
    ape::Vec3 normal;
    if (!ContactNormal(event, body, normal))
        return;
    static const float kSteep = std::sin(0.5235988f);
    if (std::fabs(normal.y) < kSteep) {
        wallNormal.x += normal.x;
        wallNormal.y += normal.y;
        wallNormal.z += normal.z;
    }
}

} // namespace objects
} // namespace sly
