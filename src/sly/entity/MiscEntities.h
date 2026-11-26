// openTIT - Entity classes known only by their vtables whose small virtual
// overrides and destructors sit in 0x00758FF0-0x007594C4. Names are
// descriptive (vtable based); their constructors and updates are not
// ported.
#pragma once

#include "sly/entity/PropEntities.h"

namespace sly {
namespace entity {

// A scripted entity with one rigid body built from its definition's shape
// (vtable 0x00B19198 + 0x00B1932C/0x00B19354; constructor 0x002040A0,
// definition vtable 0x00B19148 with bounds 0x00203E70 and spawn 0x00204660).
class RigidEntityB19198 : public RigidBodyEntity {
public:
    ~RigidEntityB19198() override;
    // Slot 36: the rigid body (see NOTES on slot 36).
    ape::world::ApeObj* GetRenderObject() override;
    // Slot 95: empty. NOTE: no caller known.
    virtual void Slot95();
    // Empty overrides of the +0x17C interface's slots 6/7 (primary slots 97/96).
    void Interface17CSlot6() override;
    void Interface17CSlot7() override;
    // Primary slot 98 / +0x180 slot 6.
    void Interface180Slot6(void* body) override;
};

// Entity spawned by EntityDefB19378 (vtable 0x00B193C8, 0x1C0 bytes; plain
// entity base) that runs an anim script of its own.
class ScriptedObjectB193C8 : public Entity {
public:
    ~ScriptedObjectB193C8() override;
    // @func 0x007594C0 ScriptedObjectB193C8::GetAnimScript
    void* GetAnimScript() override { return animScript; }   // slot 68

    void* animScript = nullptr;   // 0x1AC
};

// Entity spawned by EntityDefB14FD8 (vtable 0x00B15028, constructor
// 0x001A4680; plain entity base) with one array (+0x158).
class EntityB15028 : public Entity {
public:
    ~EntityB15028() override;
    ape::ApeTArray<void*> field_158;   // +0x158 NOTE: element type unknown
};

} // namespace entity
} // namespace sly
