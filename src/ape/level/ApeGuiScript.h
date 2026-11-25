// openTIT - GUI scripts: timed sequences of property animations and events.
//
// A GUI file's <scripts> section holds <script name loop> elements. Each
// child is a step that starts a given time ("delay") after the previous
// one:
//   * <delay time>          only waits;
//   * <interp ...>/<set ...> animates one property of an object ("target",
//     a path relative to the player's scope object) from "from" (default:
//     the current value) to "to"/"value" over "duration" seconds;
//   * a message step sends a GUI event ("msg") about an object to the game.
// Playing a script creates a player (kept in a global list) that starts
// the steps as their time comes and updates the property animations they
// spawn until all are finished.
#pragma once

#include "ape/level/ApeGui.h"

#include <cstdint>

namespace ape {

class GuiScriptPlayer;
class GuiScriptAction;

// Animated property of a property step ("property" attribute).
enum GuiScriptProperty : int32_t {
    kGuiProp_None = 0,
    kGuiProp_Position, kGuiProp_PositionX, kGuiProp_PositionY, kGuiProp_PositionZ,
    kGuiProp_Rotation,
    kGuiProp_Color, kGuiProp_ColorR, kGuiProp_ColorG, kGuiProp_ColorB, kGuiProp_ColorA,
    kGuiProp_ColorTL, kGuiProp_ColorTR, kGuiProp_ColorBL, kGuiProp_ColorBR,
    kGuiProp_Scale, kGuiProp_ScaleX, kGuiProp_ScaleY,
    kGuiProp_Size, kGuiProp_SizeX, kGuiProp_SizeY,
    kGuiProp_Visible, kGuiProp_Flag30, kGuiProp_Expand, kGuiProp_Anchor, kGuiProp_Align,
    kGuiProp_Count
};

// "ease" of a property step.
enum GuiScriptEase : int32_t {
    kGuiEase_Linear = 0,
    kGuiEase_Sin = 1,    // cosine ease in and out
    kGuiEase_Log = 2,    // NOTE: accepted but has no effect
    kGuiEase_Noise = 3,  // random on/off flicker with the progress as probability
};

// "loop" of a property step.
enum GuiScriptLoop : int32_t {
    kGuiLoop_None = 0,
    kGuiLoop_Repeat = 1,
    kGuiLoop_PingPong = 2,
};

// A value of a property step: up to four floats, a colour or an integer.
union GuiScriptValue {
    float f[4];
    int32_t i;
    uint32_t u;
    xml::Color8 color;
};

// Base of the steps (vtable 0x00B228B0).
class GuiScriptStep {
public:
    explicit GuiScriptStep(float startDelay) : delay(startDelay) {}
    virtual ~GuiScriptStep();
    GuiScriptStep(const GuiScriptStep&) = delete;
    GuiScriptStep& operator=(const GuiScriptStep&) = delete;
    // Starts the step for 'player'; returns an action to update, if any.
    virtual GuiScriptAction* Start(GuiScriptPlayer* player) = 0;

    float delay = 0.0f;              // +0x04 seconds after the previous step
    GuiScriptStep* next = nullptr;   // +0x08
};

// <delay> (vtable 0x00B228D8, 0x0C bytes).
class GuiScriptDelay : public GuiScriptStep {
public:
    using GuiScriptStep::GuiScriptStep;
    ~GuiScriptDelay() override;
    GuiScriptAction* Start(GuiScriptPlayer* player) override;
};

// <interp>/<set> (vtable 0x00B228F0, 0x80 bytes).
class GuiScriptPropertyStep : public GuiScriptStep {
public:
    using GuiScriptStep::GuiScriptStep;
    ~GuiScriptPropertyStep() override;
    GuiScriptAction* Start(GuiScriptPlayer* player) override;

    int32_t property = kGuiProp_None; // +0x0C GuiScriptProperty
    int32_t ease = kGuiEase_Linear;   // +0x10
    int32_t loop = kGuiLoop_None;     // +0x14
    float duration = 0.0f;            // +0x18 "duration"
    char target[0x40] = {};           // +0x1C object path
    GuiScriptValue from{};            // +0x60 FLT_MAX in f[0]: the current value
    GuiScriptValue to{};              // +0x70
};

// Message step (element hash 0xA0CED6D9, vtable 0x00B22908, 0x50 bytes):
// sends GUI event 'message' about the target object, with the player as
// the event argument.
class GuiScriptMessageStep : public GuiScriptStep {
public:
    using GuiScriptStep::GuiScriptStep;
    ~GuiScriptMessageStep() override;
    GuiScriptAction* Start(GuiScriptPlayer* player) override;

    uint32_t message = 0;             // +0x0C index of the "msg" name
    char target[0x40] = {};           // +0x10
};

// Something a running step keeps doing (vtable 0x00B228C4).
class GuiScriptAction {
public:
    GuiScriptAction(GuiScriptStep* ownerStep, GuiScriptPlayer* ownerPlayer)
        : step(ownerStep), player(ownerPlayer) {}
    virtual ~GuiScriptAction();
    GuiScriptAction(const GuiScriptAction&) = delete;
    GuiScriptAction& operator=(const GuiScriptAction&) = delete;
    // Advances by 'dt'; false once finished.
    virtual bool Update(float dt) = 0;

    GuiScriptStep* step = nullptr;      // +0x04
    GuiScriptPlayer* player = nullptr;  // +0x08
    float time = 0.0f;                  // +0x0C
    GuiScriptAction* next = nullptr;    // +0x10
};

// The animation of one property (vtable 0x00B22F70, 0x30 bytes).
class GuiPropertyAction : public GuiScriptAction {
public:
    GuiPropertyAction(GuiScriptPropertyStep* step, GuiScriptPlayer* player, ApeGuiObject* target);
    ~GuiPropertyAction() override;
    bool Update(float dt) override;

    ApeGuiObject* target = nullptr;     // +0x14
    GuiScriptValue from{};              // +0x20
};

// A <script> (vtable 0x00B22F60, 0x38 bytes), kept in a global list.
class GuiScript {
public:
    GuiScript(const char* name, void* owner);
    virtual ~GuiScript();
    GuiScript(const GuiScript&) = delete;
    GuiScript& operator=(const GuiScript&) = delete;

    char name[0x20] = {};               // +0x04
    uint32_t nameHash = 0;              // +0x24
    void* owner = nullptr;              // +0x28 GUI file
    bool loop = false;                  // +0x2C "loop": restart after the last step
    GuiScriptStep* firstStep = nullptr; // +0x30
    GuiScript* nextScript = nullptr;    // +0x34

    static GuiScript* First();
    static GuiScript* FindByHash(uint32_t nameHash);
};

// A playing script (vtable 0x00B228A0, 0x20 bytes), kept in a global list.
class GuiScriptPlayer {
public:
    GuiScriptPlayer(GuiScript* script, ApeGuiObject* scope, bool autoDelete, bool ownsScope);
    virtual ~GuiScriptPlayer();
    GuiScriptPlayer(const GuiScriptPlayer&) = delete;
    GuiScriptPlayer& operator=(const GuiScriptPlayer&) = delete;

    // Starts due steps and updates the actions; false when the script is
    // over.
    bool Tick(float dt);
    // Deletes every running action and forgets the remaining steps.
    void Stop();

    GuiScript* script = nullptr;            // +0x04
    ApeGuiObject* scope = nullptr;          // +0x08 targets are looked up from here
    bool autoDelete = false;                // +0x0C delete the player when the script is over
    bool ownsScope = false;                 // +0x0D delete the scope object with the player
    GuiScriptStep* currentStep = nullptr;   // +0x10 next step to start
    float stepDelay = 0.0f;                 // +0x14 time until it starts
    GuiScriptAction* actions = nullptr;     // +0x18
    GuiScriptPlayer* nextPlayer = nullptr;  // +0x1C

    static GuiScriptPlayer* First();
};

// Creates a script from a <script> element; null without a name.
GuiScript* CreateGuiScript(xml::XmlDocument* doc, xml::XmlElement* element, void* owner);
// Creates one step of a script element.
GuiScriptStep* CreateGuiScriptStep(xml::XmlDocument* doc, xml::XmlElement* element, GuiScript* script);

// Plays the script with the name (hash); null when there is none.
GuiScriptPlayer* PlayGuiScript(uint32_t nameHash, ApeGuiObject* scope, bool autoDelete, bool ownsScope);
GuiScriptPlayer* PlayGuiScript(const char* name, ApeGuiObject* scope, bool autoDelete, bool ownsScope);
// Stops the players of the script (0: any) with the scope (null: any).
void StopGuiScripts(uint32_t nameHash, ApeGuiObject* scope);
void StopGuiScripts(const char* name, ApeGuiObject* scope);

// Ticks every player (deleting the finished auto-delete ones).
void TickGuiScripts(float dt);
// Deletes every player, then every script.
void DestroyGuiScripts();
// Deletes the scripts created by a GUI file.
void DestroyGuiScriptsOf(void* owner);

} // namespace ape
