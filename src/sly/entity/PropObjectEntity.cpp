// openTIT - Prop objects (see PropObjectEntity.h).
#include "sly/entity/PropObjectEntity.h"

#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/PhysicsHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/memory/ApeMem.h"
#include "ape/world/ApeObj.h"

namespace sly {
namespace entity {

namespace {
constexpr uint32_t kState_Spawn = 0x04B6FB5Bu;    // "Spawn"
constexpr uint32_t kState_Despawn = 0x879D1B7Au;  // "Despawn"
constexpr uint32_t kState_Suspend = 0xE69B8987u;
constexpr uint32_t kState_Resume = 0xD5E4206Fu;
constexpr uint32_t kState_Contact = 0x04B8CC42u;  // NOTE: name unknown
constexpr int32_t kLodField_Faded = 6;

ape::Matrix34 InverseRigid(const ape::Matrix34& m)
{
    ape::Matrix34 inv;
    inv.xAxis = { m.xAxis.x, m.yAxis.x, m.zAxis.x, 0.0f };
    inv.yAxis = { m.xAxis.y, m.yAxis.y, m.zAxis.y, 0.0f };
    inv.zAxis = { m.xAxis.z, m.yAxis.z, m.zAxis.z, 0.0f };
    const ape::Vec3& t = m.translation;
    inv.translation = { -(t.x * m.xAxis.x + t.y * m.xAxis.y + t.z * m.xAxis.z),
                        -(t.x * m.yAxis.x + t.y * m.yAxis.y + t.z * m.yAxis.z),
                        -(t.x * m.zAxis.x + t.y * m.zAxis.y + t.z * m.zAxis.z), 0.0f };
    return inv;
}

bool IsSimulated(void* body)
{
    const uint8_t motion = RigidBodyMotionType(body);
    return motion != kMotionType_Fixed && motion != kMotionType_Keyframed;
}

// Drops a body with its constraints and the game's user properties.
void ReleaseBody(void* body)
{
    if (RigidBodyInWorld(body))
        RigidBodyRemoveFromWorld(body);
    for (int i = 0; i < RigidBodyConstraintCount(body); ++i)
        HavokRelease(RigidBodyConstraint(body, i));
    RigidBodyClearUserProperties(body);
    HavokRelease(body);
}
} // namespace

// @func 0x000DD8A4 PropObjectDef::~PropObjectDef
// @func 0x000DD9B0 PropObjectDef::~PropObjectDef (deleting)
PropObjectDef::~PropObjectDef()
{
    if (bodyTemplate)
        HavokRelease(bodyTemplate);
}

PropObjectEntity::~PropObjectEntity()
{
    if (alphaBuffer)
        ape::MemFree(alphaBuffer);
}

// @func 0x000DE56C PropObjectEntity::OnSpawn
// Like the entity's, but the children are placed directly; the body reports
// its contacts only when someone wants them (a contact script or an AI).
void PropObjectEntity::OnSpawn()
{
    GameObjectSpawn(this);
    if (renderId == 0 || renderId == 0xFFFFFFFFu)
        renderId = NewRenderId();
    SetRenderId(renderId);
    if (components) {
        for (EntityComponent* component : *components)
            component->OnOwnerSpawned(this);
    }
    UpdateChildTransforms();
    if (!body)
        return;
    if (!Def()->contactScript && !GetComponent0D())
        return;
    RigidBodyAddContactListener(body, static_cast<EntityInterface17C*>(this));
    propFlags &= ~uint32_t(kProp_Contact);
}

// @func 0x000DE8D8 PropObjectEntity::Update
bool PropObjectEntity::Update(float dt)
{
    if (!Entity::Update(dt))
        return false;
    if (propFlags & kProp_Contact) {
        propFlags &= ~uint32_t(kProp_Contact);
        if (def && Def()->contactScript) {
            if (ape::ApeAnimScriptInstance* instance = StartScript(Def()->contactScript, 0, true))
                instance->SetStateAndUpdate(kState_Contact);
        }
    }
    if (propFlags & kProp_SpawnPending) {
        if (script) {
            // NOTE: this class asks for placement flag bit 6 (record +0x0C);
            // the hook applies the scripted entities' test.
            if ((propFlags & kProp_UsePlacement) && object)
                ApplyPlacementToScript(script, object);
            script->SetStateAndUpdate(kState_Spawn);
        }
        propFlags &= ~uint32_t(kProp_SpawnPending);
    }
    if ((propFlags & kProp_ScriptOneShot) && script && script->IsFinished(false)) {
        script->SetRemoveWhenFinished(true);
        script = nullptr;
        propFlags &= ~uint32_t(kProp_ScriptOneShot);
    }
    if (body && !RigidBodyInWorld(body))
        RigidBodyAddToWorld(body, ~0u);
    return true;
}

// @func 0x000DEAE0 PropObjectEntity::PostUpdate
// False once the lifetime is used up.
bool PropObjectEntity::PostUpdate(float dt)
{
    if (!Entity::PostUpdate(dt))
        return false;
    if (body && RigidBodyInWorld(body) && IsSimulated(body))
        SyncFromBody();
    const float lifetime = Def()->lifetime;
    if (lifetime < 0.0f)
        return true;
    age += dt;
    return age < lifetime;
}

// @func 0x000DE6DC PropObjectEntity::Release
bool PropObjectEntity::Release(bool kill)
{
    if (kill) {
        if (script) {
            script->SetActive(true);
            script->SetState(kState_Despawn);
            script->SetRemoveWhenFinished(true);
            script = nullptr;
        }
        if (body)
            ReleaseBody(body);
    }
    return Entity::Release(kill);
}

// @func 0x000DEF4C PropObjectEntity::SetTransform
void PropObjectEntity::SetTransform(const ape::Matrix34& m)
{
    Entity::SetTransform(m);
    PlaceBody();
}

// @func 0x000DDA30 PropObjectEntity::PlaceBody
// The body sits at the template's offset in the entity's frame.
void PropObjectEntity::PlaceBody()
{
    if (!body)
        return;
    ape::Matrix34 placed;
    ape::world::Matrix34Mul(placed, RigidBodyTransform(Def()->bodyTemplate), transform);
    RigidBodySetTransform(body, placed);
}

// @func 0x000DF010 PropObjectEntity::SyncFromBody
void PropObjectEntity::SyncFromBody()
{
    if (!body)
        return;
    const ape::Matrix34 offset = InverseRigid(RigidBodyTransform(Def()->bodyTemplate));
    ape::Matrix34 frame;
    ape::world::Matrix34Mul(frame, offset, RigidBodyTransform(body));
    Entity::SetTransform(frame);
}

// @func 0x000DEE68 PropObjectEntity::SetFlag80
// Hiding takes the body out of the world; showing puts it back where the
// entity is.
void PropObjectEntity::SetFlag80(bool on, int part)
{
    (void)part;
    flags = on ? (flags | kEnt_80) : (flags & ~uint32_t(kEnt_80));
    if (!body || RigidBodyInWorld(body) != on)
        return;
    if (on) {
        RigidBodyRemoveFromWorld(body);
    } else {
        RigidBodyAddToWorld(body, ~0u);
        SetTransform(transform);
    }
}

// @func 0x000DEC64 PropObjectEntity::SetTimeScale
void PropObjectEntity::SetTimeScale(uint32_t value, bool children)
{
    if (script)
        script->SetStateAndUpdate(value);
    Entity::SetTimeScale(value, children);
}

// @func 0x000DED00 PropObjectEntity::SetVelocity
void PropObjectEntity::SetVelocity(const ape::Vec3& v)
{
    if (body && RigidBodyInWorld(body) && IsSimulated(body))
        RigidBodySetLinearVelocity(body, v);
}

// @func 0x000DEDF4 PropObjectEntity::GetVelocity
ape::Vec3 PropObjectEntity::GetVelocity()
{
    return body ? RigidBodyLinearVelocity(body) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x000DF500 PropObjectEntity::OnSuspend
void PropObjectEntity::OnSuspend()
{
    Entity::OnSuspend();
    if (script) {
        script->SetStateAndUpdate(kState_Suspend);
        script->SetActive(false);
    }
}

// @func 0x000DF650 PropObjectEntity::OnResume
void PropObjectEntity::OnResume()
{
    Entity::OnResume();
    if (script) {
        script->SetActive(true);
        script->SetState(kState_Resume);
    }
}

// @func 0x000DF7E0 PropObjectEntity::Interface17CSlot5
// @func 0x000DF848 PropObjectEntity::Interface17CSlot5 (thunk, +0x158)
void PropObjectEntity::Interface17CSlot5(const void* event)
{
    propFlags |= kProp_Contact;
    if (listener)
        ListenerOnContact(listener, event);
}

// @func 0x000DF330 PropObjectEntity::SetAlphaBuffer
void PropObjectEntity::SetAlphaBuffer(const uint32_t* values, int count)
{
    if (!values || alphaBuffer)
        return;
    alphaBuffer = static_cast<uint32_t*>(ape::MemAlloc(static_cast<size_t>(count) * 4, ape::kMemClass_Default, 0x10));
    for (int i = 0; i < count; ++i)
        alphaBuffer[i] = values[i];
}

// @func 0x000DF3CC PropObjectEntity::SetFadeAlpha
void PropObjectEntity::SetFadeAlpha(bool fading, uint8_t newAlpha, bool immediate)
{
    if (!fading && alphaBuffer) {
        ape::MemFree(alphaBuffer);
        alphaBuffer = nullptr;
    }
    alpha = newAlpha;
    SetAlpha(newAlpha, false, false, immediate);
    if (fading) {
        const int32_t lod = RenderObjectLodField(object);
        if (lod != kLodField_Faded) {
            savedLodField = lod;
            RenderObjectSetLodField(object, kLodField_Faded);
        }
        return;
    }
    RenderObjectSetLodField(object, savedLodField);
}

// @func 0x000DF4CC PropObjectEntity::ApplyFadeBuffer
void PropObjectEntity::ApplyFadeBuffer()
{
    ObjectApplyAlphaBuffer(object, alphaBuffer, alpha);
}

// Characters fade the prop objects they hold (type bit 70).
void EntitySetFadeAlpha(Entity* entity, bool flag, uint8_t alpha, bool immediate)
{
    static_cast<PropObjectEntity*>(entity)->SetFadeAlpha(flag, alpha, immediate);
}

void EntityApplyFadeBuffer(Entity* entity)
{
    static_cast<PropObjectEntity*>(entity)->ApplyFadeBuffer();
}

} // namespace entity
} // namespace sly
