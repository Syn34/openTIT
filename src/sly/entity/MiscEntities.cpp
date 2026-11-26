// openTIT - Small overrides of the vtable-only entity classes (see MiscEntities.h).
#include "sly/entity/MiscEntities.h"

#include "sly/entity/CharacterHooks.h"
#include "sly/entity/PhysicsHooks.h"
#include "sly/game/SlyGame.h"

#include "ape/anim/ApeAnimScript.h"

namespace sly {
namespace entity {

// @func 0x002070E8 RigidEntityB19198::GetRenderId
uint32_t RigidEntityB19198::GetRenderId()
{
    if (RigidBodyInWorld(rigidBody))
        return RigidBodyFilterInfo(rigidBody) >> 16;
    return groupBits & 0x00FFFFFFu;
}

// @func 0x00207114 RigidEntityB19198::SetRenderId
void RigidEntityB19198::SetRenderId(uint32_t id)
{
    groupBits = (groupBits & 0xFF000000u) | (id & 0x00FFFFFFu);
    if (RigidBodyInWorld(rigidBody))
        RigidBodySetFilterInfo(rigidBody, (id << 16) | (RigidBodyFilterInfo(rigidBody) & 0x1Fu));
    Entity::SetRenderId(id);
}

// @func 0x002071AC RigidEntityB19198::GetGravityFactor
float RigidEntityB19198::GetGravityFactor()
{
    if (attachParent) {
        const EntityChildLink* link = attachParent->FindChildLink(this);
        return link ? link->restoreBlend : 0.0f;
    }
    return RigidBodyGravityFactor(rigidBody);
}

// @func 0x00207204 RigidEntityB19198::Detached
void RigidEntityB19198::Detached(float gravityFactor)
{
    RigidBodySetGravityFactor(rigidBody, gravityFactor);
    Entity::Detached(gravityFactor);
}

// @func 0x00206F6C RigidEntityB19198::GetVelocity
ape::Vec3 RigidEntityB19198::GetVelocity()
{
    return RigidBodyLinearVelocity(rigidBody);
}

// @func 0x00208D60 RigidEntityB19198::ObjectForCollidable
ape::world::ApeObj* RigidEntityB19198::ObjectForCollidable(const void* collidable, int32_t* bone)
{
    if (rigidBody && CollidableOwnerBody(collidable) == rigidBody) {
        *bone = -1;
        return object;
    }
    return Entity::ObjectForCollidable(collidable, bone);
}

// @func 0x00758FF0 RigidEntityB19198::~RigidEntityB19198
// @func 0x00759038 RigidEntityB19198::~RigidEntityB19198 (deleting)
// @func 0x007591EC RigidEntityB19198::~RigidEntityB19198 (thunk, +0x17C)
// @func 0x00759124 RigidEntityB19198::~RigidEntityB19198 (deleting thunk, +0x17C)
// @func 0x0075919C RigidEntityB19198::~RigidEntityB19198 (thunk, +0x180)
// @func 0x007590AC RigidEntityB19198::~RigidEntityB19198 (deleting thunk, +0x180)
RigidEntityB19198::~RigidEntityB19198() = default;

// @func 0x0075923C RigidEntityB19198::GetRenderObject
ape::world::ApeObj* RigidEntityB19198::GetRenderObject() { return static_cast<ape::world::ApeObj*>(rigidBody); }

// @func 0x00759244 RigidEntityB19198::Slot95
void RigidEntityB19198::Slot95() {}

// @func 0x00759250 RigidEntityB19198::Interface17CSlot6
// @func 0x00759254 RigidEntityB19198::Interface17CSlot6 (thunk, +0x17C)
void RigidEntityB19198::Interface17CSlot6() {}

// @func 0x00759248 RigidEntityB19198::Interface17CSlot7
// @func 0x0075924C RigidEntityB19198::Interface17CSlot7 (thunk, +0x17C)
void RigidEntityB19198::Interface17CSlot7() {}

// @func 0x00759258 RigidEntityB19198::Interface180Slot6
// @func 0x00759288 RigidEntityB19198::Interface180Slot6 (thunk, +0x180)
void RigidEntityB19198::Interface180Slot6(void* body) { StopListeningTo(body); }

// @func 0x00759438 ScriptedObjectB193C8::~ScriptedObjectB193C8
// @func 0x00759468 ScriptedObjectB193C8::~ScriptedObjectB193C8 (deleting)
ScriptedObjectB193C8::~ScriptedObjectB193C8() = default;

namespace {
constexpr uint32_t kScript_End = 0x00010CBBu;       // "End"
constexpr uint32_t kScript_Suspend = 0xE69B8987u;   // NOTE: names unknown
constexpr uint32_t kScript_Resume = 0xD5E4206Fu;
constexpr int kCallbackList_ScriptedObject = 10;
} // namespace

// @func 0x0020ABD8 ScriptedObjectB193C8::Release
bool ScriptedObjectB193C8::Release(bool kill)
{
    if (kill) {
        UnregisterEntity(kList_B, this);
        if (g_game)
            g_game->RemoveCallback(kCallbackList_ScriptedObject, this, nullptr);
        if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript)) {
            script->SetState(kScript_End);
            script->SetRemoveWhenFinished(true);
            animScript = nullptr;
        }
        delete field_1B0;
        field_1B0 = nullptr;
        if (field_1A0) {
            field_1A0->Release();
            field_1A0 = nullptr;
        }
    }
    return Entity::Release(kill);
}

// @func 0x0020B95C ScriptedObjectB193C8::SetTimeScale
void ScriptedObjectB193C8::SetTimeScale(uint32_t value, bool children)
{
    if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript))
        script->SetStateAndUpdate(value);
    Entity::SetTimeScale(value, children);
}

// @func 0x0020BAC8 ScriptedObjectB193C8::OnSuspend
void ScriptedObjectB193C8::OnSuspend()
{
    Entity::OnSuspend();
    if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript)) {
        script->SetStateAndUpdate(kScript_Suspend);
        script->SetActive(false);
    }
}

// @func 0x0020BB2C ScriptedObjectB193C8::OnResume
void ScriptedObjectB193C8::OnResume()
{
    Entity::OnResume();
    if (auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript)) {
        script->SetActive(true);
        script->SetState(kScript_Resume);
    }
}

// @func 0x00751048 EntityB15028::~EntityB15028
// @func 0x007510B0 EntityB15028::~EntityB15028 (deleting)
EntityB15028::~EntityB15028() = default;

} // namespace entity
} // namespace sly
