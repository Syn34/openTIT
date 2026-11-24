// openTIT - Game script commands, ninth block (MapMaterialToVar to PlayerEnableAllButtons).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

#include <cstring>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrMaterialVar = AnimHash("MaterialVar");
constexpr uint32_t kAttrObjectVar = AnimHash("ObjectVar");
constexpr uint32_t kAttrMaterial = AnimHash("material");
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrEnabled = AnimHash("enabled");
constexpr uint32_t kAttrColor = AnimHash("Color");
constexpr uint32_t kAttrPause = AnimHash("pause");
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrEntityChild = AnimHash("EntityChild");
constexpr uint32_t kAttrClip = AnimHash("clip");
constexpr uint32_t kAttrAnimation = AnimHash("animation");
constexpr uint32_t kAttrClipVar = AnimHash("ClipVar");
constexpr uint32_t kAttrAnimationVar = AnimHash("AnimationVar");
constexpr uint32_t kAttrLibrary = AnimHash("Library");
constexpr uint32_t kAttrLayer = AnimHash("layer");
constexpr uint32_t kAttrLayerVar = AnimHash("LayerVar");
constexpr uint32_t kAttrPlayAnim34 = 0x375509ACu;  // NOTE: name unknown
constexpr uint32_t kAttrBlend = AnimHash("Blend");
constexpr uint32_t kAttrSpeed = AnimHash("Speed");
constexpr uint32_t kAttrStartTime = AnimHash("StartTime");
constexpr uint32_t kAttrSpeedVar = AnimHash("speedVar");
constexpr uint32_t kAttrPlayAnim48 = 0xA4871DCBu;  // NOTE: name unknown (float variable)
constexpr uint32_t kAttrChannelMap = AnimHash("ChannelMap");
constexpr uint32_t kAttrSkeletonMap = AnimHash("SkeletonMap");
constexpr uint32_t kAttrPlayAnimBit31 = 0xC1B18F57u;  // NOTE: name unknown, default true
constexpr uint32_t kAttrPlayAnimBit30 = 0xE588ABCCu;  // NOTE: name unknown
constexpr uint32_t kAttrPlayAnimMode = 0x96987C96u;   // NOTE: name unknown; value "Loop" loops
constexpr uint32_t kValueLoop = AnimHash("Loop");
constexpr uint32_t kLibraryDefault = AnimHash("Default");
constexpr uint32_t kAttrPlayerNum = AnimHash("PlayerNum");
constexpr uint32_t kAttrEnable = AnimHash("enable");
constexpr uint32_t kAttrButton = AnimHash("Button");
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrScale = AnimHash("Scale");
constexpr uint32_t kAttrOffset = AnimHash("Offset");

// Default colour of OverrideColor (console global 0x01164CA0, see NOTES.md).
constexpr ape::xml::Color8 kDefaultColor = {0xFF, 0xFF, 0xFF, 0xFF};

constexpr int kMaxPlayerNum = 8;

int ClampPlayerNum(int32_t value)
{
    return (value < 0 || value > kMaxPlayerNum) ? 0 : value;
}
} // namespace

// ---------------------------------------------------------------------------
// "MapMaterialToVar": stores a material (by name, optionally searched on an
// object) in a material variable.

class MapMaterialToVarCommand : public ApeAnimScriptCommand {
public:
    MapMaterialToVarCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_materialVar = 0;  // +0x0C "MaterialVar"
    uint32_t m_objectVar = 0;    // +0x10 "ObjectVar"
    uint32_t m_material = 0;     // +0x14 "material"
};

// @func 0x000812C0 MapMaterialToVarCommand::MapMaterialToVarCommand
MapMaterialToVarCommand::MapMaterialToVarCommand() = default;

// @func 0x00081310 MapMaterialToVarCommand::Create
ApeAnimScriptCommand* MapMaterialToVarCommand::Create(XmlElement* element) const
{
    auto* command = new MapMaterialToVarCommand();
    element->GetHash(kAttrMaterialVar, &command->m_materialVar, 0);
    element->GetHash(kAttrObjectVar, &command->m_objectVar, 0);
    element->GetHash(kAttrMaterial, &command->m_material, 0);
    return command;
}

// TODO(0x00081400 MapMaterialToVarCommand::Execute) searches the global AglMaterial
// list (or the materials of the object's model) for the name.

// ---------------------------------------------------------------------------
// Hash 0x27D06D5B (name unknown): no parameters.

class WorldResumeCommand : public ApeAnimScriptCommand {
public:
    WorldResumeCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// @func 0x00081714 WorldResumeCommand::WorldResumeCommand
WorldResumeCommand::WorldResumeCommand() = default;

// @func 0x00081754 WorldResumeCommand::Create
ApeAnimScriptCommand* WorldResumeCommand::Create(XmlElement*) const
{
    return new WorldResumeCommand();
}

// @func 0x000817D4 WorldResumeCommand::Execute
void WorldResumeCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GameWorldResumeObject12847C();
}

// ---------------------------------------------------------------------------
// "OpenBinocucom": puts the player into the binocucom.

class OpenBinocucomCommand : public ApeAnimScriptCommand {
public:
    OpenBinocucomCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;
};

// @func 0x00081800 OpenBinocucomCommand::OpenBinocucomCommand
OpenBinocucomCommand::OpenBinocucomCommand() = default;

// @func 0x00081840 OpenBinocucomCommand::Create
ApeAnimScriptCommand* OpenBinocucomCommand::Create(XmlElement*) const
{
    return new OpenBinocucomCommand();
}

// @func 0x000818C0 OpenBinocucomCommand::Execute
void OpenBinocucomCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (WorldObject* player = PlayerEntity()) {
        EntityVirtualCommand(player, 0, 0);
        EntityVirtualCommand(player, 1, 7);
    }
}

// ---------------------------------------------------------------------------
// "OverrideColor": tints an entity (and its outline/glow parts).

class OverrideColorCommand : public ApeAnimScriptCommand {
public:
    OverrideColorCommand();
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;                     // +0x0C "object"
    bool m_enabled = false;                    // +0x10 "enabled"
    ape::xml::Color8 m_color = kDefaultColor;  // +0x11 "Color"
};

// @func 0x00081958 OverrideColorCommand::OverrideColorCommand
OverrideColorCommand::OverrideColorCommand() = default;

// @func 0x000819D8 OverrideColorCommand::Create
ApeAnimScriptCommand* OverrideColorCommand::Create(XmlElement* element) const
{
    auto* command = new OverrideColorCommand();
    element->GetHash(kAttrObject, &command->m_object, command->m_object);
    element->GetBool(kAttrEnabled, &command->m_enabled, command->m_enabled);
    element->GetColor(kAttrColor, &command->m_color, &kDefaultColor);
    return command;
}

// TODO(0x00081B00 OverrideColorCommand::Execute) writes the colour into several
// entity-class specific fields (character +0x3C8, class flags 0x800/0x200/0x400/0x100).

// ---------------------------------------------------------------------------
// Hash 0x3173666E (name unknown): sets the game state's pause flag.

class SetPauseFlagCommand : public ApeAnimScriptCommand {
public:
    SetPauseFlagCommand();
    ~SetPauseFlagCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    bool m_pause = true;  // +0x0C "pause"
};

// @func 0x00081E90 SetPauseFlagCommand::SetPauseFlagCommand
SetPauseFlagCommand::SetPauseFlagCommand() = default;

// @func 0x00081ED8 SetPauseFlagCommand::~SetPauseFlagCommand
// @func 0x00081F08 SetPauseFlagCommand::~SetPauseFlagCommand (deleting)
SetPauseFlagCommand::~SetPauseFlagCommand() = default;

// @func 0x00081F60 SetPauseFlagCommand::Create
ApeAnimScriptCommand* SetPauseFlagCommand::Create(XmlElement* element) const
{
    auto* command = new SetPauseFlagCommand();
    element->GetBool(kAttrPause, &command->m_pause, true);
    return command;
}

// @func 0x00082008 SetPauseFlagCommand::Execute
void SetPauseFlagCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GameStateSetPauseFlag(m_pause);
}

// ---------------------------------------------------------------------------
// "GamePlayAnim": plays an animation clip on an entity (or object).

class GamePlayAnimCommand : public ApeAnimScriptCommand {
public:
    GamePlayAnimCommand();
    explicit GamePlayAnimCommand(XmlElement* element);
    ~GamePlayAnimCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;       // +0x0C "object"
    uint32_t m_entity = 0;       // +0x10 "Entity"
    uint32_t m_entityChild = 0;  // +0x14 "EntityChild"
    uint32_t m_clip = 0;         // +0x18 "clip" / "animation"
    uint32_t m_clipVar = 0;      // +0x1C "ClipVar" / "AnimationVar"
    uint32_t m_library = 0;      // +0x24 "Library"
    bool m_loop = false;         // +0x28 attribute 0x96987C96 == "Loop"
    uint32_t m_layer = 0;        // +0x2C "layer"
    uint32_t m_layerVar = 0;     // +0x30 "LayerVar"
    bool m_flag34 = false;       // +0x34 attribute 0x375509AC
    float m_blend = 0.0f;        // +0x38 "Blend"
    float m_speed = 1.0f;        // +0x3C "Speed"
    float m_startTime = 0.0f;    // +0x40 "StartTime"
    uint32_t m_speedVar = 0;     // +0x44 "speedVar"
    uint32_t m_var48 = 0;        // +0x48 attribute 0xA4871DCB
    uint32_t m_channelMap = 0;   // +0x4C "ChannelMap"
    uint32_t m_skeletonMap = 0;  // +0x50 "SkeletonMap"
    // +0x54 bit field.
    bool m_bit31 = true;         // bit 31 attribute 0xC1B18F57
    bool m_bit30 = false;        // bit 30 attribute 0xE588ABCC
};

// @func 0x0008202C GamePlayAnimCommand::GamePlayAnimCommand
GamePlayAnimCommand::GamePlayAnimCommand() = default;

// @func 0x000820E0 GamePlayAnimCommand::GamePlayAnimCommand
GamePlayAnimCommand::GamePlayAnimCommand(XmlElement* element)
{
    if (element->GetHash(kAttrObject, &m_object, 0) != ape::xml::kQuery_Ok &&
        element->GetHash(kAttrEntity, &m_entity, 0) != ape::xml::kQuery_Ok) {
        // Neither operand: the command plays from the default library only.
        m_library = kLibraryDefault;
        return;
    }
    element->GetHash(kAttrEntityChild, &m_entityChild, 0);

    uint32_t value = 0;
    if (element->GetHash(kAttrClip, &value, 0) == ape::xml::kQuery_Ok ||
        element->GetHash(kAttrAnimation, &value, 0) == ape::xml::kQuery_Ok)
        m_clip = value;
    if (element->GetHash(kAttrClipVar, &value, 0) == ape::xml::kQuery_Ok ||
        element->GetHash(kAttrAnimationVar, &value, 0) == ape::xml::kQuery_Ok)
        m_clipVar = value;

    element->GetHash(kAttrLibrary, &m_library, 0);
    element->GetHash(kAttrLayer, &m_layer, 0);
    element->GetHash(kAttrLayerVar, &m_layerVar, 0);
    element->GetBool(kAttrPlayAnim34, &m_flag34, false);
    element->GetFloat(kAttrBlend, &m_blend, 0.0f);
    element->GetFloat(kAttrSpeed, &m_speed, 1.0f);
    element->GetFloat(kAttrStartTime, &m_startTime, 0.0f);
    element->GetHash(kAttrSpeedVar, &m_speedVar, 0);
    element->GetHash(kAttrPlayAnim48, &m_var48, 0);
    element->GetHash(kAttrChannelMap, &m_channelMap, 0);
    element->GetHash(kAttrSkeletonMap, &m_skeletonMap, m_skeletonMap);
    element->GetBool(kAttrPlayAnimBit31, &m_bit31, m_bit31);
    element->GetBool(kAttrPlayAnimBit30, &m_bit30, m_bit30);

    uint32_t mode = 0;
    if (element->GetHash(kAttrPlayAnimMode, &mode, 0) == ape::xml::kQuery_Ok)
        m_loop = (mode == kValueLoop);
}

// @func 0x00082534 GamePlayAnimCommand::~GamePlayAnimCommand
// @func 0x00082564 GamePlayAnimCommand::~GamePlayAnimCommand (deleting)
GamePlayAnimCommand::~GamePlayAnimCommand() = default;

// @func 0x000825BC GamePlayAnimCommand::Create
ApeAnimScriptCommand* GamePlayAnimCommand::Create(XmlElement* element) const
{
    return new GamePlayAnimCommand(element);
}

// TODO(0x00082638 GamePlayAnimCommand::Execute) "GamePlayAnim can't find Entity '%s'":
// character animation controller (0x002775A8 / 0x002983C0) or the entity's animation
// player (0x0034A398 clip lookup, 0x003514AC / 0x00350958 / 0x003535C4 play).

// ---------------------------------------------------------------------------
// "PlayerEnableAllButtons": turns all controller buttons of a player on or off.

class PlayerEnableAllButtonsCommand : public ApeAnimScriptCommand {
public:
    PlayerEnableAllButtonsCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerNum = 0;  // +0x0C "PlayerNum" (0..8)
    bool m_enable = false;    // +0x10 "enable"
};

// @func 0x00082D7C PlayerEnableAllButtonsCommand::PlayerEnableAllButtonsCommand
PlayerEnableAllButtonsCommand::PlayerEnableAllButtonsCommand() = default;

// @func 0x00082DCC PlayerEnableAllButtonsCommand::Create
ApeAnimScriptCommand* PlayerEnableAllButtonsCommand::Create(XmlElement* element) const
{
    auto* command = new PlayerEnableAllButtonsCommand();
    element->GetBool(kAttrEnable, &command->m_enable, false);
    element->GetInt(kAttrPlayerNum, &command->m_playerNum, command->m_playerNum);
    command->m_playerNum = ClampPlayerNum(command->m_playerNum);
    return command;
}

// @func 0x00082EB4 PlayerEnableAllButtonsCommand::Execute
void PlayerEnableAllButtonsCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    // NOTE: the console does not check for a missing player here.
    WorldObject* player = PlayerEntityByIndex(m_playerNum);
    if (player && EntityIsCharacter(player))
        PlayerEnableAllButtons(player, m_enable);
}

// ---------------------------------------------------------------------------
// "PlayerEnableButton": turns one controller button of a player on or off.

class PlayerEnableButtonCommand : public ApeAnimScriptCommand {
public:
    PlayerEnableButtonCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerNum = 0;  // +0x0C "PlayerNum"
    uint32_t m_button = 0;    // +0x10 "Button"
    bool m_enable = false;    // +0x14 "enable"
};

// @func 0x00082F58 PlayerEnableButtonCommand::PlayerEnableButtonCommand
PlayerEnableButtonCommand::PlayerEnableButtonCommand() = default;

// @func 0x00082FA8 PlayerEnableButtonCommand::Create
ApeAnimScriptCommand* PlayerEnableButtonCommand::Create(XmlElement* element) const
{
    auto* command = new PlayerEnableButtonCommand();
    int32_t playerNum = 0;
    element->GetInt(kAttrPlayerNum, &playerNum, 0);
    command->m_playerNum = ClampPlayerNum(playerNum);
    element->GetHash(kAttrButton, &command->m_button, command->m_button);
    element->GetBool(kAttrEnable, &command->m_enable, command->m_enable);
    return command;
}

// @func 0x000830BC PlayerEnableButtonCommand::Execute
void PlayerEnableButtonCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    WorldObject* player = PlayerEntityByIndex(m_playerNum);
    if (player && EntityIsCharacter(player))
        PlayerEnableButton(player, m_button, m_enable);
}

// ---------------------------------------------------------------------------
// Hash 0xC18490E5 (name unknown): switches a player's control on or off.

class PlayerEnableControlCommand : public ApeAnimScriptCommand {
public:
    PlayerEnableControlCommand();
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    int32_t m_playerNum = 0;  // +0x0C "PlayerNum"
    bool m_enable = false;    // +0x10 "enable"
};

// @func 0x00083154 PlayerEnableControlCommand::PlayerEnableControlCommand
PlayerEnableControlCommand::PlayerEnableControlCommand() = default;

// @func 0x000831A4 PlayerEnableControlCommand::Create
ApeAnimScriptCommand* PlayerEnableControlCommand::Create(XmlElement* element) const
{
    auto* command = new PlayerEnableControlCommand();
    element->GetBool(kAttrEnable, &command->m_enable, false);
    element->GetInt(kAttrPlayerNum, &command->m_playerNum, command->m_playerNum);
    command->m_playerNum = ClampPlayerNum(command->m_playerNum);
    return command;
}

// @func 0x0008328C PlayerEnableControlCommand::Execute
void PlayerEnableControlCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (WorldObject* player = PlayerEntityByIndex(m_playerNum))
        PlayerSetControlEnabled(player, m_enable);
}

// ---------------------------------------------------------------------------
// Hash 0xF1F9B0AA (name unknown): shows a named overlay picture.

class ShowOverlayCommand : public ApeAnimScriptCommand {
public:
    ShowOverlayCommand();
    ~ShowOverlayCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    char m_name[0x80] = {};               // +0x0C "name"
    float m_scale[2] = {1.0f, 1.0f};      // +0x90 "Scale" (NOTE: default global 0x01164AF4, assumed 1,1)
    float m_offset[2] = {0.0f, 0.0f};     // +0x98 "Offset" (NOTE: default global 0x01164AFC, assumed 0,0)
};

// @func 0x0008331C ShowOverlayCommand::ShowOverlayCommand
ShowOverlayCommand::ShowOverlayCommand() = default;

// @func 0x0008339C ShowOverlayCommand::~ShowOverlayCommand
// @func 0x000833CC ShowOverlayCommand::~ShowOverlayCommand (deleting)
ShowOverlayCommand::~ShowOverlayCommand() = default;

// @func 0x00083424 ShowOverlayCommand::Create
ApeAnimScriptCommand* ShowOverlayCommand::Create(XmlElement* element) const
{
    auto* command = new ShowOverlayCommand();
    const char* name = element->GetString("name");
    if (name && *name) {
        std::strncpy(command->m_name, name, sizeof(command->m_name) - 1);
        command->m_name[sizeof(command->m_name) - 1] = '\0';
    }
    element->GetVec2(kAttrScale, command->m_scale, command->m_scale);
    element->GetVec2(kAttrOffset, command->m_offset, command->m_offset);
    return command;
}

// @func 0x00083560 ShowOverlayCommand::Execute
void ShowOverlayCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    GameWorldShowOverlay(m_name, m_scale, m_offset);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsK()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("MapMaterialToVar"), new MapMaterialToVarCommand());
    prototypes.Add(0x27D06D5Bu, new WorldResumeCommand());
    prototypes.Add(AnimHash("OpenBinocucom"), new OpenBinocucomCommand());
    prototypes.Add(AnimHash("OverrideColor"), new OverrideColorCommand());
    prototypes.Add(0x3173666Eu, new SetPauseFlagCommand());
    prototypes.Add(AnimHash("GamePlayAnim"), new GamePlayAnimCommand());
    prototypes.Add(AnimHash("PlayerEnableAllButtons"), new PlayerEnableAllButtonsCommand());
    prototypes.Add(AnimHash("PlayerEnableButton"), new PlayerEnableButtonCommand());
    prototypes.Add(0xC18490E5u, new PlayerEnableControlCommand());
    prototypes.Add(0xF1F9B0AAu, new ShowOverlayCommand());
}

} // namespace levels
} // namespace sly
