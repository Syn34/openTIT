// openTIT - Entity classes known only by their vtables whose small virtual
// overrides and destructors sit in 0x00758FF0-0x007594C4. Names are
// descriptive (vtable based); their constructors and updates are not
// ported.
#pragma once

#include "sly/entity/PropEntities.h"

namespace ape { class ApeRefCounted; }

namespace sly {
namespace entity {

// A scripted entity with one rigid body built from its definition's shape
// (vtable 0x00B19198 + 0x00B1932C/0x00B19354; constructor 0x002040A0,
// definition vtable 0x00B19148 with bounds 0x00203E70 and spawn 0x00204660).
class RigidEntityB19198 : public RigidBodyEntity {
public:
    ~RigidEntityB19198() override;
    // Slots 25/26: collision group of the body (kept in `groupBits` while
    // the body is out of the world).
    uint32_t GetRenderId() override;
    void SetRenderId(uint32_t id) override;
    // Slots 27/28: gravity factor of the body (the attach link's while attached).
    float GetGravityFactor() override;
    void Detached(float gravityFactor) override;
    // Slot 35: linear velocity of the body.
    ape::Vec3 GetVelocity() override;
    // Slot 37: the entity's world object answers for its own body.
    ape::world::ApeObj* ObjectForCollidable(const void* collidable, int32_t* bone) override;
    // Slot 36: the rigid body (see NOTES on slot 36).
    ape::world::ApeObj* GetRenderObject() override;
    // Slot 76: interaction kinds 0 (not with an entity without team bits)
    // and 4 only, then the base checks.
    bool CanInteractWith(Entity* other, int kind, float* priority = nullptr) override;
    // Slot 88: damage only while there is health left; true while some is.
    bool Damage(float amount, Entity* source, uint32_t type) override;
    // Slot 95: empty. NOTE: no caller known.
    virtual void Slot95();
    // Empty overrides of the +0x17C interface's slots 6/7 (primary slots 97/96).
    void Interface17CSlot6() override;
    void Interface17CSlot7() override;
    // Primary slot 98 / +0x180 slot 6.
    void Interface180Slot6(void* body) override;

    uint32_t groupBits = 0;   // +0x190 bits 0-23: collision group
};

// Entity spawned by EntityDefB19378 (vtable 0x00B193C8, 0x1C0 bytes; plain
// entity base) that runs an anim script of its own.
class ScriptedObjectB193C8 : public Entity {
public:
    ~ScriptedObjectB193C8() override;
    // Slot 10: with `kill` the entity leaves the per-frame lists, its script
    // finishes its "End" state and its helper objects go.
    bool Release(bool kill) override;
    // Slot 32: the script is told the new time scale first.
    void SetTimeScale(uint32_t value, bool children) override;
    // Slots 60/61: the script pauses (state 0xE69B8987) and resumes (0xD5E4206F).
    void OnSuspend() override;
    void OnResume() override;
    // @func 0x007594C0 ScriptedObjectB193C8::GetAnimScript
    void* GetAnimScript() override { return animScript; }   // slot 68

    // NOTE: types of the helper objects unknown; +0x1A0 is released, +0x1B0
    // deleted with the entity.
    ape::ApeRefCounted* field_1A0 = nullptr;   // 0x1A0
    void* animScript = nullptr;   // 0x1AC
    ape::ApeRefCounted* field_1B0 = nullptr;   // 0x1B0
};

// Entity spawned by EntityDefB14FD8 (vtable 0x00B15028, constructor
// 0x001A4680; plain entity base) with one array (+0x158).
class EntityB15028 : public Entity {
public:
    ~EntityB15028() override;
    // Slot 10: a kill drops the two referenced objects first.
    bool Release(bool kill) override;
    ape::ApeTArray<void*> field_158;   // +0x158 NOTE: element type unknown
    ape::world::ApeObj* object_170 = nullptr;   // +0x170 counted reference
    ape::world::ApeObj* object_174 = nullptr;   // +0x174 counted reference
};

} // namespace entity
} // namespace sly
