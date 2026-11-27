// openTIT - The interaction component (<Interact>, type 0x1B, console vtable
// 0x00B1CE20; constructor 0x002587C0, members 0x002B6E2C-0x002B82C0 and the
// state switch 0x00266E6C). See InteractionComponent in sly/entity/Entity.h.
//
// An interaction runs through the states of InteractionComponent::State:
// "available" until a player comes in range, then the in-range state,
// "active" while someone uses the owner, and back to "available" or (for
// interactions used up after one use) "disabled" when it is withdrawn.
// "inactive" interactions are not offered at all.
#include "sly/components/ComponentDescs.h"
#include "sly/components/ComponentHooks.h"
#include "sly/entity/Character.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/Entity.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/memory/ApeMem.h"
#include "ape/world/ApeObj.h"

#include <cmath>

namespace sly {
namespace entity {

namespace {

constexpr uint32_t kCharacterTypeBit = 0x8u;   // entity type bit of the characters

// Lifetime script state of each interaction state (table 0x00AD4CB4).
constexpr uint32_t kStateNames[5] = {
    ape::AnimHash("available"),
    0x9F632718u,                   // NOTE: name unknown (the in-range state)
    ape::AnimHash("active"),
    ape::AnimHash("disabled"),
    0xA5E5BC74u,                   // NOTE: "inactive" assumed
};

// Script object variable that holds the contextual marker, and the state
// the owner's script enters once the marker is in place.
constexpr uint32_t kVar_ContextualMarker = 0x2D3BE751u;    // NOTE: name unknown
constexpr uint32_t kState_MarkerReady = 0x5C428658u;       // NOTE: name unknown

// Name of the proximity body of an interaction without an owner.
constexpr const char* kDefaultBodyName = "InteractCompRB";

// Radius of the proximity sphere and the description bits copied into the
// component's flag word (bits 23..30 of the description go to 24..31).
constexpr float kBodyRadius = 0.5f;

ape::ApeAnimScriptInstance* OwnerScript(Entity* owner)
{
    return owner ? static_cast<ape::ApeAnimScriptInstance*>(owner->GetAnimScript()) : nullptr;
}

// Switches a script to a state even while the instance is not running.
// NOTE: the console sets and restores bit 8 of the instance flag word (+0x88)
// around the call; read here as the instance's active flag.
void ForceScriptState(ape::ApeAnimScriptInstance* script, uint32_t stateName)
{
    const bool wasActive = script->IsActive();
    script->SetActive(true);
    script->SetStateAndUpdate(stateName);
    script->SetActive(wasActive);
}

// Lets go of an entity reference held by the component.
void DropUser(Entity*& user)
{
    if (user)
        user->Release(false);
    user = nullptr;
}

} // namespace

// @func 0x002587C0 InteractionComponent::InteractionComponent
InteractionComponent::InteractionComponent(const components::InteractComponentDesc* desc)
    : EntityComponent(desc),
      offset(desc->vec20),
      rotation(desc->rotation),
      maxDistSq(desc->interactDistSq),
      halfAngle(desc->halfAngleThreshold),
      userHalfAngle(desc->halfAngle48),
      value74(desc->value50),
      physicsStates(desc->physicsStates)
{
    offset.pad = 0.0f;
    rotation.pad = 0.0f;
    // Description flags 30..24 and the "active, no cull" bit 23 move up by one.
    uint32_t copied = 0;
    for (int i = 0; i < 7; ++i) {
        if (desc->flags[i])
            copied |= 1u << (31 - i);
    }
    if (desc->activeNoCull)
        copied |= kFlag_ActiveNoCull;
    interactFlags = copied | kFlag_ReleaseOnEnd;

    // The proximity body sits on the interaction frame's position.
    ape::Vec3 position = offset;
    if (owner) {
        const ape::Matrix34& m = owner->transform;
        position.x = offset.x * m.xAxis.x + offset.y * m.yAxis.x + offset.z * m.zAxis.x + m.translation.x;
        position.y = offset.x * m.xAxis.y + offset.y * m.yAxis.y + offset.z * m.zAxis.y + m.translation.y;
        position.z = offset.x * m.xAxis.z + offset.y * m.yAxis.z + offset.z * m.zAxis.z + m.translation.z;
    }
    (void)kBodyRadius;   // the radius is part of the body hook's contract
    body = components::CreateInteractionBody(owner ? owner->GetDebugName() : kDefaultBodyName, position, owner);

    if (desc->startAvailable)
        state = kState_Available;
    if (desc->scriptLifetime)
        SetScript(desc->scriptLifetime);
    if (desc->text)
        text = ape::StrDup(desc->text);
}

// @func 0x002B6E2C InteractionComponent::~InteractionComponent
// @func 0x002B7074 InteractionComponent::~InteractionComponent (deleting)
InteractionComponent::~InteractionComponent()
{
    if (user && (user->typeBits[0] & kCharacterTypeBit))
        static_cast<Character*>(user)->LeaveInteractState();
    DropUser(user);
    if (text) {
        ape::MemFree(text);
        text = nullptr;
    }
    if (body)
        components::ReleaseInteractionBody(body);
    if (marker) {
        marker->Release();
        marker = nullptr;
    }
    if (script) {
        ape::ApeAnimScriptInstance::DestroyInstance(script);
        script = nullptr;
    }
}

// @func 0x00266E6C InteractionComponent::SetState
void InteractionComponent::SetState(int32_t newState)
{
    const uint32_t name = kStateNames[newState];
    if (ape::ApeAnimScriptInstance* ownerScript = OwnerScript(owner))
        ForceScriptState(ownerScript, name);
    if (script)
        script->SetStateAndUpdate(name);
    state = newState;
    if ((interactFlags & kFlag_ActiveNoCull) && owner)
        owner->SetBusyFlag(newState == kState_Active);
}

// @func 0x002B70BC InteractionComponent::Update
void InteractionComponent::Update(float dt)
{
    (void)dt;
    if (!(interactFlags & kFlag_StateApplied)) {
        SetState(state);
        interactFlags |= kFlag_StateApplied;
    }
    if (state == kState_Available && playerMask != 0)
        SetState(kState_InRange);
    else if (state == kState_InRange && playerMask == 0)
        SetState(kState_Available);
    if (marker && (interactFlags & kFlag_MarkerHidden))
        marker->SetVisible(false, true);
}

// @func 0x002B719C InteractionComponent::Withdraw
bool InteractionComponent::Withdraw()
{
    if (state != kState_Active && state != kState_Available)
        return false;
    const bool wasActive = state == kState_Active;
    interactFlags |= kFlag_Withdrawn;
    SetState((interactFlags & kFlag_DisableAfterUse) ? kState_Disabled : kState_Available);
    if (wasActive) {
        if (owner)
            owner->InteractionCancel();
        DropUser(user);
        if (onEnded)
            onEnded(this, onEndedContext);
    }
    return true;
}

// @func 0x002B72DC InteractionComponent::Offer
bool InteractionComponent::Offer(Entity* newUser)
{
    if (canStart && !canStart(this, canStartContext))
        return false;
    if (state >= kState_Active)
        return false;
    interactFlags |= kFlag_Started;
    SetState(kState_Active);
    DropUser(user);
    user = newUser;
    newUser->AddRef();
    if (owner && !owner->InteractionBegin(user))
        return false;
    if (interactFlags & kFlag_EndImmediately) {
        if (user && (user->typeBits[0] & kCharacterTypeBit))
            static_cast<Character*>(user)->LeaveInteractState();
        Withdraw();
    }
    return true;
}

// @func 0x002B7560 InteractionComponent::Start
void InteractionComponent::Start(Entity* newUser)
{
    if (!newUser)
        return;
    interactFlags |= kFlag_Started;
    SetState(kState_Active);
    DropUser(user);
    user = newUser;
    newUser->AddRef();
    if (interactFlags & kFlag_EndImmediately)
        Withdraw();
}

// @func 0x002B7704 InteractionComponent::Stop
bool InteractionComponent::Stop()
{
    if (owner)
        owner->InteractionCancel();
    if (user) {
        if (user->typeBits[0] & kCharacterTypeBit)
            static_cast<Character*>(user)->LeaveInteractState();
        user->Release(false);
    }
    user = nullptr;
    SetState(kState_Available);
    return true;
}

// @func 0x002B77E0 InteractionComponent::End
bool InteractionComponent::End(bool completed)
{
    if (state != kState_Active)
        return false;
    SetState(kState_Available);
    if (owner)
        owner->InteractionEnd(completed);
    if (interactFlags & kFlag_ReleaseOnEnd) {
        if (user && (user->typeBits[0] & kCharacterTypeBit))
            static_cast<Character*>(user)->LeaveInteractState();
        DropUser(user);
    }
    return true;
}

// @func 0x002B79F8 InteractionComponent::CanInteract
bool InteractionComponent::CanInteract(const ape::Matrix34& userFrame, const EntityComponent* userPhysics) const
{
    if (state >= kState_Active)
        return false;
    if (userPhysics && !(physicsStates & (1u << PhysicsCompMovementMode(userPhysics))))
        return false;
    if (maxDistSq <= 0.0f && halfAngle < 0.0f)
        return true;

    // The interaction frame: the local rotation and offset, then the owner.
    ape::Vec3 euler = rotation;
    const float hx = euler.x * 0.5f, hy = euler.y * 0.5f, hz = euler.z * 0.5f;
    const float cx = ape::FastCos(hx), cy = ape::FastCos(hy), cz = ape::FastCos(hz);
    const float sx = ape::FastSin(hx), sy = ape::FastSin(hy), sz = ape::FastSin(hz);
    ape::Quat q;
    q.x = sy * sz * cx + cy * cz * sx;
    q.y = cz * sy * cx + cy * sz * sx;
    q.z = cy * sz * cx - cz * sy * sx;
    q.w = cy * cz * cx - sy * sz * sx;
    ape::Matrix34 frame;
    ape::Matrix34FromQuat(frame, q, offset);
    if (owner) {
        ape::Matrix34 world = owner->transform;
        ape::Matrix34PreMultiply(world, frame);
        frame = world;
    }

    // From the user to the frame.
    ape::Vec3 toFrame;
    ape::Vec3Sub(toFrame, frame.translation, userFrame.translation);
    const float distSq = ape::Vec3LengthSq(toFrame);
    bool inRange = true;
    if (maxDistSq > 0.0f)
        inRange = distSq <= maxDistSq;
    bool facing = true;
    if (halfAngle >= 0.0f && inRange) {
        const float inv = 1.0f / std::sqrt(distSq);
        ape::Vec3 dir = {toFrame.x * inv, toFrame.y * inv, toFrame.z * inv, 0.0f};
        const float along = ape::Vec3Dot(dir, frame.zAxis);
        if (along < 0.0f || !(interactFlags & kFlag_FrontOnly)) {
            // The user must stand inside the cone in front of the frame.
            const float angle = std::acos(ape::Clamp(along, -1.0f, 1.0f));
            facing = 3.14159274f - angle <= halfAngle;
            // ... and face the frame closely enough.
            if (facing && userHalfAngle > 0.0f && ape::Vec3Dot(dir, userFrame.zAxis) < ape::FastCos(userHalfAngle))
                facing = false;
        } else {
            facing = false;
        }
    }
    return facing && inRange;
}

// @func 0x002B78F0 InteractionComponent::SetScript
void InteractionComponent::SetScript(uint32_t scriptName)
{
    if (!scriptName)
        return;
    ape::ApeAnimScript* source = ape::ApeAnimScriptLibrary::FindScriptAnywhere(scriptName, 0, true);
    if (!source)
        return;
    if (script)
        ape::ApeAnimScriptInstance::DestroyInstance(script);
    script = new ape::ApeAnimScriptInstance(source);
    ape::ApeAnimScriptInstance::Register(script);
    // NOTE: bit 9 of the instance flags is cleared: read as "not removed
    // when finished".
    script->SetRemoveWhenFinished(false);
}

// @func 0x002B7EA0 InteractionComponent::SetContextualMarker
void InteractionComponent::SetContextualMarker(ape::world::ApeObj* templateObject)
{
    if (!owner)
        return;
    ape::ApeAnimScriptInstance* ownerScript = OwnerScript(owner);
    ape::ApeAnimScriptVar* markerVar = nullptr;
    if (ownerScript) {
        ape::ApeAnimScriptVar* var = ownerScript->FindVar(kVar_ContextualMarker);
        if (var && var->GetType() == ape::AnimScriptVarType::Object) {
            if (ape::world::ApeObj* held = components::ScriptObjectVarSceneObject(var))
                held->Release();
            components::ScriptObjectVarSetSceneObject(var, nullptr);
            markerVar = var;
        }
    }

    // The previous marker goes away.
    if (marker) {
        if (ape::world::ApeObj* parent = marker->GetParent())
            parent->DetachChild(marker);
        marker->Release();
        marker = nullptr;
    }

    if (!(interactFlags & kFlag_NoMarker) && markerVar && templateObject) {
        const uint32_t defName = owner->def ? owner->def->nameHash : 0;
        marker = templateObject->Clone(5);
        if (marker) {
            marker->SetName(ape::TempFormat("Contextual_%s", ape::TempHexString(defName)), true);
            ape::Matrix34 local = marker->GetLocal();
            local.translation = {owner->transform.translation.x, owner->transform.translation.y,
                                 owner->transform.translation.z, local.translation.pad};
            marker->SetLocal(local);
            marker->MarkDirty();
            const bool visible = state < kState_Active && enabled && !(interactFlags & kFlag_MarkerHidden);
            marker->SetVisible(visible, true);
        }
    }

    if (markerVar && ownerScript) {
        if (marker)
            marker->AddRef();
        components::ScriptObjectVarSetSceneObject(markerVar, marker);
        ForceScriptState(ownerScript, kState_MarkerReady);
    }
}

// @func 0x002B81FC InteractionComponent::SetMarkerVisible
void InteractionComponent::SetMarkerVisible(bool visible)
{
    ape::ApeAnimScriptInstance* ownerScript = OwnerScript(owner);
    if (!ownerScript)
        return;
    // NOTE: the console resolves the variable to a scene object and calls its
    // SetVisible (virtual slot 10); see ScriptObjectVarSceneObject.
    ape::ApeAnimScriptVar* var = ownerScript->FindVar(kVar_ContextualMarker);
    if (!var || var->GetType() != ape::AnimScriptVarType::Object)
        return;
    if (ape::world::ApeObj* object = components::ScriptObjectVarSceneObject(var))
        object->SetVisible(visible && !(interactFlags & kFlag_MarkerHidden), true);
}

// @func 0x002B82C0 InteractionComponent::ClearUser
void InteractionComponent::ClearUser()
{
    DropUser(user);
}

} // namespace entity
} // namespace sly
