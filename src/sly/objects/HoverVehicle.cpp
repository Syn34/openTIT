// openTIT - "HoverVehicle" (0x0023E3E0-0x00244900), see HoverVehicle.h.
#include "sly/objects/HoverVehicle.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeObj.h"
#include "ape/xml/ApeXml.h"
#include "sly/camera/CameraModes.h"
#include "sly/components/ControlComponents.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameCamera.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/objects/VehicleHooks.h"
#include "sly/objects/VehiclePhysics.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sly {
namespace objects {

using entity::Entity;

namespace {

constexpr float kDegToRad = 0.017453292f;

// Layer states shared with VehicleEntity (see Vehicle.h).
constexpr int kState_Destroyed = 1;
constexpr int kDestroy_Animating = 1;
constexpr int kDestroy_Exploding = 2;
constexpr int kDestroy_Done = 3;

// Script state the entity's own script switches to when it blows up.
constexpr uint32_t kScriptState_Destroy = 0x879F2BDAu;   // "destroy"
// Script variable bound to the vehicle's world object.
constexpr uint32_t kScriptVar_This = 0x0027469Eu;        // "this"

// Controllers built by code.
constexpr uint32_t kControlType_RadarPad = 0x0B;
constexpr uint32_t kControlType_External = 0x0A;

// Animation driver state while the turbo runs (and back to idle).
constexpr uint32_t kDriver_Turbo = 6;
constexpr uint32_t kDriver_Idle = 0;

// Body response scaling of every contact: the contact does not turn the body
// (inverse inertia scaled to 0, inverse mass kept).
const ape::Vec4 kContactResponseScale{0.0f, 0.0f, 0.0f, 1.0f};

// Collision layers of the other body that destroy a vehicle mid-destruction.
bool IsDestroyingLayer(uint32_t layer) { return layer >= 1 && layer <= 3; }
// Layers that count as static world geometry for "REJECTCOLLISIONS".
bool IsWorldLayer(uint32_t layer) { return layer == 0 || layer > 4; }

// Attribute and element hashes of the hover settings.
namespace attr {
constexpr uint32_t kMass = 0x0023FEF4u;
constexpr uint32_t kAngularDamping = 0x8933D0E0u;
constexpr uint32_t kDeadZone = 0x4CB13E10u;
constexpr uint32_t kDpadScaleY = 0x824119DCu;
constexpr uint32_t kDpadScaleX = 0x2CA6B8AFu;
constexpr uint32_t kStickScaleY = 0x92AC6682u;
constexpr uint32_t kStickScaleX = 0x24EC3F49u;
constexpr uint32_t kField32C = 0x6C3A7CD2u;
constexpr uint32_t kField3F0 = 0x0F05FBBFu;
constexpr uint32_t kRejectCollisions = 0xA7A23860u;
constexpr uint32_t kFlightEnabled = 0x88CBF571u;
constexpr uint32_t kType = 0x0027873Au;
constexpr uint32_t kMinSpeed = 0xDE326D35u;
constexpr uint32_t kMaxSpeed = 0x48110963u;
constexpr uint32_t kMaxAccel = 0x470D7204u;
constexpr uint32_t kMaxDecel = 0x4738A145u;
constexpr uint32_t kField344 = 0x8E0994A0u;
constexpr uint32_t kField348 = 0x332715B6u;
constexpr uint32_t kField34C = 0xBD488CBDu;
constexpr uint32_t kField350 = 0x7694E8F2u;
constexpr uint32_t kMaxRotAccel = 0xF32CD2B5u;
constexpr uint32_t kMaxRotDecel = 0xF35801F6u;
constexpr uint32_t kMaxAngle = 0x471281EFu;
constexpr uint32_t kMinAngle = 0xDD33E5C1u;
constexpr uint32_t kAngleScale = 0x31BC0857u;
constexpr uint32_t kMaxRate = 0x5D292C24u;
constexpr uint32_t kStartCost = 0xBEDAEF09u;
constexpr uint32_t kPowerCostRate = 0xED971A12u;
constexpr uint32_t kField3A0 = 0xA2F8435Au;
constexpr uint32_t kMinTime = 0x6A440BFFu;
constexpr uint32_t kMaxTime = 0x5D2A3211u;
constexpr uint32_t kMinPower = 0x6F625C15u;
constexpr uint32_t kScript = 0x9178C4CBu;
constexpr uint32_t kMinAngleScale = 0x7E7B9C09u;
constexpr uint32_t kMaxAngleScale = 0x11DD6C9Bu;
constexpr uint32_t kMaxSpeedTurbo = 0x23E3F27Bu;
constexpr uint32_t kMaxAccelTurbo = 0xAD24C67Au;
constexpr uint32_t kField3E4 = 0xF7740920u;
constexpr uint32_t kField3E8 = 0x7F1D654Bu;
} // namespace attr
namespace elem {
constexpr uint32_t kMovement = 0x48D1D90Eu;
constexpr uint32_t kAltMovement = 0x22CB5538u;
constexpr uint32_t kPitch = 0x0489CB20u;
constexpr uint32_t kRoll = 0x0026787Du;
constexpr uint32_t kTurboBoost = 0xD6E3A445u;
constexpr uint32_t kAngles = 0x94174CE7u;
} // namespace elem

float Saturate(float v) { return std::clamp(v, 0.0f, 1.0f); }

void ReadTilt(ape::xml::XmlElement* e, HoverVehicleSettings::Tilt& tilt)
{
    e->GetFloat(attr::kMaxAngle, &tilt.maxAngle, tilt.maxAngle);
    e->GetFloat(attr::kAngleScale, &tilt.angleScale, tilt.angleScale);
    e->GetFloat(attr::kMaxRate, &tilt.maxRate, tilt.maxRate);
    e->GetFloat(attr::kMaxDecel, &tilt.maxDecel, tilt.maxDecel);
    e->GetFloat(attr::kMaxAccel, &tilt.maxAccel, tilt.maxAccel);
    tilt.maxAngle *= kDegToRad;
    tilt.maxRate *= kDegToRad;
    tilt.maxDecel *= kDegToRad;
    tilt.maxAccel *= kDegToRad;
}

// Cosine of an angle attribute given in degrees, when present.
void ReadCosine(ape::xml::XmlElement* e, uint32_t name, float* out)
{
    float degrees = 0.0f;
    if (e->GetFloat(name, &degrees, 0.0f) == ape::xml::kQuery_Ok)
        *out = std::cos(degrees * kDegToRad);
}

} // namespace

// ===========================================================================
// Settings

const uint32_t HoverVehicleSettings::kMovementTypes[2] = {0xFB3C8FC3u, 0x1E7E11DAu};

void HoverVehicleSettings::Parse(ape::xml::XmlElement* e)
{
    e->GetFloat(attr::kMass, &mass, mass);
    e->GetFloat(attr::kAngularDamping, &angularDamping, angularDamping);
    e->GetFloat(attr::kDeadZone, &deadZone, deadZone);
    e->GetFloat(attr::kDpadScaleY, &dpadScaleY, dpadScaleY);
    e->GetFloat(attr::kDpadScaleX, &dpadScaleX, dpadScaleX);
    e->GetFloat(attr::kStickScaleY, &stickScaleY, stickScaleY);
    e->GetFloat(attr::kStickScaleX, &stickScaleX, stickScaleX);
    e->GetFloat(attr::kField32C, &field_32C, field_32C);
    const ape::Vec3 offsetDefault = field_3F0;
    e->GetVec3(attr::kField3F0, &field_3F0.x, &offsetDefault.x);
    e->GetBool(attr::kRejectCollisions, &rejectCollisions, rejectCollisions);
    e->GetBool(attr::kFlightEnabled, &flightEnabled, flightEnabled);

    mass = std::max(mass, 0.0f);
    angularDamping = std::max(angularDamping, 0.0f);
    deadZone = Saturate(deadZone);
    dpadScaleY = Saturate(dpadScaleY);
    dpadScaleX = Saturate(dpadScaleX);
    stickScaleY = Saturate(stickScaleY);
    stickScaleX = Saturate(stickScaleX);
    field_32C = std::max(field_32C, 0.0f);

    for (ape::xml::XmlElement* child = e->FirstChildElement(); child; child = child->NextSiblingElement()) {
        switch (child->GetNameHash()) {
        case elem::kMovement: {
            uint32_t type = 0;
            if (child->GetHash(attr::kType, &type, 0) == ape::xml::kQuery_Ok) {
                for (uint32_t i = 0; i < 2; ++i) {
                    if (kMovementTypes[i] == type) {
                        movementType = i;
                        break;
                    }
                }
            }
            child->GetFloat(attr::kMinSpeed, &minSpeed, minSpeed);
            child->GetFloat(attr::kMaxSpeed, &maxSpeed, maxSpeed);
            child->GetFloat(attr::kMaxAccel, &maxAccel, maxAccel);
            child->GetFloat(attr::kMaxDecel, &maxDecel, maxDecel);
            maxDecel = std::max(maxDecel, 0.1f);
            child->GetFloat(attr::kField344, &field_344, field_344);
            child->GetFloat(attr::kField348, &field_348, field_348);
            child->GetFloat(attr::kField34C, &field_34C, field_34C);
            child->GetFloat(attr::kField350, &field_350, field_350);
            child->GetFloat(attr::kMaxRotAccel, &maxRotAccel, maxRotAccel);
            child->GetFloat(attr::kMaxRotDecel, &maxRotDecel, maxRotDecel);
            minSpeed = std::max(minSpeed, 0.0f);
            field_344 = std::max(field_344, 0.0f);
            maxSpeed = std::max(maxSpeed, minSpeed);
            field_348 = std::max(field_348, field_344);
            field_350 = Saturate(field_350);
            field_34C = std::clamp(field_34C, 0.0f, 180.0f);
            maxRotAccel = std::max(maxRotAccel, 0.0f);
            maxRotDecel = std::max(maxRotDecel, 0.0f);
            break;
        }
        case elem::kAltMovement:
            child->GetVec2(attr::kMaxAccel, altMaxAccel, altMaxAccel);
            child->GetFloat(attr::kMaxDecel, &altMaxDecel, altMaxDecel);
            child->GetFloat(attr::kMinSpeed, &altMinSpeed, altMinSpeed);
            child->GetFloat(attr::kMaxSpeed, &altMaxSpeed, altMaxSpeed);
            child->GetFloat(attr::kMaxSpeedTurbo, &altMaxSpeedTurbo, altMaxSpeedTurbo);
            child->GetVec2(attr::kMaxAccelTurbo, altMaxAccelTurbo, altMaxAccelTurbo);
            child->GetFloat(attr::kField3E4, &altField3E4, altField3E4);
            child->GetFloat(attr::kField3E8, &altField3E8, altField3E8);
            altField3E8 = std::clamp(altField3E8, 1.0f, 20.0f);
            break;
        case elem::kPitch:
            ReadTilt(child, pitch);
            break;
        case elem::kRoll:
            ReadTilt(child, roll);
            break;
        case elem::kTurboBoost:
            child->GetFloat(attr::kMinSpeed, &turboMinSpeed, turboMinSpeed);
            child->GetFloat(attr::kMaxSpeed, &turboMaxSpeed, turboMaxSpeed);
            child->GetFloat(attr::kMaxDecel, &turboMaxDecel, turboMaxDecel);
            child->GetFloat(attr::kMaxAccel, &turboMaxAccel, turboMaxAccel);
            child->GetFloat(attr::kField350, &turboField394, turboField394);
            child->GetFloat(attr::kStartCost, &turboStartCost, turboStartCost);
            child->GetFloat(attr::kPowerCostRate, &turboCostRate, turboCostRate);
            child->GetFloat(attr::kField3A0, &turboField3A0, turboField3A0);
            child->GetFloat(attr::kMinTime, &turboMinTime, turboMinTime);
            child->GetFloat(attr::kMaxTime, &turboMaxTime, turboMaxTime);
            child->GetFloat(attr::kMinPower, &turboMinPower, turboMinPower);
            child->GetHash(attr::kScript, &turboScript, turboScript);
            turboMinSpeed = std::max(turboMinSpeed, 0.0f);
            turboField394 = Saturate(turboField394);
            turboMaxSpeed = std::max(turboMaxSpeed, turboMinSpeed);
            break;
        case elem::kAngles:
            ReadCosine(child, attr::kMinAngle, &cosMinAngle);
            child->GetFloat(attr::kMinAngleScale, &minAngleScale, minAngleScale);
            ReadCosine(child, attr::kMaxAngle, &cosMaxAngle);
            child->GetFloat(attr::kMaxAngleScale, &maxAngleScale, maxAngleScale);
            break;
        default:
            break;
        }
    }
}

// ===========================================================================
// Definition

// @func 0x0023E3E0 HoverVehicleDef::~HoverVehicleDef
// @func 0x0023E470 HoverVehicleDef::~HoverVehicleDef (deleting)
HoverVehicleDef::~HoverVehicleDef()
{
    ReleaseBody(bodyTemplate);
    bodyTemplate = nullptr;
}

// @func 0x00240AC0 HoverVehicleDef::ResolveBodyTemplate
void HoverVehicleDef::ResolveBodyTemplate()
{
    if (bodyTemplate)
        return;
    PhysicsBody* source = FindWorldBody(bodyName);
    if (!source)
        return;
    // The level's own copy leaves the world; the definition keeps a clone
    // with the same name to build each vehicle's body from.
    RemoveBodyFromWorld(source);
    bodyTemplate = CloneBody(source, BodyName(source));
}

// @func 0x00240C64 HoverVehicleDef::Spawn
// NOTE: the console builds the scripted entity spawn description first
// (0x000DFDF8) and lets the spawn parameters complete it (slot 2); the port
// passes the parameters straight to the constructor. The physics world's
// body counter (+0x74, +0x30) is advanced before the template is resolved.
Entity* HoverVehicleDef::Spawn(entity::EntityPlacement* placement)
{
    ResolveBodyTemplate();
    auto* vehicle = new HoverVehicle(this, *placement);
    vehicle->OnSpawn();
    return vehicle;
}

// ===========================================================================
// Entity

// @func 0x002404F0 HoverVehicle::HoverVehicle
HoverVehicle::HoverVehicle(HoverVehicleDef* hoverDef, const entity::EntityPlacement& placement)
    : VehicleEntity(hoverDef)
{
    (void)placement;
    // NOTE: the console copies word +0xC8 of the spawn description here; the
    // port's spawn parameters do not carry it yet.
    skinValue = 0;

    if (PhysicsBody* tmpl = hoverDef->bodyTemplate) {
        // The body sits at the definition's offset from the placed object.
        const ape::Vec3& offset = hoverDef->settings.field_3F0;
        transform.translation.x += offset.x;
        transform.translation.y += offset.y;
        transform.translation.z += offset.z;
        objectRestoreTransform.translation.x += offset.x;
        objectRestoreTransform.translation.y += offset.y;
        objectRestoreTransform.translation.z += offset.z;

        body = CloneBody(tmpl, object ? object->GetName() : "<unknown>");
        if (body) {
            BodySetTransform(body, transform);
            BodySetOwner(body, static_cast<Entity*>(this));
            BodyAddContactListener(body, static_cast<entity::EntityInterface17C*>(this));
            BodyAddEntityListener(body, static_cast<entity::EntityInterface180*>(this));
            AddBodyToWorld(body);
        }
    }

    skin = entity::EntitySkin(this);
    if (skin)
        SkinSetValue4D8(skin, skinValue);
    StartTurboScript();
    CreateControllers();
    PlayerSetPowerCapacity(PlayerSlot(), hoverDef->settings.turboField3A0);
    if (skin)
        SkinSetFiring(skin, false);

    moveDirection = {0.0f, 0.0f, 0.0f, 0.0f};
    throttle = 0.0f;
    turboBlend = 0.0f;
    if (animScript) {
        ape::ApeAnimScriptState* state = animScript->GetState();
        scriptState = state ? state->GetNameHash() : 0;
    }
    leashPoint = {0.0f, 0.0f, 0.0f, 0.0f};
    leashRadiusSquared = 0.0f;
    leashDuration = 0.0f;
    leashTimer = 0.0f;
    leashEnabled = false;
}

// @func 0x00240E8C HoverVehicle::~HoverVehicle
// @func 0x00240F6C HoverVehicle::~HoverVehicle (deleting)
// @func 0x00240FB4 HoverVehicle::~HoverVehicle (entity listener thunk, deleting)
// @func 0x00241000 HoverVehicle::~HoverVehicle (contact listener thunk, deleting)
// @func 0x0024104C HoverVehicle::~HoverVehicle (entity listener thunk)
// @func 0x00241074 HoverVehicle::~HoverVehicle (contact listener thunk)
HoverVehicle::~HoverVehicle()
{
    ReleaseBody(body);
    body = nullptr;
}

// @func 0x0023FFD8 HoverVehicle::CreateControllers
void HoverVehicle::CreateControllers()
{
    if (classBits & 1) {
        // A player drives it with the pad (the radar pad controller).
        components::ControlComponentDesc desc;
        desc.name = ape::HashNameNoCase("TmpControl");
        desc.type = kControlType_RadarPad;
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
}

// @func 0x00240288 HoverVehicle::StartTurboScript
void HoverVehicle::StartTurboScript()
{
    ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0);
    if (!library)
        return;
    ape::ApeAnimScript* script = library->FindScript(HoverDef()->settings.turboScript);
    if (!script)
        return;
    turboScript = new ape::ApeAnimScriptInstance(script);
    ape::ApeAnimScriptVar* self = turboScript->FindVar(kScriptVar_This);
    if (self && self->GetType() == ape::AnimScriptVarType::Object)
        static_cast<ape::ApeAnimScriptObjectVar*>(self)->SetObject(entity::EntityWorldObjectRef(this));
    turboScript->SetRemoveWhenFinished(false);
    ape::ApeAnimScriptInstance::Register(turboScript);
    turboScript->Update(1.0f / 60.0f);
}

void HoverVehicle::SetTurboScriptState(uint32_t state)
{
    turboScriptState = state;
    if (turboScript)
        turboScript->SetStateAndUpdate(state);
}

// @func 0x0024109C HoverVehicle::Release
bool HoverVehicle::Release(bool kill)
{
    if (kill) {
        // The scripts finish on their own once the vehicle is gone.
        if (field_1A8) {
            field_1A8->SetRemoveWhenFinished(true);
            field_1A8 = nullptr;
            field_1AC = 0;
            field_1B0 = 0;
        }
        if (turboScript) {
            turboScript->SetRemoveWhenFinished(true);
            turboScript = nullptr;
        }
        PlayerResetPower(PlayerSlot());
    }
    return VehicleEntity::Release(kill);
}

// @func 0x0023FF8C HoverVehicle::ResetState
void HoverVehicle::ResetState()
{
    if (skin)
        SkinSetFiring(skin, false);
    moveDirection = {0.0f, 0.0f, 0.0f, 0.0f};
    throttle = 0.0f;
    turboBlend = 0.0f;
}

// @func 0x002411B0 HoverVehicle::ReadSteerInput
// The stick of pad 0 (outside its dead zone) or the d-pad, scaled per axis;
// the d-pad wins. While the leash pulls, the direction toward the leash point
// on screen replaces the pad.
void HoverVehicle::ReadSteerInput(float out[3]) const
{
    const HoverVehicleSettings& s = HoverDef()->settings;
    const uint32_t buttons = PadButtons(0);
    auto held = [buttons](uint32_t bit) { return (buttons & bit) ? 1.0f : 0.0f; };
    const float dpadX = (held(kPad_DpadRight) - held(kPad_DpadLeft)) * s.dpadScaleX;
    const float dpadY = (held(kPad_DpadUp) - held(kPad_DpadDown)) * s.dpadScaleY;

    const float stickX = PadAxis(0, 0);
    const float stickY = PadAxis(0, 1);
    const float magnitude = std::sqrt(stickX * stickX + stickY * stickY);
    const float strength = Saturate((magnitude - s.deadZone) / (1.0f - s.deadZone));
    float x = 0.0f;
    float y = 0.0f;
    if (dpadX == 0.0f && dpadY == 0.0f) {
        x = stickX * strength * s.stickScaleX;
        y = stickY * strength * s.stickScaleY;
    }
    out[0] = std::max(std::min(dpadX + x, 1.0f), -1.0f);
    out[1] = std::max(std::min(dpadY + y, 1.0f), -1.0f);

    if (leashEnabled && leashTimer > 0.0f) {
        float screen[2];
        MainViewWorldToScreen(leashPoint, screen);
        const float dx = Saturate(screen[0]) - 0.5f;
        const float dy = Saturate(screen[1]) - 0.5f;
        const float inv = 1.0f / std::sqrt(dx * dx + dy * dy);
        out[0] = dx * inv;
        out[1] = dy * inv;
        out[2] = 0.0f;
    }
}

// TODO(0x00241500 HoverVehicle::UpdateMoveDirection): the scripted-input
// path is ported; the pad path (camera-relative steering from
// ReadSteerInput) has bad instruction data in the disassembly.
float HoverVehicle::UpdateMoveDirection()
{
    if (scriptedInputs.Count() == 0)
        return -1.0f;
    float x = 0.0f;
    float z = 0.0f;
    for (const ape::Vec3& input : scriptedInputs) {
        x += input.x;
        z += input.z;
    }
    scriptedInputs.Clear();
    const float inv = 1.0f / std::sqrt(x * x + z * z);
    moveDirection = {x * inv, 0.0f, z * inv, 0.0f};
    return HoverDef()->settings.maxSpeed;
}

// TODO(0x002418B0 HoverVehicle::UpdateFlight): the flight model (speed,
// turbo blend, roll/pitch and the body velocities) is vector code with bad
// instruction data in the disassembly.
void HoverVehicle::UpdateFlight(float dt)
{
    (void)dt;
    if (!(vehicleFlags & kVehicle_Enabled) || !body || !BodyInWorld(body))
        return;
    UpdateMoveDirection();
}

// @func 0x002428E0 HoverVehicle::UpdateTurbo
void HoverVehicle::UpdateTurbo(float dt)
{
    if (!(vehicleFlags & kVehicle_Enabled) || scriptedInputs.Count() != 0)
        return;
    const HoverVehicleSettings& s = HoverDef()->settings;
    const uint32_t player = PlayerSlot();
    const bool released = (PadButtons(PlayerPadIndex(player)) & kPad_Boost) == 0;
    float power = PlayerPower(player);

    // The boost arms once the button is up with enough power, and disarms
    // when the power runs out.
    if (turboArmed && power <= 0.0f)
        turboArmed = false;
    else if (!turboArmed && released && s.turboMinPower <= power)
        turboArmed = true;

    const bool wasOn = turboActive != 0.0f;
    bool on = false;
    if (turboArmed) {
        const bool overTime = wasOn && s.turboMaxTime > 0.0f && turboTime > s.turboMaxTime;
        if (overTime)
            turboArmed = false;
        else if ((wasOn && turboTime < s.turboMinTime) || !released)
            on = true;
    }
    turboActive = on ? 1.0f : 0.0f;

    float time = 0.0f;
    if (on) {
        const float startCost = wasOn ? 0.0f : s.turboStartCost;
        power = std::max(power - (s.turboCostRate * dt + startCost), 0.0f);
        PlayerMarkPowerUsed(player);
        PlayerSetPower(player, power);
        time = turboTime + dt;
    }
    turboTime = time;

    if (wasOn != on) {
        if (entity::EntityComponent* driver = GetComponent(entity::kComp_17))
            AnimDriverRequestState(driver, on ? kDriver_Turbo : kDriver_Idle);
    }
}

// @func 0x00242B38 HoverVehicle::Update
bool HoverVehicle::Update(float dt)
{
    const HoverVehicleDef* hoverDef = HoverDef();
    if (!VehicleEntity::Update(dt))
        return false;
    if (controller)
        controller->Update(dt);

    if (leashEnabled) {
        leashTimer = std::max(leashTimer - dt, 0.0f);
        const float dx = leashPoint.x - transform.translation.x;
        const float dz = leashPoint.z - transform.translation.z;
        if (leashRadiusSquared <= dx * dx + dz * dz)
            leashTimer = leashDuration;
    }

    cachedVelocity = GetVelocity();
    cachedAngularVelocity = GetAngularVelocity();
    UpdateTurbo(dt);
    if (hoverDef && hoverDef->settings.flightEnabled)
        UpdateFlight(dt);

    // The effects script follows the turbo, passing through the matching
    // "End" state when it changes.
    const uint32_t wanted = turboActive != 0.0f ? kTurboState_StartOn : kTurboState_StartOff;
    if (turboScriptState != wanted) {
        if (turboScriptState == kTurboState_StartOn)
            SetTurboScriptState(kTurboState_EndOn);
        else if (turboScriptState == kTurboState_StartOff)
            SetTurboScriptState(kTurboState_EndOff);
    }
    if (turboScriptState != wanted)
        SetTurboScriptState(wanted);
    return true;
}

// @func 0x00242DF8 HoverVehicle::CheckExplode
// A contact during the destruction animation blew the vehicle up: freeze
// its animation, hold the camera where it is and run the "destroy" state of
// the vehicle's script.
void HoverVehicle::CheckExplode()
{
    if (!explodePending)
        return;
    entity::EntitySetState(this, 1, kDestroy_Exploding);
    explodePending = false;
    if (entity::EntityComponent* character = GetCharacterComponent())
        CharacterPauseLayer(character, 0);
    else
        SetAnimPaused(0, true);

    if (entity::GameCamera* camera = entity::MainGameCamera()) {
        auto* hold = new camera::FixedCameraMode(camera);
        hold->transform = camera->transform;
        hold->transform.translation.pad = 0.0f;
        hold->fov = -1.0f;
        hold->dofIterations = 1;
        camera->PushMode(-1.0f, hold, false);
    }
    if (animScript && scriptState != kScriptState_Destroy) {
        scriptState = kScriptState_Destroy;
        ScriptedEntity::SetTimeScale(kScriptState_Destroy, true);
    }
}

// @func 0x0024356C HoverVehicle::SyncFromBody
void HoverVehicle::SyncFromBody()
{
    if (!body)
        return;
    const ape::Matrix34 m = BodyTransform(body);
    transform.xAxis = {m.xAxis.x, m.xAxis.y, m.xAxis.z, transform.xAxis.pad};
    transform.yAxis = {m.yAxis.x, m.yAxis.y, m.yAxis.z, transform.yAxis.pad};
    transform.zAxis = {m.zAxis.x, m.zAxis.y, m.zAxis.z, transform.zAxis.pad};
    transform.translation = {m.translation.x, m.translation.y, m.translation.z, transform.translation.pad};
    // TODO(0x002430C8 HoverVehicle::AlignBody): without the flight model the
    // console turns the body upright here (vector code, bad instruction data).
    if (object)
        object->UpdateWorld(false, false);
}

// @func 0x00243728 HoverVehicle::UpdateSkinTarget
void HoverVehicle::UpdateSkinTarget()
{
    if (!(vehicleFlags & kVehicle_Enabled) || !skin)
        return;
    const uint32_t buttons = PadButtons(PlayerPadIndex(PlayerSlot()));
    Entity* target = nullptr;
    if (entity::EntityComponent* list = entity::EntityComponent01(this)) {
        for (Entity* e = Component01Next(list, nullptr); e; e = Component01Next(list, e)) {
            if (e != this) {
                target = e;
                break;
            }
        }
    }
    SkinSetFiring(skin, (buttons & kPad_Fire) != 0);
    SkinSetTarget(skin, target);
}

// @func 0x00243870 HoverVehicle::PostUpdate
bool HoverVehicle::PostUpdate(float dt)
{
    if (entity::EntityGetState(this, 0) == kState_Destroyed && entity::EntityGetState(this, 1) == kDestroy_Exploding &&
        scriptState == kScriptState_Destroy && animScript->IsFinished(false)) {
        health = std::clamp(0.0f, 0.0f, std::max(maxHealth, 0.0f));
        entity::EntitySetState(this, 1, kDestroy_Done);
    }
    const bool alive = VehicleEntity::PostUpdate(dt);
    if (alive) {
        UpdateSkinTarget();
        SyncFromBody();
        CheckExplode();
    }
    return alive;
}

// @func 0x002439EC HoverVehicle::Slot67
// Takes the current transform as the one put back on the world object.
void HoverVehicle::Slot67()
{
    objectRestoreTransform = transform;
}

// @func 0x00243A80 HoverVehicle::SetTransform
void HoverVehicle::SetTransform(const ape::Matrix34& m)
{
    objectRestoreTransform.translation.x += m.translation.x - transform.translation.x;
    objectRestoreTransform.translation.y += m.translation.y - transform.translation.y;
    objectRestoreTransform.translation.z += m.translation.z - transform.translation.z;
    VehicleEntity::SetTransform(m);
    if (body) {
        BodySetTransform(body, m);
        BodySetLinearVelocity(body, {0.0f, 0.0f, 0.0f, 0.0f});
        BodySetAngularVelocity(body, {0.0f, 0.0f, 0.0f, 0.0f});
    }
    moveDirection = {0.0f, 0.0f, 0.0f, 0.0f};
    throttle = 0.0f;
    cachedVelocity = {0.0f, 0.0f, 0.0f, 0.0f};
    cachedAngularVelocity = {0.0f, 0.0f, 0.0f, 0.0f};
    std::fill(std::begin(field_200), std::end(field_200), 0.0f);
}

// @func 0x00243E5C HoverVehicle::Slot70
void* HoverVehicle::Slot70(uint32_t nameHash) { return FindBody(nameHash); }

PhysicsBody* HoverVehicle::FindBody(uint32_t nameHash) const
{
    if (!body || ape::HashNameNoCase(HoverDef()->bodyName) != nameHash)
        return nullptr;
    return body;
}

// @func 0x00243EC4 HoverVehicle::SetActive
void HoverVehicle::SetActive(bool active)
{
    if (active && turboScriptState != kTurboState_EndOn)
        SetTurboScriptState(kTurboState_EndOn);
    Entity::SetActive(active);
}

// @func 0x00243F4C HoverVehicle::GetCenterOfMass
ape::Vec3 HoverVehicle::GetCenterOfMass()
{
    return body ? BodyCenterOfMass(body) : transform.translation;
}

// @func 0x00243FF0 HoverVehicle::SetVelocity
void HoverVehicle::SetVelocity(const ape::Vec3& velocity)
{
    if (body)
        BodySetLinearVelocity(body, {velocity.x, velocity.y, velocity.z, 0.0f});
}

// @func 0x002440BC HoverVehicle::GetVelocity
ape::Vec3 HoverVehicle::GetVelocity()
{
    return body ? BodyLinearVelocity(body) : ape::Vec3{0.0f, 0.0f, 0.0f, 0.0f};
}

// @func 0x00244130 HoverVehicle::GetAngularVelocity
ape::Vec3 HoverVehicle::GetAngularVelocity()
{
    return body ? BodyAngularVelocity(body) : ape::Vec3{0.0f, 0.0f, 0.0f, 0.0f};
}

// @func 0x002441E0 HoverVehicle::SetAngularVelocity
void HoverVehicle::SetAngularVelocity(const ape::Vec3& velocity)
{
    if (body)
        BodySetAngularVelocity(body, {velocity.x, velocity.y, velocity.z, 0.0f});
}

// @func 0x002442E0 HoverVehicle::ApplyPointImpulse
void HoverVehicle::ApplyPointImpulse(const ape::Vec3& impulse, const ape::Vec3& point)
{
    if (body && BodyIsDynamic(body) && BodyInWorld(body))
        BodyApplyPointImpulse(body, {impulse.x, impulse.y, impulse.z, 0.0f}, {point.x, point.y, point.z, 0.0f});
}

// @func 0x00244418 HoverVehicle::GetMass
float HoverVehicle::GetMass()
{
    return (body && BodyInWorld(body)) ? BodyMass(body) : 0.0f;
}

// @func 0x00244468 HoverVehicle::SetEnabled
void HoverVehicle::SetEnabled(bool on)
{
    const bool wasOn = (vehicleFlags & kVehicle_Enabled) != 0;
    if (on != wasOn) {
        if (skin)
            SkinSetFiring(skin, false);
        const ape::Vec3 zero{0.0f, 0.0f, 0.0f, 0.0f};
        SetVelocity(zero);
        SetAngularVelocity(zero);
        cachedVelocity = zero;
        cachedAngularVelocity = zero;
        std::fill(std::begin(field_200), std::end(field_200), 0.0f);
    }
    VehicleEntity::SetEnabled(on);
}

// @func 0x0024458C HoverVehicle::OnDestroyed
void HoverVehicle::OnDestroyed()
{
    SetEnabled(false);
    flags |= entity::kEnt_RootMotion;
    VehicleEntity::OnDestroyed();
}

// @func 0x00244630 HoverVehicle::OnContact
// @func 0x002447E8 HoverVehicle::OnContact (contact listener thunk)
void HoverVehicle::OnContact(const void* event)
{
    PhysicsBody* own = body;
    PhysicsBody* other = ContactBody(event, 0);
    if (other == own)
        other = ContactBody(event, 1);
    if (other) {
        const HoverVehicleSettings& s = HoverDef()->settings;
        const uint32_t layer = BodyCollisionLayer(other);
        if (IsWorldLayer(layer) && s.rejectCollisions)
            ContactDisable(event);
        // Hitting something solid while the destruction animation plays
        // blows the vehicle up.
        if (entity::EntityGetState(this, 0) == kState_Destroyed &&
            entity::EntityGetState(this, 1) == kDestroy_Animating) {
            if (IsDestroyingLayer(layer))
                explodePending = true;
            else if (s.rejectCollisions)
                ContactDisable(event);
        }
    }
    ContactScaleResponse(event, body, kContactResponseScale);
}

// @func 0x0075DAA4 HoverVehicle::GetRenderObject
ape::world::ApeObj* HoverVehicle::GetRenderObject() { return reinterpret_cast<ape::world::ApeObj*>(body); }

// @func 0x0075DA88 HoverVehicle::GetUpVector
ape::Vec3 HoverVehicle::GetUpVector() { return upVector; }

// @func 0x0075DAC8 HoverVehicle::StopContactListening
void HoverVehicle::StopContactListening(PhysicsBody* other)
{
    BodyRemoveContactListener(other, static_cast<entity::EntityInterface17C*>(this));
}

// @func 0x00244810 HoverVehicle::QueueScriptedInput
void HoverVehicle::QueueScriptedInput(const ape::Vec3& direction)
{
    scriptedInputs.PushBack({direction.x, direction.y, direction.z, 0.0f});
}

// @func 0x002448C0 HoverVehicle::SetLeashEnabled
void HoverVehicle::SetLeashEnabled(bool enabled)
{
    leashTimer = 0.0f;
    leashEnabled = enabled;
    if (leashRadiusSquared <= 0.0f)
        leashRadiusSquared = 1.0f;
}

// @func 0x00244900 HoverVehicle::SetLeash
void HoverVehicle::SetLeash(float radiusSquared, float duration, const ape::Vec3& point)
{
    leashPoint = {point.x, point.y, point.z, 0.0f};
    leashRadiusSquared = radiusSquared > 0.0f ? radiusSquared : 1.0f;
    leashDuration = duration;
    leashTimer = 0.0f;
    leashEnabled = true;
}

} // namespace objects
} // namespace sly
