// openTIT - Built-in anim script commands that only touch the script system.
//
//   IfEquals / IfRange  run nested commands depending on a variable
//   Call                start another script as a child (or detached) instance
//   StopChildScript     destroy child instances
//   ChangeState         switch the instance (or a child) to another state
//   SetFloat / SetInt   arithmetic on a variable
#pragma once

#include "ape/anim/ApeAnimScript.h"
#include "ape/math/ApeMath.h"

namespace ape {

// Arithmetic of SetFloat/SetInt ("operation" attribute).
enum class AnimScriptOperation : uint8_t {
    Set = 0,
    Add = 1,
    Subtract = 2,
    Multiply = 3,
    Divide = 4,
};

class ApeAnimScriptIfEqualsCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptIfEqualsCommand() = default;
    explicit ApeAnimScriptIfEqualsCommand(xml::XmlElement* element);
    ~ApeAnimScriptIfEqualsCommand() override;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_hashValue = 0;
    Vec3 m_vecValue = {0.0f, 0.0f, 0.0f, 0.0f};
    float m_floatValue = 0.0f;
    uint32_t m_variable = 0;      // "variable"
    AnimScriptCommandList m_commands;
    bool m_reverse = false;       // "Reverse": run when not equal
};

class ApeAnimScriptIfRangeCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptIfRangeCommand() = default;
    explicit ApeAnimScriptIfRangeCommand(xml::XmlElement* element);
    ~ApeAnimScriptIfRangeCommand() override;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    void Link() override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_variable = 0;      // "variable"
    float m_min = -1.0e7f;        // "min"
    float m_max = 1.0e7f;         // "max"
    AnimScriptCommandList m_commands;
    bool m_reverse = false;       // "Reverse": run when outside the range
};

class ApeAnimScriptCallCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptCallCommand() = default;
    explicit ApeAnimScriptCallCommand(xml::XmlElement* element);
    ~ApeAnimScriptCallCommand() override;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_script = 0;        // "script"
    uint32_t m_scriptVar = 0;     // "ScriptVar": int variable holding the script name
    uint32_t m_scriptLib = 0;     // "ScriptLib"
    // "Param" children: callee variable name -> caller variable name.
    AVLMap<uint32_t, uint32_t> m_params;
    bool m_detach = false;        // "detach": run from the global list
};

class ApeAnimScriptStopChildScriptCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptStopChildScriptCommand() = default;
    ~ApeAnimScriptStopChildScriptCommand() override;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_child = 0;         // "child": script name of the children to stop
    // NOTE: attribute 0x2A9C853E (name unknown): stop every child.
    bool m_all = false;
};

// "CallRandom": starts one of several scripts, picked at random by weight,
// as a child of the running instance.
class ApeAnimScriptCallRandomCommand : public ApeAnimScriptCommand {
public:
    static constexpr uint32_t kMaxScripts = 32;
    static constexpr uint32_t kMaxParams = 4;

    ApeAnimScriptCallRandomCommand();
    explicit ApeAnimScriptCallRandomCommand(xml::XmlElement* element);
    ~ApeAnimScriptCallRandomCommand() override = default;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    struct Choice {
        float weight;             // "Weight"
        uint32_t script;          // "Script"
        uint32_t library;         // "ScriptLib"
    };
    struct Param {
        uint32_t name;            // "name": callee variable
        uint32_t value;           // "Value": caller variable
    };
    std::vector<Choice> m_choices;  // "Script" children
    std::vector<Param> m_params;    // "Param" children
    float m_totalWeight = 0.0f;
    // NOTE: attribute 0xEE1B3C3B (name unknown): weights are percentages of
    // 100, so a roll past the listed weights starts nothing.
    bool m_percent = false;
};

class ApeAnimScriptChangeStateCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptChangeStateCommand() = default;
    explicit ApeAnimScriptChangeStateCommand(xml::XmlElement* element);
    ~ApeAnimScriptChangeStateCommand() override;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    std::vector<uint32_t> m_randomStates;  // "state" children when Random
    uint32_t m_state = 0;         // "state"
    uint32_t m_childScript = 0;   // "ChildScript": change a child instead
    uint32_t m_delayTimeVar = 0;  // "DelayTimeVar"
    float m_delayTime = 0.0f;     // "DelayTime": state time before switching
    bool m_random = false;        // "Random"
};

class ApeAnimScriptSetFloatCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptSetFloatCommand() = default;
    explicit ApeAnimScriptSetFloatCommand(xml::XmlElement* element);
    ~ApeAnimScriptSetFloatCommand() override;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_variable = 0;      // "variable"
    uint32_t m_valueVar = 0;      // "ValueVar": float variable used as operand
    float m_value = 0.0f;         // "Value"
    AnimScriptOperation m_operation = AnimScriptOperation::Set;
};

class ApeAnimScriptSetIntCommand : public ApeAnimScriptCommand {
public:
    ApeAnimScriptSetIntCommand() = default;
    explicit ApeAnimScriptSetIntCommand(xml::XmlElement* element);
    ~ApeAnimScriptSetIntCommand() override;

    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptCommand* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_variable = 0;      // "variable"
    int32_t m_value = 0;          // "str" (as hash) or "Value"
    AnimScriptOperation m_operation = AnimScriptOperation::Set;
};

// Parses the "operation" attribute (missing or unknown keeps Set).
AnimScriptOperation ReadOperation(xml::XmlElement* element);

} // namespace ape
