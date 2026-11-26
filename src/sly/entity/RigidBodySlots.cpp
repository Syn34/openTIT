// openTIT - Small virtual overrides of the rigid body entities (0x0074EF70-
// 0x0074F670, 0x00753258-0x0075396C, 0x00758248-0x00758E90): the body
// accessors, the Havok listener callbacks the classes override and the
// destructors of the classes that own a nav obstacle handle.
//
// Each class has its own copy of these methods on the console. The +0x17C
// interface is the entity's contact listener and the +0x180 interface its
// entity listener; overriding one of their slots also adds a primary vtable
// slot, and the interface slot then holds a this-adjusting thunk of the same
// method (tagged on the method as "thunk").
#include "sly/entity/PropEntities.h"

#include "sly/entity/PhysicsHooks.h"
#include "sly/levels/nav/NavGraph.h"

namespace sly {
namespace entity {

namespace {

// Destructor of the nav obstacle handle these entities hold (NavPower side,
// 0x00638498): the obstacle leaves the level's navigation.
void ReleaseNavObstacle(void*& handle)
{
    if (!handle)
        return;
    if (levels::NavManager* nav = levels::GetNavManager())
        levels::RemoveNavObstacle(*nav, handle);
    handle = nullptr;
}

ape::world::ApeObj* BodyAsObject(void* body)
{
    // NOTE: slot 36 is declared as returning a world object, but the
    // rigid body entities hand out their Havok body.
    return static_cast<ape::world::ApeObj*>(body);
}

} // namespace

void RigidBodyEntity::StopListeningTo(void* body)
{
    RigidBodyRemoveContactListener(body, static_cast<EntityInterface17C*>(this));
}

// -- RigidProp (vtable 0x00B143A8) --------------------------------------------------

// @func 0x0074EF8C RigidProp::GetRenderObject
ape::world::ApeObj* RigidProp::GetRenderObject() { return BodyAsObject(rigidBody); }

// @func 0x0074EF70 RigidProp::GetUpVector
ape::Vec3 RigidProp::GetUpVector() { return { field_1A0.x, field_1A0.y, field_1A0.z, 0.0f }; }

// @func 0x0074EF94 RigidProp::Slot95
void RigidProp::Slot95() {}

// @func 0x0074EFA0 RigidProp::Interface17CSlot6
// @func 0x0074EFA4 RigidProp::Interface17CSlot6 (thunk, +0x17C)
void RigidProp::Interface17CSlot6() {}

// @func 0x0074EF98 RigidProp::Interface17CSlot7
// @func 0x0074EF9C RigidProp::Interface17CSlot7 (thunk, +0x17C)
void RigidProp::Interface17CSlot7() {}

// @func 0x0074EFA8 RigidProp::Interface180Slot6
// @func 0x0074EFD8 RigidProp::Interface180Slot6 (thunk, +0x180)
void RigidProp::Interface180Slot6(void* body) { StopListeningTo(body); }

// -- CompoundEntity (vtable 0x00B14730) ----------------------------------------------

// @func 0x0074F188 CompoundEntity::GetRenderObject
ape::world::ApeObj* CompoundEntity::GetRenderObject() { return BodyAsObject(rigidBody); }

// @func 0x0074F1F4 CompoundEntity::GetUpVector
ape::Vec3 CompoundEntity::GetUpVector() { return { 0.5f, 0.5f, 0.5f, 0.0f }; }

// @func 0x0074F190 CompoundEntity::Slot95
void CompoundEntity::Slot95() {}

// @func 0x0074F194 CompoundEntity::Interface180Slot6
// @func 0x0074F1C4 CompoundEntity::Interface180Slot6 (thunk, +0x180)
void CompoundEntity::Interface180Slot6(void* body) { StopListeningTo(body); }

// -- AlterEgoEntity (vtable 0x00B14968) ----------------------------------------------

// @func 0x0074F2DC AlterEgoEntity::GetRenderObject
ape::world::ApeObj* AlterEgoEntity::GetRenderObject() { return BodyAsObject(rigidBody); }

// @func 0x0074F2E4 AlterEgoEntity::Slot95
void AlterEgoEntity::Slot95() {}

// @func 0x0074F2F0 AlterEgoEntity::Interface17CSlot6
// @func 0x0074F2F4 AlterEgoEntity::Interface17CSlot6 (thunk, +0x17C)
void AlterEgoEntity::Interface17CSlot6() {}

// @func 0x0074F2E8 AlterEgoEntity::Interface17CSlot7
// @func 0x0074F2EC AlterEgoEntity::Interface17CSlot7 (thunk, +0x17C)
void AlterEgoEntity::Interface17CSlot7() {}

// @func 0x001FD57C TargetableCBase::ObjectForCollidable
ape::world::ApeObj* TargetableCBase::ObjectForCollidable(const void* collidable, int32_t* bone)
{
    void* owner = CollidableOwnerBody(collidable);
    if (owner && owner == rigidBody) {
        *bone = -1;
        return object;
    }
    return Entity::ObjectForCollidable(collidable, bone);
}

// @func 0x0017BFDC AlterEgoEntity::ObjectForCollidable
ape::world::ApeObj* AlterEgoEntity::ObjectForCollidable(const void* collidable, int32_t* bone)
{
    void* owner = CollidableOwnerBody(collidable);
    if (owner && owner == rigidBody) {
        *bone = -1;
        return object;
    }
    return Entity::ObjectForCollidable(collidable, bone);
}

// @func 0x0074F2F8 AlterEgoEntity::Interface180Slot6
// @func 0x0074F328 AlterEgoEntity::Interface180Slot6 (thunk, +0x180)
void AlterEgoEntity::Interface180Slot6(void* body) { StopListeningTo(body); }

// -- TargetableA (vtable 0x00B14B98) -------------------------------------------------

// @func 0x0074F460 TargetableA::GetRenderObject
ape::world::ApeObj* TargetableA::GetRenderObject() { return BodyAsObject(rigidBody); }

// @func 0x0074F444 TargetableA::GetUpVector
ape::Vec3 TargetableA::GetUpVector() { return { field_1A0.x, field_1A0.y, field_1A0.z, 0.0f }; }

// @func 0x0074F42C TargetableA::GetSideVector
ape::Vec3 TargetableA::GetSideVector() { return { 0.0f, -7.0f, 0.0f, 0.0f }; }

// @func 0x0074F470 TargetableA::Slot99
void TargetableA::Slot99() {}

// @func 0x0074F468 TargetableA::Interface17CSlot5
// @func 0x0074F46C TargetableA::Interface17CSlot5 (thunk, +0x17C)
void TargetableA::Interface17CSlot5(const void* event) { (void)event; }

// @func 0x0074F47C TargetableA::Interface17CSlot6
// @func 0x0074F480 TargetableA::Interface17CSlot6 (thunk, +0x17C)
void TargetableA::Interface17CSlot6() {}

// @func 0x0074F474 TargetableA::Interface17CSlot7
// @func 0x0074F478 TargetableA::Interface17CSlot7 (thunk, +0x17C)
void TargetableA::Interface17CSlot7() {}

// @func 0x0074F484 TargetableA::Interface180Slot6
// @func 0x0074F4B4 TargetableA::Interface180Slot6 (thunk, +0x180)
void TargetableA::Interface180Slot6(void* body) { StopListeningTo(body); }

// -- TargetableB (vtable 0x00B14DE0) -------------------------------------------------

// @func 0x0074F5F4 TargetableB::GetRenderObject
ape::world::ApeObj* TargetableB::GetRenderObject() { return BodyAsObject(rigidBody); }

// @func 0x0074F5C0 TargetableB::GetUpVector
ape::Vec3 TargetableB::GetUpVector() { return { field_1A0.x, field_1A0.y, field_1A0.z, 0.0f }; }

// @func 0x0074F5DC TargetableB::GetSideVector
ape::Vec3 TargetableB::GetSideVector() { return { 0.0f, -7.0f, 0.0f, 0.0f }; }

// @func 0x0074F5FC TargetableB::Slot99
void TargetableB::Slot99() {}

// @func 0x0074F608 TargetableB::Interface17CSlot6
// @func 0x0074F60C TargetableB::Interface17CSlot6 (thunk, +0x17C)
void TargetableB::Interface17CSlot6() {}

// @func 0x0074F600 TargetableB::Interface17CSlot7
// @func 0x0074F604 TargetableB::Interface17CSlot7 (thunk, +0x17C)
void TargetableB::Interface17CSlot7() {}

// @func 0x0074F610 TargetableB::Interface180Slot6
// @func 0x0074F640 TargetableB::Interface180Slot6 (thunk, +0x180)
void TargetableB::Interface180Slot6(void* body) { StopListeningTo(body); }

// -- PushBlock (vtable 0x00B160A8) ---------------------------------------------------

// @func 0x00753258 PushBlock::~PushBlock
// @func 0x00753360 PushBlock::~PushBlock (deleting)
// @func 0x007536D4 PushBlock::~PushBlock (thunk, +0x180)
// @func 0x00753484 PushBlock::~PushBlock (deleting thunk, +0x180)
// @func 0x007537E0 PushBlock::~PushBlock (thunk, +0x17C)
// @func 0x007535AC PushBlock::~PushBlock (deleting thunk, +0x17C)
PushBlock::~PushBlock()
{
    ReleaseNavObstacle(navObstacle);
}

// @func 0x007538EC PushBlock::GetRenderObject
ape::world::ApeObj* PushBlock::GetRenderObject() { return BodyAsObject(rigidBody); }

// @func 0x007538F4 PushBlock::OnHitCategory0
void PushBlock::OnHitCategory0(EntityHitRecord& hit) { (void)hit; }
// @func 0x007538F8 PushBlock::OnHitCategory2
void PushBlock::OnHitCategory2(EntityHitRecord& hit) { (void)hit; }
// @func 0x007538FC PushBlock::OnHitCategory3
void PushBlock::OnHitCategory3(EntityHitRecord& hit) { (void)hit; }
// @func 0x00753900 PushBlock::OnHitCategory4
void PushBlock::OnHitCategory4(EntityHitRecord& hit) { (void)hit; }

// @func 0x00753904 PushBlock::Damage
bool PushBlock::Damage(float amount, Entity* source, uint32_t type)
{
    (void)amount;
    (void)source;
    (void)type;
    return true;
}

// @func 0x0075390C PushBlock::Interface180Slot6
// @func 0x0075393C PushBlock::Interface180Slot6 (thunk, +0x180)
void PushBlock::Interface180Slot6(void* body)
{
    RigidBodyRemoveContactListener(body, static_cast<EntityInterface17C*>(this));
}

// -- TargetableCBase and its two entity classes (vtables 0x00B18B48, 0x00B18D30,
//    0x00B18F60) ---------------------------------------------------------------------

namespace {
constexpr uint32_t kNavType_None = 0x0024A738u;      // "None"
constexpr uint32_t kNavType_A970EC1F = 0xA970EC1Fu;  // NOTE: name unknown
} // namespace

// @func 0x00758248 TargetableCBase::~TargetableCBase
// @func 0x007582D0 TargetableCBase::~TargetableCBase (deleting)
// @func 0x00758550 TargetableCBase::~TargetableCBase (thunk, +0x17C)
// @func 0x0075841C TargetableCBase::~TargetableCBase (deleting thunk, +0x17C)
// @func 0x007584C4 TargetableCBase::~TargetableCBase (thunk, +0x180)
// @func 0x00758374 TargetableCBase::~TargetableCBase (deleting thunk, +0x180)
TargetableCBase::~TargetableCBase()
{
    ReleaseNavObstacle(navObstacle);
}

// @func 0x007586A8 TargetableCVariant::~TargetableCVariant
// @func 0x00758730 TargetableCVariant::~TargetableCVariant (deleting)
// @func 0x007589B0 TargetableCVariant::~TargetableCVariant (thunk, +0x17C)
// @func 0x0075887C TargetableCVariant::~TargetableCVariant (deleting thunk, +0x17C)
// @func 0x00758924 TargetableCVariant::~TargetableCVariant (thunk, +0x180)
// @func 0x007587D4 TargetableCVariant::~TargetableCVariant (deleting thunk, +0x180)
TargetableCVariant::~TargetableCVariant() = default;

// @func 0x00758B88 TargetableCLinked::~TargetableCLinked
// @func 0x00758C10 TargetableCLinked::~TargetableCLinked (deleting)
// @func 0x00758E90 TargetableCLinked::~TargetableCLinked (thunk, +0x17C)
// @func 0x00758D5C TargetableCLinked::~TargetableCLinked (deleting thunk, +0x17C)
// @func 0x00758E04 TargetableCLinked::~TargetableCLinked (thunk, +0x180)
// @func 0x00758CB4 TargetableCLinked::~TargetableCLinked (deleting thunk, +0x180)
TargetableCLinked::~TargetableCLinked() = default;

// @func 0x0075811C TargetableDefC::DefSlot6
uint32_t TargetableDefC::DefSlot6() const
{
    switch (navType) {
    case 0x9260C26Eu:   // "Static"
    case 0x41EB0AA3u:
    case 0x29846DDAu:
    case 0x4438A28Fu:   // "Obstacle"
        return 1;
    default:
        return 0;
    }
}

// @func 0x0075862C TargetableCBase::GetRenderObject
ape::world::ApeObj* TargetableCBase::GetRenderObject() { return BodyAsObject(rigidBody); }

// @func 0x007585DC TargetableCBase::Slot41
int TargetableCBase::Slot41() { return Def()->navType != kNavType_None ? 1 : 0; }

// @func 0x00758604 TargetableCBase::Slot42
int TargetableCBase::Slot42() { return Def()->navType == kNavType_A970EC1F ? 1 : 0; }

// @func 0x00758634 TargetableCBase::Slot95
void TargetableCBase::Slot95() {}

// @func 0x00758640 TargetableCBase::Interface17CSlot6
// @func 0x00758644 TargetableCBase::Interface17CSlot6 (thunk, +0x17C)
void TargetableCBase::Interface17CSlot6() {}

// @func 0x00758638 TargetableCBase::Interface17CSlot7
// @func 0x0075863C TargetableCBase::Interface17CSlot7 (thunk, +0x17C)
void TargetableCBase::Interface17CSlot7() {}

// @func 0x00758648 TargetableCBase::Interface180Slot6
// @func 0x00758678 TargetableCBase::Interface180Slot6 (thunk, +0x180)
void TargetableCBase::Interface180Slot6(void* body)
{
    RigidBodyRemoveContactListener(body, static_cast<EntityInterface17C*>(this));
}

} // namespace entity
} // namespace sly
