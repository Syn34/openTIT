// openTIT - States of the Bentley alter ego entity (constructed by
// 0x0016BF00, definition AlterEgoBentleyDef).
//
// The entity embeds one object per state; each is a small polymorphic class
// (base vtable 0x00B145E0) with enter/update callbacks. Two states drive an
// anim script instance they own. The original class names are not known.
#pragma once

#include <cstdint>

namespace sly {
namespace entity {

class AlterEgoBentleyState {
public:
    virtual ~AlterEgoBentleyState();
    // NOTE: slot names from their position; the base versions do nothing.
    virtual void Enter(void* owner);
    virtual void Update(void* owner, float dt);
    virtual void Slot4() {}
    virtual const char* Name() const { return ""; }
    virtual const char* Name2() const { return ""; }
};

// Plain states (vtables 0x00B14610, 0x00B14640, 0x00B14670, 0x00B146A0).
class AlterEgoBentleyStateA : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateA() override;
};
class AlterEgoBentleyStateB : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateB() override;
};
class AlterEgoBentleyStateC : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateC() override;
    void Slot4() override;
};
class AlterEgoBentleyStateD : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateD() override;
};

// States that run an anim script (vtables 0x00B146D0, 0x00B14700).
class AlterEgoBentleyScriptState : public AlterEgoBentleyState {
public:
    // Orphans and releases the script instance.
    void ReleaseScript();
    void* script = nullptr;    // +0x04 ApeAnimScriptInstance
};
class AlterEgoBentleyScriptStateA : public AlterEgoBentleyScriptState {
public:
    ~AlterEgoBentleyScriptStateA() override;
};
class AlterEgoBentleyScriptStateB : public AlterEgoBentleyScriptState {
public:
    ~AlterEgoBentleyScriptStateB() override;
    void Slot4() override;
};

} // namespace entity
} // namespace sly
