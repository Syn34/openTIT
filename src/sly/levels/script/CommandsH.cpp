// openTIT - Game script commands, sixth block (GiveAward and the HUD commands).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"
#include "sly/trophy/TrophyManager.h"

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrAwardNum = AnimHash("AwardNum");
constexpr uint32_t kAttrAwardName = AnimHash("AwardName");
constexpr uint32_t kAttrVehicleEntity = AnimHash("VehicleEntity");
constexpr uint32_t kAttrVehicleObject = AnimHash("VehicleObject");
constexpr uint32_t kAttrRefEntity = AnimHash("RefEntity");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrVehicle1C = 0xB61AB1C6u;  // NOTE: name unknown
constexpr uint32_t kAttrInputDir = AnimHash("InputDir");
constexpr uint32_t kAttrVehicle20 = 0x8A7E47A4u;  // NOTE: name unknown
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrHealthMeter = AnimHash("HealthMeter");
constexpr uint32_t kAttrPlayerNum = AnimHash("PlayerNum");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrHudObj = AnimHash("HudObj");
constexpr uint32_t kAttrAnimState = AnimHash("AnimState");
constexpr uint32_t kAttrAnim = AnimHash("Anim");
constexpr uint32_t kAttrLoop = AnimHash("Loop");
constexpr uint32_t kAttrMsg = AnimHash("Msg");
constexpr uint32_t kAttrMsgVar = AnimHash("MsgVar");
constexpr uint32_t kAttrAnimMode = AnimHash("AnimMode");
constexpr uint32_t kAttrAnimRate = AnimHash("AnimRate");
constexpr uint32_t kAttrIndex = AnimHash("Index");
constexpr uint32_t kAttrState = AnimHash("state");

// Player number meaning "no player" (the HUD supports players 0..8).
constexpr uint8_t kNoPlayer = 9;

// HUD anim states that start the animation.
constexpr uint32_t kAnimState_Idle = AnimHash("Idle");
constexpr uint32_t kAnimState_048129EB = 0x048129EBu;  // NOTE: name unknown
constexpr int kHudAnimMode_Play = 3;
} // namespace

// ---------------------------------------------------------------------------
// "GiveAward" (hash 0xFD0361CC; NOTE: name from a hash match): unlocks a trophy.

class GiveAwardCommand : public ApeAnimScriptCommand {
public:
    GiveAwardCommand();
    ~GiveAwardCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_awardNum = -1;    // +0x0C "AwardNum"
    uint32_t m_awardName = 0;   // +0x10 "AwardName"
};

// @func 0x0007C714 GiveAwardCommand::GiveAwardCommand
GiveAwardCommand::GiveAwardCommand() = default;

// @func 0x0007C76C GiveAwardCommand::~GiveAwardCommand
// @func 0x0007C79C GiveAwardCommand::~GiveAwardCommand (deleting)
GiveAwardCommand::~GiveAwardCommand() = default;

// @func 0x0007C7F4 GiveAwardCommand::Create
ApeAnimScriptCommand* GiveAwardCommand::Create(XmlElement* element) const
{
    auto* command = new GiveAwardCommand();
    element->GetInt(kAttrAwardNum, &command->m_awardNum, -1);
    element->GetHash(kAttrAwardName, &command->m_awardName, command->m_awardName);
    return command;
}

// @func 0x0007C8C8 GiveAwardCommand::Execute
void GiveAwardCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    // The name takes precedence over the number.
    if (m_awardName) {
        AwardTrophyByName(m_awardName);
    } else if (m_awardNum != -1) {
        if (TrophyManager* trophies = TrophyManager::Instance())
            trophies->RequestUnlock(m_awardNum);
    }
}

// ---------------------------------------------------------------------------
// Hash 0x553CAE39 (name unknown): drives a vehicle with a scripted input.

class VehicleInputCommand : public ApeAnimScriptCommand {
public:
    VehicleInputCommand();
    ~VehicleInputCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_vehicleObject = 0;                     // +0x0C "VehicleObject"
    uint32_t m_vehicleEntity = 0;                     // +0x10 "VehicleEntity"
    uint32_t m_refObject = 0;                         // +0x14 "RefObject"
    uint32_t m_refEntity = 0;                         // +0x18 "RefEntity"
    uint32_t m_field1C = 0;                           // +0x1C attribute 0xB61AB1C6
    uint32_t m_field20 = 0;                           // +0x20 attribute 0x8A7E47A4
    ape::Vec3 m_inputDir = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x30 "InputDir"
};

// @func 0x0007C918 VehicleInputCommand::VehicleInputCommand
VehicleInputCommand::VehicleInputCommand() = default;

// @func 0x0007C998 VehicleInputCommand::~VehicleInputCommand
// @func 0x0007C9C8 VehicleInputCommand::~VehicleInputCommand (deleting)
VehicleInputCommand::~VehicleInputCommand() = default;

// @func 0x0007CA20 VehicleInputCommand::Create
ApeAnimScriptCommand* VehicleInputCommand::Create(XmlElement* element) const
{
    auto* command = new VehicleInputCommand();
    if (element->GetHash(kAttrVehicleEntity, &command->m_vehicleEntity, 0) == ape::xml::kQuery_Ok ||
        element->GetHash(kAttrVehicleObject, &command->m_vehicleObject, 0) == ape::xml::kQuery_Ok) {
        element->GetHash(kAttrRefEntity, &command->m_refEntity, command->m_refEntity);
        element->GetHash(kAttrRefObject, &command->m_refObject, command->m_refObject);
        element->GetHash(kAttrVehicle1C, &command->m_field1C, command->m_field1C);
        element->GetVec3(kAttrInputDir, &command->m_inputDir.x, &command->m_inputDir.x);
        element->GetHash(kAttrVehicle20, &command->m_field20, command->m_field20);
    }
    return command;
}

// TODO(0x0007CBD4 VehicleInputCommand::Execute)

// ---------------------------------------------------------------------------
// "HudHealthMeter" (hash 0xD22F2156; NOTE: name from a hash match)

class HudHealthMeterCommand : public ApeAnimScriptCommand {
public:
    HudHealthMeterCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_entity = 0;          // +0x0C "Entity"
    uint32_t m_healthMeter = 0;     // +0x10 "HealthMeter"
    uint8_t m_playerNum = kNoPlayer;// +0x14 "PlayerNum" (0..8)
    bool m_enable = true;           // +0x15 "enable"
};

// @func 0x0007CEFC HudHealthMeterCommand::HudHealthMeterCommand
HudHealthMeterCommand::HudHealthMeterCommand() = default;

// @func 0x0007CF54 HudHealthMeterCommand::Create
ApeAnimScriptCommand* HudHealthMeterCommand::Create(XmlElement* element) const
{
    auto* command = new HudHealthMeterCommand();
    element->GetHash(kAttrEntity, &command->m_entity, 0);
    element->GetHash(kAttrHealthMeter, &command->m_healthMeter, command->m_healthMeter);
    int32_t playerNum = kNoPlayer;
    element->GetInt(kAttrPlayerNum, &playerNum, kNoPlayer);
    command->m_playerNum = (playerNum >= 0 && playerNum < kNoPlayer) ? static_cast<uint8_t>(playerNum) : kNoPlayer;
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    return command;
}

// TODO(0x0007D098 HudHealthMeterCommand::Execute)

// ---------------------------------------------------------------------------
// Hash 0xE573697F (name unknown): plays an animation on a HUD object.

class HudPlayAnimCommand : public ApeAnimScriptCommand {
public:
    HudPlayAnimCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_hudObj = 0;     // +0x0C "HudObj"
    uint32_t m_animState = 0;  // +0x10 "AnimState"
    uint32_t m_anim = 0;       // +0x14 "Anim"
    bool m_loop = false;       // +0x18 "Loop"
};

// @func 0x0007D154 HudPlayAnimCommand::HudPlayAnimCommand
HudPlayAnimCommand::HudPlayAnimCommand() = default;

// @func 0x0007D1A4 HudPlayAnimCommand::Create
ApeAnimScriptCommand* HudPlayAnimCommand::Create(XmlElement* element) const
{
    auto* command = new HudPlayAnimCommand();
    element->GetHash(kAttrHudObj, &command->m_hudObj, 0);
    element->GetHash(kAttrAnimState, &command->m_animState, command->m_animState);
    element->GetHash(kAttrAnim, &command->m_anim, command->m_anim);
    element->GetBool(kAttrLoop, &command->m_loop, command->m_loop);
    return command;
}

// @func 0x0007D2B8 HudPlayAnimCommand::Execute
void HudPlayAnimCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    void* hud = FindHudObject(m_hudObj);
    if (!hud)
        return;
    if (m_animState == kAnimState_Idle || m_animState == kAnimState_048129EB)
        HudPlayAnim(hud, kHudAnimMode_Play, m_anim, m_loop);
}

// ---------------------------------------------------------------------------
// Hash 0x742543CF (name unknown): shows a message on a HUD object.

class HudMessageCommand : public ApeAnimScriptCommand {
public:
    HudMessageCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    char m_msg[0x20] = {};      // +0x0C "Msg"
    uint32_t m_msgVar = 0;      // +0x30 "MsgVar"
    uint32_t m_hudObj = 0;      // +0x34 "HudObj"
    uint32_t m_animMode = 0;    // +0x38 "AnimMode"
    float m_animRate = -1.0f;   // +0x3C "AnimRate"
    int32_t m_index = 0;        // +0x40 "Index"
};

// @func 0x0007D340 HudMessageCommand::HudMessageCommand
HudMessageCommand::HudMessageCommand() = default;

// @func 0x0007D3A0 HudMessageCommand::Create
ApeAnimScriptCommand* HudMessageCommand::Create(XmlElement* element) const
{
    auto* command = new HudMessageCommand();
    const char* msg = nullptr;
    element->GetStringOrNumber(kAttrMsg, &msg, "");
    if (msg && msg[0])
        ape::StrCopyBounded(command->m_msg, msg, sizeof(command->m_msg));
    element->GetHash(kAttrMsgVar, &command->m_msgVar, command->m_msgVar);
    element->GetHash(kAttrHudObj, &command->m_hudObj, command->m_hudObj);
    element->GetHash(kAttrAnimMode, &command->m_animMode, command->m_animMode);
    element->GetFloat(kAttrAnimRate, &command->m_animRate, command->m_animRate);
    element->GetInt(kAttrIndex, &command->m_index, command->m_index);
    return command;
}

// TODO(0x0007D540 HudMessageCommand::Execute)

// ---------------------------------------------------------------------------
// Hash 0xD1CD23E6 (name unknown): sets the animation mode and rate of a HUD object.

class HudAnimModeCommand : public ApeAnimScriptCommand {
public:
    HudAnimModeCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_hudObj = 0;      // +0x0C "HudObj"
    uint32_t m_animMode = 0;    // +0x10 "AnimMode"
    float m_animRate = -1.0f;   // +0x14 "AnimRate"
};

// @func 0x0007D678 HudAnimModeCommand::HudAnimModeCommand
HudAnimModeCommand::HudAnimModeCommand() = default;

// @func 0x0007D6D0 HudAnimModeCommand::Create
ApeAnimScriptCommand* HudAnimModeCommand::Create(XmlElement* element) const
{
    auto* command = new HudAnimModeCommand();
    element->GetHash(kAttrHudObj, &command->m_hudObj, 0);
    element->GetHash(kAttrAnimMode, &command->m_animMode, command->m_animMode);
    element->GetFloat(kAttrAnimRate, &command->m_animRate, command->m_animRate);
    return command;
}

// @func 0x0007D7C8 HudAnimModeCommand::Execute
void HudAnimModeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    void* hud = FindHudObject(m_hudObj);
    if (!hud)
        return;
    if (m_animMode)
        HudSetAnimMode(hud, m_animMode);
    HudSetAnimRate(hud, m_animRate);
}

// ---------------------------------------------------------------------------
// Hash 0x244FEDA4 (name unknown): switches the script state of a HUD object.

class HudSetStateCommand : public ApeAnimScriptCommand {
public:
    HudSetStateCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_hudObj = 0;  // +0x0C "HudObj"
    uint32_t m_state = 0;   // +0x10 "state"
};

// @func 0x0007D83C HudSetStateCommand::HudSetStateCommand
HudSetStateCommand::HudSetStateCommand() = default;

// @func 0x0007D884 HudSetStateCommand::Create
ApeAnimScriptCommand* HudSetStateCommand::Create(XmlElement* element) const
{
    auto* command = new HudSetStateCommand();
    element->GetHash(kAttrHudObj, &command->m_hudObj, 0);
    element->GetHash(kAttrState, &command->m_state, command->m_state);
    return command;
}

// @func 0x0007D94C HudSetStateCommand::Execute
void HudSetStateCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    void* hud = FindHudObject(m_hudObj);
    if (!hud)
        return;
    if (ApeAnimScriptInstance* script = HudScriptInstance(hud))
        script->SetStateAndUpdate(m_state);
    else
        ScriptWarning("%s", m_hudObj);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsH()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("GiveAward"), new GiveAwardCommand());
    prototypes.Add(0x553CAE39u, new VehicleInputCommand());
    prototypes.Add(AnimHash("HudHealthMeter"), new HudHealthMeterCommand());
    prototypes.Add(0xE573697Fu, new HudPlayAnimCommand());
    prototypes.Add(0x742543CFu, new HudMessageCommand());
    prototypes.Add(0xD1CD23E6u, new HudAnimModeCommand());
    prototypes.Add(0x244FEDA4u, new HudSetStateCommand());
}

} // namespace levels
} // namespace sly
