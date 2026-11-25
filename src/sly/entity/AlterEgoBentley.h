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
    // @func 0x0074F0DC AlterEgoBentleyState::Slot4
    virtual void Slot4() {}
    // Slots 5/6: names of the state (empty in every class).
    // @func 0x0074F0E0 AlterEgoBentleyState::Name
    virtual const char* Name() const { return ""; }
    // @func 0x0074F0EC AlterEgoBentleyState::Name2
    virtual const char* Name2() const { return ""; }
};

// Plain states (vtables 0x00B14610, 0x00B14640, 0x00B14670, 0x00B146A0).
class AlterEgoBentleyStateA : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateA() override;
    // @func 0x0074F0F8 AlterEgoBentleyStateA::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F104 AlterEgoBentleyStateA::Name2
    const char* Name2() const override { return ""; }
};
class AlterEgoBentleyStateB : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateB() override;
    // @func 0x0074F110 AlterEgoBentleyStateB::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F11C AlterEgoBentleyStateB::Name2
    const char* Name2() const override { return ""; }
};
class AlterEgoBentleyStateC : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateC() override;
    // @func 0x0074F128 AlterEgoBentleyStateC::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F134 AlterEgoBentleyStateC::Name2
    const char* Name2() const override { return ""; }
    void Slot4() override;
};
class AlterEgoBentleyStateD : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateD() override;
    // @func 0x0074F140 AlterEgoBentleyStateD::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F14C AlterEgoBentleyStateD::Name2
    const char* Name2() const override { return ""; }
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
    // @func 0x0074F158 AlterEgoBentleyScriptStateA::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F164 AlterEgoBentleyScriptStateA::Name2
    const char* Name2() const override { return ""; }
};
class AlterEgoBentleyScriptStateB : public AlterEgoBentleyScriptState {
public:
    ~AlterEgoBentleyScriptStateB() override;
    // @func 0x0074F170 AlterEgoBentleyScriptStateB::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F17C AlterEgoBentleyScriptStateB::Name2
    const char* Name2() const override { return ""; }
    void Slot4() override;
};

} // namespace entity
} // namespace sly
