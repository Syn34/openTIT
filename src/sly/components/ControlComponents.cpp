// openTIT - Control components built by code (0x0027C290-0x00280628).
#include "sly/components/ControlComponents.h"

#include "sly/entity/ScriptedEntity.h"

namespace sly {
namespace components {

namespace {

// Pad buttons (the masks of the button table 0x00AD429C).
enum PadButton : uint32_t {
    kPad_Up = 0x0001, kPad_Down = 0x0002, kPad_Left = 0x0004, kPad_Right = 0x0008,
    kPad_Triangle = 0x0010, kPad_Square = 0x0020, kPad_Circle = 0x0040, kPad_Cross = 0x0080,
    kPad_L1 = 0x0100, kPad_R1 = 0x0200, kPad_L2 = 0x0400, kPad_R2 = 0x0800,
    kPad_L3 = 0x1000, kPad_R3 = 0x2000,
};

// Player character actions (table 0x00F64A78).
constexpr ActionBinding kPlayerActions[] = {
    {0x0022EC2Eu, kPad_Cross},     // "Jump"
    {0x1AF3AD1Au, kPad_Circle},    // "ClimbAttach"
    {0x1F3ED668u, kPad_Triangle},  // "ClimbDetach"
    {0x922FF61Au, kPad_R1},        // "Sprint"
    {0x898CCF3Fu, kPad_Square},    // "AttackPri"
    {0x898CD8E9u, kPad_Triangle},  // "AttackSec"
    {0x898C9641u, kPad_Circle},    // "AttackAlt"
    {0xDAD5A4F7u, kPad_Circle},    // "Pickpocket"
    {0x57F4FA42u, kPad_Circle},    // "InteractEnter"
    {0xD98C2C54u, kPad_Circle},    // "InteractExit"
    {0xB260869Du, kPad_R3},        // "BinocEnter"
    {0x81A02899u, kPad_R3},        // "BinocExit"
    {0xB32303E4u, kPad_R1},        // "BinocShoot"
    {0xB694FD32u, kPad_R2},        // "SuperAbil1"
    {0xB694FD33u, kPad_R2},        // "SuperAbil2"
    {0x3F2950FFu, kPad_L2},        // "Disguise"
    {0x040C2B85u, kPad_R1},        // "Glide"
    {0x0000FD25u, kPad_L1},        // "Aim"
    {0x04B38BFFu, kPad_R1},        // "Shoot"
    {0x001F32A2u, kPad_Triangle},  // "Bomb"
    {0x00000A9Bu, kPad_Up},        // "Up"
    {0x00201CA2u, kPad_Down},      // "Down"
    {0x00239807u, kPad_Left},      // "Left"
    {0x04A5C9FCu, kPad_Right},     // "Right"
    {0x375599D9u, kPad_L3},        // "RadarActivate"
    {0xF91ADF94u, kPad_R2},        // "CrackShot"
    {0x0026772Eu, kPad_R2},        // "Roar"
    {0xBC885EFCu, kPad_R2},        // "BallRoll"
    {0xEE1B444Eu, kPad_R2},        // "SlowTime"
    {0x924B3E09u, kPad_R2},        // "SuperJump"
    {0x6DB2AC20u, kPad_R2},        // "Detonate"
};

// Vehicle actions (table 0x00AD46A8).
constexpr ActionBinding kVehicleActions[] = {
    {0x13A2F438u, kPad_Triangle},  // "Reorient"
    {0xCC2E66DDu, kPad_Cross},     // "Accelerate"
    {0xAD399676u, kPad_Square},    // "BrakeAndReverse"
    {0xA96387FCu, kPad_R2},        // "Handbrake"
    {0x5F69EA3Eu, kPad_Left},      // "CycleControlLeft"
    {0x8E29BEA5u, kPad_Right},     // "CycleControlRight"
    {0x04B38BFFu, kPad_R1},        // "Shoot"
    {0x3F2950FFu, kPad_L2},        // "Disguise"
    {0x04C795FEu, kPad_L1},        // "Turbo"
    {0x0021ED83u, kPad_Circle},    // "Horn"
    {0x0021ED83u, kPad_L3},        // "Radar" (the hash repeats in the table)
};

// Radar controller actions (table 0x00AD481C).
constexpr ActionBinding kRadarActions[] = {
    {0x0021ED83u, kPad_L3},        // "Radar"
};

template <size_t N>
uint32_t FindButton(const ActionBinding (&table)[N], uint32_t action)
{
    for (const ActionBinding& binding : table) {
        if (binding.action == action)
            return binding.button;
    }
    return 0;
}

} // namespace

// @func 0x0027C290 ControlComponent::Update
void ControlComponent::Update(float dt)
{
    (void)dt;
    constexpr int kActionCount = 31;
    for (int i = 0; i < kActionCount; ++i) {
        if (!(pressed & (1u << i)))
            continue;
        uint32_t& word = input.actions[i >> 4];
        const int shift = (i & 15) * 2;
        // Bit 0: held; bit 1: went down this frame (only if it was not held).
        const uint32_t state = (word & (1u << shift)) ? 1u : 3u;
        word = (word & ~(3u << shift)) | (state << shift);
    }
    pressed = 0;
}

// @func 0x0074EC88 ControlComponentDesc::~ControlComponentDesc (copy, character module)
// @func 0x0074ECFC ControlComponentDesc::~ControlComponentDesc (deleting, copy, character module)
// @func 0x0075D6E8 ControlComponentDesc::~ControlComponentDesc (copy, vehicle module)
// @func 0x0075D974 ControlComponentDesc::~ControlComponentDesc (deleting, copy, vehicle module)
// @func 0x0075DA44 ControlComponentDesc::~ControlComponentDesc (copy 2, vehicle module)
// @func 0x0075DB28 ControlComponentDesc::~ControlComponentDesc (deleting, copy 2, vehicle module)
ControlComponentDesc::~ControlComponentDesc() = default;

// @func 0x0074EBBC CodeComponentDesc::~CodeComponentDesc (copy, character module)
// @func 0x0074EDCC CodeComponentDesc::~CodeComponentDesc (deleting, copy, character module)
// @func 0x0075D6A4 CodeComponentDesc::~CodeComponentDesc (copy, vehicle module)
// @func 0x0075D9DC CodeComponentDesc::~CodeComponentDesc (deleting, copy, vehicle module)
CodeComponentDesc::~CodeComponentDesc() = default;

// -- pad controllers ---------------------------------------------------------------

// The destructors only run the base destructor.
// @func 0x007633D4 ControlPlayerComponent::~ControlPlayerComponent
// @func 0x007634D0 ControlPlayerComponent::~ControlPlayerComponent (deleting)
ControlPlayerComponent::~ControlPlayerComponent() = default;
// @func 0x007635F0 ControlPassiveComponent::~ControlPassiveComponent
// @func 0x007636EC ControlPassiveComponent::~ControlPassiveComponent (deleting)
ControlPassiveComponent::~ControlPassiveComponent() = default;
// @func 0x00763850 ControlVehicleComponent::~ControlVehicleComponent
// @func 0x0076394C ControlVehicleComponent::~ControlVehicleComponent (deleting)
ControlVehicleComponent::~ControlVehicleComponent() = default;
// @func 0x00763AB0 ControlRadarComponent::~ControlRadarComponent
// @func 0x00763BAC ControlRadarComponent::~ControlRadarComponent (deleting)
ControlRadarComponent::~ControlRadarComponent() = default;

// @func 0x0027C9A0 ControlPlayerComponent::ButtonForAction
uint32_t ControlPlayerComponent::ButtonForAction(uint32_t action) const { return FindButton(kPlayerActions, action); }

// @func 0x0027E8F4 ControlVehicleComponent::ButtonForAction
uint32_t ControlVehicleComponent::ButtonForAction(uint32_t action) const { return FindButton(kVehicleActions, action); }

// @func 0x002800A4 ControlRadarComponent::ButtonForAction
uint32_t ControlRadarComponent::ButtonForAction(uint32_t action) const { return FindButton(kRadarActions, action); }

// -- scripted controllers ----------------------------------------------------------

int ControlScriptedComponent::MessageState(const Message& message, bool* ownerDead)
{
    int state = machine.GetState(0);
    if (message.type == gameplay::StateMachine::kMsg_Enter && message.layer == 0)
        state = message.newState;
    *ownerDead = false;
    if (message.type == gameplay::StateMachine::kMsg_Update) {
        input.stick = {};
        input.move = {};
        input.magnitude = 0.0f;
        input.actions[0] = input.actions[1] = 0;
        input.pad = 0;
        // NOTE: the owner is read as a scripted entity (health at +0x15C).
        const auto* scripted = static_cast<const entity::ScriptedEntity*>(owner);
        *ownerDead = scripted->health <= 0.0f;
    }
    return state;
}

// @func 0x0027CE34 ControlScriptedA::~ControlScriptedA
// @func 0x0027CF60 ControlScriptedA::~ControlScriptedA (deleting)
// @func 0x0027D200 ControlScriptedA::~ControlScriptedA (state machine thunk)
// @func 0x0027D0A8 ControlScriptedA::~ControlScriptedA (state machine thunk, deleting)
ControlScriptedA::~ControlScriptedA() = default;

// @func 0x0027E330 ControlScriptedA::OnStateMessage
// @func 0x0027E480 ControlScriptedA::OnStateMessage (state machine thunk)
bool ControlScriptedA::OnStateMessage(const Message& message)
{
    bool ownerDead = false;
    const int state = MessageState(message, &ownerDead);
    if (ownerDead)
        return true;
    switch (state) {
    case 4: return State4(message);
    case 3: return State3(message);
    case 2: return State2(message);
    case 1: return State1(message);
    default: return true;
    }
}

// @func 0x0027E968 ControlScriptedB::~ControlScriptedB
// @func 0x0027EA94 ControlScriptedB::~ControlScriptedB (deleting)
// @func 0x0027ED34 ControlScriptedB::~ControlScriptedB (state machine thunk)
// @func 0x0027EBDC ControlScriptedB::~ControlScriptedB (state machine thunk, deleting)
ControlScriptedB::~ControlScriptedB() = default;

// @func 0x0027FAF0 ControlScriptedB::OnStateMessage
// @func 0x0027FC30 ControlScriptedB::OnStateMessage (state machine thunk)
bool ControlScriptedB::OnStateMessage(const Message& message)
{
    bool ownerDead = false;
    const int state = MessageState(message, &ownerDead);
    if (ownerDead)
        return true;
    switch (state) {
    case 1: return State1(message);
    case 2: return State2(message);
    case 3: return State3(message);
    default: return true;
    }
}

// @func 0x00280118 ControlScriptedC::~ControlScriptedC
// @func 0x00280244 ControlScriptedC::~ControlScriptedC (deleting)
// @func 0x002804E4 ControlScriptedC::~ControlScriptedC (state machine thunk)
// @func 0x0028038C ControlScriptedC::~ControlScriptedC (state machine thunk, deleting)
ControlScriptedC::~ControlScriptedC() = default;

} // namespace components
} // namespace sly
