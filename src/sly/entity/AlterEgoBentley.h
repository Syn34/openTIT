// openTIT - States of the Bentley alter ego entity (constructed by
// 0x0016BF00, definition AlterEgoBentleyDef).
//
// The entity embeds one object per state; each is a small polymorphic class
// (base vtable 0x00B145E0) with enter/update callbacks. Two states drive an
// anim script instance they own. The original class names are not known.
#pragma once

#include "sly/entity/ScriptedEntity.h"

#include <cstdint>

namespace sly {
namespace entity {

// The Bentley alter ego entity the states act on (built by 0x0016BF00; the
// class is not modelled beyond what the states use).
class AlterEgoBentleyEntity : public ScriptedEntity {
public:
    // Plays a power-up effect on the character component unless the entity
    // is dead with its death effect showing; remembers the effect.
    void PlayPowerUpFx(uint32_t fx);

    uint32_t buffFlags = 0;          // 0x290 bit 25: a timed buff is running
    int32_t buffTicks = 0;           // 0x29C
    uint32_t scriptLevel = 0;        // 0x2A8
    float speedScale = 1.0f;         // 0x2AC
    float buffRate = 0.0f;           // 0x2B0
    uint32_t lastFx = 0;             // 0x2B4 effect last played (0x03DE9A54 = the death effect)
    uint32_t skinLevel = 0;          // 0x2D0
    uint32_t skinCount = 0;          // 0x2D8
};

class AlterEgoBentleyState {
public:
    virtual ~AlterEgoBentleyState();
    // Slots 2/3: the state is applied to and taken off the owner (the
    // derived states save what they change on Enter and restore it on Exit).
    // The base versions do nothing.
    virtual void Enter(void* owner);
    virtual void Update(void* owner, float dt);
    // @func 0x0074F0DC AlterEgoBentleyState::Slot4
    virtual void Exit(void* owner);
    virtual void Slot4() {}
    // Slots 5/6: names of the state (empty in every class).
    // @func 0x0074F0E0 AlterEgoBentleyState::Name
    virtual const char* Name() const { return ""; }
    // @func 0x0074F0EC AlterEgoBentleyState::Name2
    virtual const char* Name2() const { return ""; }
};

// Plain states (vtables 0x00B14610, 0x00B14640, 0x00B14670, 0x00B146A0).
// Skin speed boost: the skin component's rate shrinks with the boosts taken.
class AlterEgoBentleyStateA : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateA() override;
    // @func 0x0074F0F8 AlterEgoBentleyStateA::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F104 AlterEgoBentleyStateA::Name2
    const char* Name2() const override { return ""; }
    void Enter(void* owner) override;
    void Exit(void* owner) override;
    float savedRate = 0.0f;      // +0x04
    uint32_t savedCount = 0;     // +0x08
};
// Speed boost: the owner moves 25% faster and its script counts the boost.
class AlterEgoBentleyStateB : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateB() override;
    // @func 0x0074F110 AlterEgoBentleyStateB::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F11C AlterEgoBentleyStateB::Name2
    const char* Name2() const override { return ""; }
    void Enter(void* owner) override;
    void Exit(void* owner) override;
    int32_t savedLevel = 0;      // +0x04 script variable before the boost
    float savedSpeed = 0.0f;     // +0x08
};
// Second skin component's rate boost.
class AlterEgoBentleyStateC : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateC() override;
    // @func 0x0074F128 AlterEgoBentleyStateC::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F134 AlterEgoBentleyStateC::Name2
    const char* Name2() const override { return ""; }
    void Enter(void* owner) override;
    void Exit(void* owner) override;
    void Slot4() override;
    uint32_t savedLevel = 0;     // +0x04
};
// Skin level boost (+3).
class AlterEgoBentleyStateD : public AlterEgoBentleyState {
public:
    ~AlterEgoBentleyStateD() override;
    // @func 0x0074F140 AlterEgoBentleyStateD::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F14C AlterEgoBentleyStateD::Name2
    const char* Name2() const override { return ""; }
    void Enter(void* owner) override;
    void Exit(void* owner) override;
    int32_t savedLevel = 0;      // +0x04
};

// States that run an anim script (vtables 0x00B146D0, 0x00B14700).
class AlterEgoBentleyScriptState : public AlterEgoBentleyState {
public:
    // Orphans and releases the script instance.
    void ReleaseScript();
    void* script = nullptr;    // +0x04 ApeAnimScriptInstance
};
// Starts its script with the owner as "this"; later visits strengthen it.
class AlterEgoBentleyScriptStateA : public AlterEgoBentleyScriptState {
public:
    ~AlterEgoBentleyScriptStateA() override;
    // @func 0x0074F158 AlterEgoBentleyScriptStateA::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F164 AlterEgoBentleyScriptStateA::Name2
    const char* Name2() const override { return ""; }
    void Enter(void* owner) override;
    void Exit(void* owner) override;
    uint32_t savedLevel = 0;     // +0x08
    float strength = 0.0f;       // +0x0C
    float period = 0.0f;         // +0x10
    float time = 0.0f;           // +0x14
    float field_18 = 0.0f;       // +0x18 -1 on odd levels
};
// Timed buff counted down by Exit.
class AlterEgoBentleyScriptStateB : public AlterEgoBentleyScriptState {
public:
    ~AlterEgoBentleyScriptStateB() override;
    // @func 0x0074F170 AlterEgoBentleyScriptStateB::Name
    const char* Name() const override { return ""; }
    // @func 0x0074F17C AlterEgoBentleyScriptStateB::Name2
    const char* Name2() const override { return ""; }
    void Enter(void* owner) override;
    void Exit(void* owner) override;
    void Slot4() override;
    uint32_t count = 0;          // +0x08
};

} // namespace entity
} // namespace sly
