// openTIT - Control components built by code (factory types 5, 7-12): the
// player's pad controller, a vehicle controller, a radar controller and the
// three scripted controllers that drive a character through a state
// machine.
#pragma once

#include "sly/components/Components.h"
#include "sly/gameplay/StateMachine.h"

namespace sly {
namespace components {

// Description of the code-built controllers that take a value (+0x14,
// 0x18 bytes). The modules that build one on the stack each carry their own
// copy of its destructor (character 0x00B1431C, vehicles 0x00B1B990 and
// 0x00B1BBF0).
struct ControlComponentDesc : entity::ComponentDesc {
    ~ControlComponentDesc() override;
    uint32_t value = 0;              // +0x14 NOTE: the pad / player index for the pad controllers
};

// A description built by code without parameters of its own (0x14 bytes;
// character 0x00B1433C, vehicles 0x00B1B9A0).
struct CodeComponentDesc : entity::ComponentDesc {
    ~CodeComponentDesc() override;
};

// Action name -> pad button bindings of a controller.
struct ActionBinding {
    uint32_t action;                 // name hash ("Jump", "Shoot", ...)
    uint32_t button;                 // pad button mask
};

// Type 5 (vtable 0x00B1C168, 0x60 bytes): the pad of a local player.
// NOTE: read as "ControlPlayer" from the element table and its 31 actions.
class ControlPlayerComponent : public ControlComponent {
public:
    explicit ControlPlayerComponent(const ControlComponentDesc* desc)
        : ControlComponent(desc), pad(desc->value) {}
    ~ControlPlayerComponent() override;
    // TODO(0x0027C478 ControlPlayerComponent::Update): reads the pad sticks
    // and turns them into camera space (vector code with bad instruction
    // data in the disassembly).
    uint32_t ButtonForAction(uint32_t action) const override;

    uint32_t pad = 0;                // 0x50
};

// Type 7 (vtable 0x00B1C208, 0x50 bytes): a controller with no input source
// of its own (input written by others).
class ControlPassiveComponent : public ControlComponent {
public:
    explicit ControlPassiveComponent(const entity::ComponentDesc* desc) : ControlComponent(desc) {}
    ~ControlPassiveComponent() override;
};

// Type 9 (vtable 0x00B1C2D8, 0x60 bytes): the pad driving a vehicle.
class ControlVehicleComponent : public ControlComponent {
public:
    explicit ControlVehicleComponent(const ControlComponentDesc* desc)
        : ControlComponent(desc), pad(desc->value) {}
    ~ControlVehicleComponent() override;
    // TODO(0x0027E508 ControlVehicleComponent::Update)
    uint32_t ButtonForAction(uint32_t action) const override;

    uint32_t pad = 0;                // 0x50
};

// Type 11 (vtable 0x00B1C3A8, 0x60 bytes): a pad controller whose only
// action is the radar.
class ControlRadarComponent : public ControlComponent {
public:
    explicit ControlRadarComponent(const ControlComponentDesc* desc)
        : ControlComponent(desc), pad(desc->value) {}
    ~ControlRadarComponent() override;
    // TODO(0x0027FCB8 ControlRadarComponent::Update)
    uint32_t ButtonForAction(uint32_t action) const override;

    uint32_t pad = 0;                // 0x50
};

// Base of the scripted controllers: a control component with a state
// machine (+0x50; a second base class on the console) whose messages go to
// slot 17.
class ControlScriptedComponent : public ControlComponent {
public:
    ControlScriptedComponent(const entity::ComponentDesc* desc, uint16_t layers, uint32_t dataSize)
        : ControlComponent(desc), machine(this, layers, dataSize) {}
    using Message = gameplay::StateMachine::Message;
    // Slot 17 (+0x44): a message of the state machine.
    virtual bool OnStateMessage(const Message& message) { (void)message; return true; }

    // The state machine part: forwards every message to the component.
    class Machine : public gameplay::StateMachine {
    public:
        Machine(ControlScriptedComponent* owner, uint16_t layers, uint32_t dataSize)
            : gameplay::StateMachine(layers, dataSize, 0), component(owner) {}
        bool OnMessage(const Message& message) override { return component->OnStateMessage(message); }
        ControlScriptedComponent* component;
    };
    Machine machine;                 // 0x50

protected:
    // The state a message is about: the new state of an enter message of
    // layer 0, else layer 0's current state. Update messages clear the input
    // first; they stop when the owner has no health left.
    int MessageState(const Message& message, bool* ownerDead);
};

// Type 8 (vtable 0x00B1C258, state machine 0x00B1C2A8, 0x70 bytes; two
// layers of 0x30 bytes of state data).
class ControlScriptedA : public ControlScriptedComponent {
public:
    explicit ControlScriptedA(const entity::ComponentDesc* desc) : ControlScriptedComponent(desc, 2, 0x30) {}
    ~ControlScriptedA() override;
    // @func 0x0076380C ControlScriptedA::Update
    void Update(float dt) override { machine.Update(dt); }
    bool OnStateMessage(const Message& message) override;

private:
    // TODO(0x0027DEA8 / 0x0027DB68 / 0x0027D6E8 / 0x0027D340): the handlers
    // of states 1-4 (scripted movement, vector code).
    bool State1(const Message& message) { (void)message; return true; }
    bool State2(const Message& message) { (void)message; return true; }
    bool State3(const Message& message) { (void)message; return true; }
    bool State4(const Message& message) { (void)message; return true; }
};

// Type 10 (vtable 0x00B1C328, state machine 0x00B1C378, 0x70 bytes; two
// layers of 0x20 bytes of state data).
class ControlScriptedB : public ControlScriptedComponent {
public:
    explicit ControlScriptedB(const entity::ComponentDesc* desc) : ControlScriptedComponent(desc, 2, 0x20) {}
    ~ControlScriptedB() override;
    // @func 0x00763A6C ControlScriptedB::Update
    void Update(float dt) override { machine.Update(dt); }
    bool OnStateMessage(const Message& message) override;

private:
    // TODO(0x0027F630 / 0x0027F2C0 / 0x0027EE70): the handlers of states 1-3.
    bool State1(const Message& message) { (void)message; return true; }
    bool State2(const Message& message) { (void)message; return true; }
    bool State3(const Message& message) { (void)message; return true; }
};

// Type 12 (vtable 0x00B1C3F8, state machine 0x00B1C444, no layers): updates
// like a plain controller; its state machine ignores every message.
class ControlScriptedC : public ControlScriptedComponent {
public:
    explicit ControlScriptedC(const entity::ComponentDesc* desc) : ControlScriptedComponent(desc, 0, 0) {}
    ~ControlScriptedC() override;
    // Slot 17 is the shared "true" stub (0x007435B8).
    bool OnStateMessage(const Message&) override { return true; }
};

} // namespace components
} // namespace sly
